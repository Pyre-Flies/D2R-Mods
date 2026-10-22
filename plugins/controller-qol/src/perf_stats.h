#pragma once
#include <windows.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>

// Kept source-identical in the independent UnHoarder fork. No shared runtime
// state or provider dependency. Hot paths only read QPC and update atomics;
// formatting, locking and output happen when the user requests a report.
namespace LootPerf {
inline std::uint64_t Now() noexcept {
    LARGE_INTEGER value{};
    QueryPerformanceCounter(&value);
    return static_cast<std::uint64_t>(value.QuadPart);
}
inline double Frequency() noexcept {
    static const double frequency=[] {
        LARGE_INTEGER value{};
        QueryPerformanceFrequency(&value);
        return value.QuadPart>0?static_cast<double>(value.QuadPart):1.0;
    }();
    return frequency;
}
struct Snapshot {std::uint64_t calls{},ticks{},units{},peak{};};
struct Counter {
    std::atomic<std::uint64_t> calls{},ticks{},units{},peak{};
    void Record(std::uint64_t elapsed,std::uint64_t items=0) noexcept {
        ticks.fetch_add(elapsed,std::memory_order_relaxed);
        if(items)units.fetch_add(items,std::memory_order_relaxed);
        auto high=peak.load(std::memory_order_relaxed);
        while(high<elapsed && !peak.compare_exchange_weak(high,elapsed,std::memory_order_relaxed)) {}
        calls.fetch_add(1,std::memory_order_relaxed);
    }
    Snapshot Read() const noexcept {
        return {calls.load(std::memory_order_relaxed),ticks.load(std::memory_order_relaxed),
            units.load(std::memory_order_relaxed),peak.load(std::memory_order_relaxed)};
    }
};
class Scope {
    Counter& counter;
    const std::uint64_t started=Now();
    std::uint64_t units{1};
public:
    explicit Scope(Counter& target) noexcept:counter(target) {}
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
    void Items(std::uint64_t count) noexcept {units=count;}
    ~Scope() noexcept {
        const auto ended=Now();
        counter.Record(ended>=started?ended-started:0,units);
    }
};
struct Description {const char* name;const char* units;};
template<std::size_t Size> class ReportWindow {
    std::mutex mutex;
    std::uint64_t since=Now();
    std::array<Snapshot,Size> previous{};
public:
    template<class Emit> void Print(const char* prefix,const char* version,
        const std::array<Description,Size>& names,const std::array<Counter,Size>& counters,
        Emit emit) noexcept {
        const std::lock_guard lock(mutex);
        const auto now=Now();
        const auto seconds=now>since?static_cast<double>(now-since)/Frequency():0.0;
        char line[384]{};
        std::snprintf(line,sizeof(line),"%s version=%s seconds=%.3f window=since-previous-report-or-load peaks=since-load nested-times=do-not-add",
            prefix,version,seconds);
        emit(line);
        for(std::size_t i=0;i<Size;++i) {
            const auto sample=counters[i].Read();
            const auto calls=sample.calls-previous[i].calls;
            const auto ticks=sample.ticks-previous[i].ticks;
            const auto units=sample.units-previous[i].units;
            const auto ms=static_cast<double>(ticks)*1000.0/Frequency();
            std::snprintf(line,sizeof(line),
                "%s metric=%s calls=%llu callsPerSec=%.2f total_ms=%.3f avg_ms=%.6f peakSinceLoad_ms=%.3f %s=%llu elapsed_msPerSec=%.3f",
                prefix,names[i].name,static_cast<unsigned long long>(calls),seconds>0?static_cast<double>(calls)/seconds:0,
                ms,calls?ms/static_cast<double>(calls):0,static_cast<double>(sample.peak)*1000.0/Frequency(),
                names[i].units,static_cast<unsigned long long>(units),seconds>0?ms/seconds:0);
            emit(line);
            previous[i]=sample;
        }
        since=now;
    }
};
}
