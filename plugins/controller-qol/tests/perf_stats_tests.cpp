#include "perf_stats.h"
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

void Check(bool valid,const char* why) {
    if(!valid){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}
}
int main() {
    std::array<LootPerf::Counter,1> counters{};
    std::vector<std::thread> workers;
    for(unsigned i=0;i<4;++i)workers.emplace_back([&] {
        for(unsigned n=0;n<1000;++n)counters[0].Record(7,3);
    });
    for(auto& worker:workers)worker.join();
    const auto sample=counters[0].Read();
    Check(sample.calls==4000 && sample.units==12000 && sample.ticks==28000 && sample.peak==7,
        "concurrent producers retain completed work and maximum");
    LootPerf::ReportWindow<1> window;
    const std::array<LootPerf::Description,1> names{{{"fixture","copiedItems"}}};
    std::vector<std::string> lines;
    auto emit=[&](const char* line) {lines.emplace_back(line);};
    window.Print("FIXTURE_PERF","test",names,counters,emit);
    Check(lines.size()==2 && lines[1].find("calls=4000 ")!=std::string::npos &&
        lines[1].find("copiedItems=12000 ")!=std::string::npos,"first report includes accumulated work");
    Check(counters[0].Read().calls==4000,"reporting leaves producer counters intact");
    counters[0].Record(100,2);lines.clear();
    window.Print("FIXTURE_PERF","test",names,counters,emit);
    Check(lines.size()==2 && lines[1].find("calls=1 ")!=std::string::npos &&
        lines[1].find("copiedItems=2 ")!=std::string::npos && counters[0].Read().peak==100,
        "later report isolates the interval and retains the lifetime peak");
    lines.clear();window.Print("FIXTURE_PERF","test",names,counters,emit);
    Check(lines[1].find("calls=0 ")!=std::string::npos && lines[1].find("avg_ms=0.000000 ")!=std::string::npos,
        "quiet interval reports zero work without stale averages");
    LootPerf::Counter timed;
    const auto before=LootPerf::Now();
    constexpr unsigned iterations=100000;
    for(unsigned n=0;n<iterations;++n){LootPerf::Scope scope(timed);scope.Items(2);}
    const auto after=LootPerf::Now();
    Check(timed.Read().calls==iterations && timed.Read().units==2*iterations,
        "timed scopes record every completion and item count");
    std::printf("Performance aggregation and report checks passed; empty scope wall cost %.3f us/call (isolated fixture).\n",
        static_cast<double>(after-before)*1000000.0/LootPerf::Frequency()/iterations);
}
