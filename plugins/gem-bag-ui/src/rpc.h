#pragma once
#include "native.h"
#include "protocol.h"
#include <deque>
#include <map>
#include <mutex>
#include <set>

namespace GemBag::Rpc {
struct Ticket {uint64_t generation{},presentation{},serial{};};
struct Result {
    Ticket ticket{};
    Protocol::Reply reply{};
};
using ResultCallback=void(*)(const Result&);
class Service {
public:
    explicit Service(uint64_t(*now)()=nullptr):clock(now){}
    bool Start(const D2RL::PluginContext*,const D2RL::NetworkService*,const D2RL::ThreadService*,
        const D2RL::InventoryService*,const D2RL::ItemService*,ResultCallback);
    void Stop();
    void ResetClient();
    bool Begin(const Info&,unsigned,bool,Ticket,bool deposit=false);
    void Tick();
    bool Pending();
    void EnableScopedPump(bool);
    void PumpScopedGame(); // Only from the qualified Core per-game frame scope.
private:
    struct Cached {Protocol::Request request{};Protocol::Reply reply{};bool done{};};
    struct Peer {
        uint64_t epoch{},clientSession{},serverSession{},highest{},lastSeen{},lease{},definition{};
        D2RL::PlayerHandle owner{};
        Info bag{};
        bool pending{};
        std::deque<Cached> recent;
    };
    struct Job {D2RL::Network::PeerHandle peer{};uint64_t epoch{};Protocol::Request request{};uint64_t queuedAt{};bool scoped{};};
    struct Intent {Info bag{};unsigned gem{};bool withdraw{};Ticket ticket{};bool deposit{};};
    enum class Phase {Idle,Connect,Hello,Action,Uncertain};
    const D2RL::PluginContext* context{};
    const D2RL::NetworkService* network{};
    const D2RL::ThreadService* threads{};
    const D2RL::InventoryService* inventory{};
    const D2RL::ItemService* items{};
    D2RL::Network::ChannelHandle channel{};
    ResultCallback callback{};
    uint64_t(*clock)(){};
    uint64_t Now() const;
    std::mutex guard,execution;
    bool stopping=true,uncertain{},haltAll{},scopedPump{};
    uint64_t epoch{},clientSession{},serverSession{},sequence{},lease{},started{},nextConnect{};
    Phase phase=Phase::Idle;
    Intent intent{};
    Protocol::Request pending{};
    std::map<D2RL::Network::PeerHandle,Peer> peers;
    std::deque<Job> jobs;
    std::deque<Protocol::Reply> replies;
    std::set<D2RL::PlayerHandle> haltedOwners;
    static void __cdecl Host(const D2RL::PluginContext*,D2RL::Network::ChannelHandle,D2RL::Network::PeerHandle,uint16_t,const void*,uint32_t,void*) noexcept;
    static void __cdecl Client(const D2RL::PluginContext*,D2RL::Network::ChannelHandle,uint16_t,const void*,uint32_t,void*) noexcept;
    static void __cdecl Connection(const D2RL::PluginContext*,const D2RL::Network::ConnectionEvent*,void*) noexcept;
    static void __cdecl Game(const D2RL::PluginContext*,void*) noexcept;
    static void __cdecl Ui(const D2RL::PluginContext*,void*) noexcept;
    void Receive(D2RL::Network::PeerHandle,const Protocol::Request&);
    void Execute(bool scoped=false);
    void Deliver(Protocol::Reply);
    void Fail(const char*,bool);
    void Send(const Job&,const Protocol::Reply&);
};
}
