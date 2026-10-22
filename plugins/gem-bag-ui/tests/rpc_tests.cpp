// Real protocol and transport, with owned SDK queues/peers/storage and native
// backend substitutes. No sockets, game functions, native publication or saves.
#include "rpc.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace Test {
using namespace D2RL;
using namespace GemBag;
using namespace Protocol;
void Check(bool value,const char* what){if(!value){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
uint64_t now=1000;
uint64_t Now(){return now;}
struct Node;
std::vector<Node*> nodes;
Node* gameThread{};
Node* uiThread{};
struct Task {Node* node{};bool game{};Threads::Callback callback{};void* data{};};
std::deque<Task> tasks;
struct Store {Info bag{};unsigned credits=3,outputs{};bool space=true,cursor{},uncertain{};uint64_t definition=1234;};
std::map<PlayerHandle,Store> stores;
unsigned inspectCalls{},executeCalls{},lastGem{};
struct Node {
    uint64_t id{},peer{};PlayerHandle owner{};
    PlayerHandle frameOwner{};
    PluginContext context{};
    Rpc::Service rpc{Now};
    Node* server{};
    Network::ChannelRegistration registration{};
    Network::ChannelHandle channel{};
    bool connected=true,dropReplies{},peerAvailable=true,registered{},gameQueueAvailable=true;
    Request lastRequest{};Reply lastReply{};
    std::vector<Rpc::Result> results;
    uint64_t ticket{};
    explicit Node(uint64_t identity):id(identity),peer(identity+100),owner(identity*11) {
        context.contextSize=sizeof(context);context.abiVersion=D2RL_PLUGIN_ABI_VERSION;context.reserved[0]=reinterpret_cast<uintptr_t>(this);
        nodes.push_back(this);
    }
};
Node& Of(const PluginContext* context){return *reinterpret_cast<Node*>(context->reserved[0]);}
void Result(const Rpc::Result& result) {
    Check(!gameThread,"client result is never applied on an authoritative callback");
    for(auto node:nodes)if(node->id==result.ticket.generation){node->results.push_back(result);return;}
    Check(false,"result keeps its presentation ticket");
}
void Receive(Node& client,const Request& request,uint32_t size=sizeof(Request)) {
    auto& host=*client.server;
    host.registration.hostMessage(&host.context,host.channel,client.peer,RequestMessage,&request,size,host.registration.userData);
}
NetworkService network{.serviceSize=NetworkServiceSize,.serviceVersion=NetworkService::AbiVersion,
    .registerChannel=[](const PluginContext* context,const Network::ChannelRegistration* registration,uint64_t* handle) noexcept {
        auto& node=Of(context);Check(registration->compatibilityToken==Compatibility&&registration->localChannelId==1,"owned compatible channel");
        node.registration=*registration;node.registered=true;*handle=node.channel=node.id+1000;return Network::Result::Success;
    },
    .unregisterChannel=[](const PluginContext* context,uint64_t) noexcept {Of(context).registered=false;return Network::Result::Success;},
    .getChannelInfo=[](const PluginContext* context,uint64_t,Network::ChannelInfo* info) noexcept {
        info->connectionState=Of(context).connected?Network::ConnectionState::Connected:Network::ConnectionState::Disconnected;return Network::Result::Success;
    },
    .connectToHost=[](const PluginContext* context,uint64_t) noexcept {Of(context).connected=true;return Network::Result::Success;},
    .sendToHost=[](const PluginContext* context,uint64_t,uint16_t message,const void* data,uint32_t size) noexcept {
        auto& node=Of(context);Check(message==RequestMessage&&Copy(data,size,node.lastRequest),"client sends a bounded valid value payload");
        if(!node.server->registered)return Network::Result::NotConnected;
        Receive(node,node.lastRequest);return Network::Result::Success;
    },
    .sendToClient=[](const PluginContext* context,uint64_t,uint64_t peer,uint16_t message,const void* data,uint32_t size) noexcept {
        auto& host=Of(context);Check(message==ReplyMessage,"server replies on its owned channel");
        for(auto node:nodes)if(node->server==&host&&node->peer==peer) {
            Check(Copy(data,size,node->lastReply),"server emits valid fixed-size replies");
            if(!node->dropReplies)node->registration.clientMessage(&node->context,node->channel,message,data,size,node->registration.userData);
            return Network::Result::Success;
        }
        return Network::Result::NotConnected;
    },
    .getPeerPlayer=[](const PluginContext* context,uint64_t,uint64_t peer,PlayerHandle* player) noexcept {
        auto& host=Of(context);Check(gameThread==&host&&!uiThread,"peer is resolved only on the scheduled host game thread");
        for(auto node:nodes)if(node->server==&host&&node->peer==peer&&node->peerAvailable&&(!host.frameOwner||host.frameOwner==node->owner)){*player=node->owner;return Network::Result::Success;}
        return Network::Result::NotFound;
    }};
ThreadService threads{.serviceSize=ThreadServiceSize,.serviceVersion=ThreadService::AbiVersion,
    .runOnUiThread=[](const PluginContext* context,Threads::Callback callback,void* data) noexcept {tasks.push_back({&Of(context),false,callback,data});return Threads::Result::Success;},
    .runOnGameThread=[](const PluginContext* context,Threads::Callback callback,void* data) noexcept {
        auto& node=Of(context);Check(node.server==&node,"joining client never queues its own mutation callback");
        if(!node.gameQueueAvailable)return Threads::Result::Unavailable;
        tasks.push_back({&node,true,callback,data});return Threads::Result::Success;
    }};
InventoryService inventory{.serviceSize=InventoryServiceSize,.serviceVersion=InventoryService::AbiVersion,
    .getLocalPlayer=[](const PluginContext*,PlayerHandle*) noexcept {Check(false,"host must never use its own local player for a peer");return Inventory::Result::Unavailable;},
    .getCursorItem=[](const PluginContext* context,PlayerHandle owner,ItemHandle* item) noexcept {
        Check(gameThread==&Of(context)&&stores.contains(owner),"server checks the sending player's cursor");*item=stores[owner].cursor?99:0;return Inventory::Result::Success;
    },
    .forEachInventoryItem=[](const PluginContext* context,PlayerHandle owner,const Inventory::ItemFilter* filter,Inventory::ItemCallback callback,void* data) noexcept {
        Check(gameThread==&Of(context)&&stores.contains(owner),"server enumerates only the resolved peer's storage");
        const auto& bag=stores[owner].bag;
        if(filter->containerMask&Items::ContainerBit(bag.container))callback(context,&bag,data);
        return Inventory::Result::Success;
    }};
ItemService items{.serviceSize=ItemServiceSize,.serviceVersion=ItemService::AbiVersion};
void Drain() {
    unsigned count{};
    while(!tasks.empty()) {
        Check(++count<10000,"bounded deferred callback drain");const auto task=tasks.front();tasks.pop_front();
        gameThread=task.game?task.node:nullptr;uiThread=task.game?nullptr:task.node;
        task.callback(&task.node->context,task.data);gameThread=uiThread=nullptr;
    }
}
void Frame(Node& host,PlayerHandle owner) {
    Check(!gameThread&&!uiThread,"native frame starts outside another fake callback");
    gameThread=&host;host.frameOwner=owner;host.rpc.PumpScopedGame();host.frameOwner=0;gameThread=nullptr;Drain();
}
Info Bag(PlayerHandle owner,Container container=Container::Inventory) {
    Info bag{.structSize=sizeof(Info)};bag.handle=owner+1000;bag.runtimeId=static_cast<uint32_t>(owner+100);bag.classId=746;bag.code=BagCode;
    bag.container=container;bag.inventoryPage=container==Container::Inventory?0:container==Container::Cube?3:4;
    bag.sharedStashPage=container==Container::SharedStash?2:UINT32_MAX;bag.x=1;bag.y=1;bag.quality=Items::Quality::Normal;
    bag.generationSeed=static_cast<uint32_t>(owner+200);bag.itemSeed=static_cast<uint32_t>(owner+300);return bag;
}
void Inspect(Node& node,const Info& bag) {
    Check(node.rpc.Begin(bag,0,false,{node.id,7,++node.ticket}),"inspection intent accepted");Drain();
}
void Withdraw(Node& node,unsigned gem=0) {
    Check(node.rpc.Begin(stores[node.owner].bag,gem,true,{node.id,7,++node.ticket}),"withdrawal intent accepted");Drain();
}
Store& Owned(const Info& bag) {
    Check(gameThread!=nullptr&&!uiThread,"native backend called only on authoritative queue");
    for(auto& [owner,store]:stores)if(store.bag.handle==bag.handle)return store;
    Check(false,"native target is owned");return stores.begin()->second;
}
}
namespace GemBag::Native {
bool Deposit(const D2RL::PluginContext* c,const D2RL::ItemService* items,const D2RL::InventoryService*,D2RL::PlayerHandle owner,const Info& bag,Snapshot& out,std::string& why,bool& halted) {
    auto& store=Test::Owned(bag);Test::Check(Test::stores[owner].bag.handle==bag.handle,"deposit uses sending peer");
    if(store.uncertain){halted=true;return false;}
    store.credits+=25;return Inspect(c,items,bag.handle,bag,out,why);
}
bool Inspect(const D2RL::PluginContext*,const D2RL::ItemService*,D2RL::ItemHandle,const Info& bag,Snapshot& out,std::string& why) {
    auto& store=Test::Owned(bag);++Test::inspectCalls;
    out={bag,store.credits,store.space?std::optional<Cell>{{0,0}}:std::nullopt,3,10,8,store.definition,255};why="Authoritative bag snapshot.";return true;
}
bool Execute(const D2RL::PluginContext* c,const D2RL::ItemService* items,D2RL::PlayerHandle owner,const Info& bag,unsigned gem,Snapshot& out,std::string& why,bool& halted) {
    auto& store=Test::Owned(bag);Test::Check(Test::stores.contains(owner)&&Test::stores[owner].bag.handle==bag.handle,"native creation belongs to the requesting peer");
    ++Test::executeCalls;Test::lastGem=gem;
    if(store.uncertain){halted=true;why="Uncertain native outcome.";return false;}
    const bool ok=store.credits&&store.space;
    if(ok){--store.credits;++store.outputs;}
    Inspect(c,items,bag.handle,bag,out,why);return ok;
}
}
int main() {
    using namespace Test;
    Node host(90),a(1),b(2),loop(3),dedicated(4);host.server=&host;a.server=b.server=dedicated.server=&host;loop.server=&loop;
    stores[a.owner].bag=Bag(a.owner);stores[b.owner].bag=Bag(b.owner);stores[loop.owner].bag=Bag(loop.owner);
    Check(host.rpc.Start(&host.context,&network,&threads,&inventory,&items,nullptr),"headless server channel starts without client state");
    for(auto node:{&a,&b,&loop,&dedicated})Check(node->rpc.Start(&node->context,&network,&threads,&inventory,&items,Result),"client or offline loopback channel starts");
    host.gameQueueAvailable=false;
    Inspect(a,stores[a.owner].bag);
    Check(a.results.back().reply.status==Status::Unavailable&&!a.rpc.Pending()&&!inspectCalls&&!executeCalls&&stores[a.owner].credits==3&&!stores[a.owner].outputs,
        "an unavailable host scheduler returns a copied refusal before native reads or mutations");
    Check(!a.rpc.Begin(stores[a.owner].bag,5,true,{a.id,7,++a.ticket}),"a refused host inspection grants no withdrawal lease or client fallback");
    host.gameQueueAvailable=true;
    for(auto container:{Container::Inventory,Container::Cube,Container::PersonalStash,Container::SharedStash})for(unsigned gem=0;gem<8;++gem) {
        auto& store=stores[a.owner];store=Store{};store.bag=Bag(a.owner,container);
        auto ui=store.bag;ui.handle=9999;ui.runtimeId=9999;ui.generationSeed=777; // No client handle/seed is used as server authority.
        Inspect(a,ui);Check((a.results.back().reply.flags&HasSnapshot)&&a.results.back().reply.lease,"host issues a bag lease from its own identity");
        Withdraw(a,gem);Check(store.credits==2&&store.outputs==1&&lastGem==gem&&(a.results.back().reply.flags&Success),"all gems/stores use sending player's server preflight and one debit/output");
    }
    Inspect(loop,stores[loop.owner].bag);Withdraw(loop,5);Check(stores[loop.owner].credits==2&&stores[loop.owner].outputs==1,"offline loopback uses identical host messaging path");
    Inspect(loop,stores[loop.owner].bag);++stores[loop.owner].definition;const auto beforeDefinition=executeCalls;Withdraw(loop,5);
    Check(executeCalls==beforeDefinition&&stores[loop.owner].credits==2&&stores[loop.owner].outputs==1&&!(loop.results.back().reply.flags&Success),"changed definition invalidates the host lease before debit or creation");
    stores[dedicated.owner].bag=Bag(dedicated.owner);host.gameQueueAvailable=false;host.rpc.EnableScopedPump(true);
    const auto priorInspect=inspectCalls,priorExecute=executeCalls;
    Inspect(dedicated,stores[dedicated.owner].bag);Check(dedicated.rpc.Pending()&&dedicated.results.empty(),"qualified frame path defers unavailable SDK scheduling without client mutation");
    Frame(host,a.owner);Check(dedicated.rpc.Pending()&&inspectCalls==priorInspect,"another hosted game's frame cannot inspect the sending player's bag");
    Frame(host,dedicated.owner);Frame(host,dedicated.owner);
    Check(!dedicated.rpc.Pending()&&(dedicated.results.back().reply.flags&HasSnapshot),"matching host frames complete Hello and inspection with a lease");
    Check(dedicated.rpc.Begin(stores[dedicated.owner].bag,5,true,{dedicated.id,7,++dedicated.ticket}),"native-frame withdrawal intent accepted");
    const auto nativeRequest=dedicated.lastRequest;
    Frame(host,a.owner);Check(executeCalls==priorExecute&&stores[dedicated.owner].credits==3,"wrong game frame cannot debit or create");
    Frame(host,dedicated.owner);Check(stores[dedicated.owner].credits==2&&stores[dedicated.owner].outputs==1&&!dedicated.rpc.Pending(),"matching native scope performs one server debit and gem creation");
    Receive(dedicated,nativeRequest);Drain();Check(stores[dedicated.owner].outputs==1,"native-frame replay returns cache without another mutation");
    dedicated.peerAvailable=false;
    Check(dedicated.rpc.Begin(stores[dedicated.owner].bag,0,true,{dedicated.id,7,++dedicated.ticket}),"missing native-frame peer queues intent");
    Frame(host,dedicated.owner);Check(dedicated.rpc.Pending(),"unresolved peer waits for its own hosted frame");
    now+=7100;Frame(host,dedicated.owner);
    Check(!dedicated.rpc.Pending()&&dedicated.results.back().reply.status==Status::Unavailable&&stores[dedicated.owner].outputs==1,"unresolved native-frame peer expires with no debit or automatic retry");
    dedicated.peerAvailable=true;host.rpc.EnableScopedPump(false);host.gameQueueAvailable=true;
    stores[a.owner]=Store{};stores[a.owner].bag=Bag(a.owner);stores[b.owner].bag=Bag(b.owner);stores[b.owner].bag.x=6;
    const auto ownCredits=stores[a.owner].credits,otherCredits=stores[b.owner].credits;
    Inspect(a,stores[b.owner].bag);Check(!(a.results.back().reply.flags&Success)&&stores[a.owner].credits==ownCredits&&stores[b.owner].credits==otherCredits,"foreign storage location cannot select another peer's bag");
    Inspect(a,stores[a.owner].bag);++stores[a.owner].bag.runtimeId;++stores[a.owner].bag.itemSeed;
    Withdraw(a);Check(stores[a.owner].credits==ownCredits&&!stores[a.owner].outputs,"same-cell replacement invalidates the authoritative bag lease");
    Inspect(a,stores[a.owner].bag);stores[a.owner].space=false;Withdraw(a);Check(stores[a.owner].credits==ownCredits&&!stores[a.owner].outputs,"server space change is checked again before debit");
    stores[a.owner].space=true;Inspect(a,stores[a.owner].bag);stores[a.owner].credits=0;Withdraw(a);Check(!stores[a.owner].outputs&&!stores[a.owner].credits,"server zero credit refuses output");
    stores[a.owner].credits=3;Inspect(a,stores[a.owner].bag);stores[a.owner].cursor=true;Withdraw(a);Check(stores[a.owner].credits==3&&!stores[a.owner].outputs,"occupied server cursor refuses output");stores[a.owner].cursor=false;
    Inspect(a,stores[a.owner].bag);a.dropReplies=true;
    Check(a.rpc.Begin(stores[a.owner].bag,5,true,{a.id,7,++a.ticket}),"duplicate test queues one mutation");
    const auto request=a.lastRequest;const auto queued=tasks.size();Receive(a,request);Receive(a,request);Check(tasks.size()==queued,"in-flight duplicates queue no extra mutation");Drain();
    Check(stores[a.owner].credits==2&&stores[a.owner].outputs==1&&a.rpc.Pending(),"server completes once even when its reply is withheld");
    a.dropReplies=false;Receive(a,request);Drain();Check(stores[a.owner].credits==2&&stores[a.owner].outputs==1&&!a.rpc.Pending(),"completed duplicate returns cached reply without another debit");
    auto altered=request;altered.gem=6;const auto calls=executeCalls;Receive(a,altered);Drain();Check(executeCalls==calls,"same sequence with changed payload never executes");
    auto malformed=request;malformed.magic^=1;Receive(a,malformed);Receive(a,request,sizeof(Request)-1);Drain();Check(executeCalls==calls,"wrong protocol and malformed length are ignored");
    const auto stale=request;a.rpc.ResetClient();Inspect(a,stores[a.owner].bag);Receive(a,stale);Drain();Check(executeCalls==calls,"previous client/server session payload is refused");
    Inspect(b,stores[b.owner].bag);Withdraw(b,7);Check(stores[b.owner].credits==otherCredits-1&&stores[a.owner].credits==2,"simultaneous peers remain isolated from host-local player and each other");
    Inspect(a,stores[a.owner].bag);a.dropReplies=true;
    Check(a.rpc.Begin(stores[a.owner].bag,0,true,{a.id,7,++a.ticket}),"timeout test queues mutation");Drain();
    now+=9000;a.rpc.Tick();Check((a.results.back().reply.flags&Halted)&&!a.rpc.Begin(stores[a.owner].bag,0,true,{a.id,7,++a.ticket}),"unacknowledged mutation latches client and is never automatically retried");
    const auto timeoutOutputs=stores[a.owner].outputs;a.dropReplies=false;Receive(a,a.lastRequest);Drain();Check(stores[a.owner].outputs==timeoutOutputs,"late duplicate cannot mint another gem");
    a.rpc.ResetClient();a.peerAvailable=false;Inspect(a,stores[a.owner].bag);Check(!(a.results.back().reply.flags&Success),"missing authoritative peer cannot inspect or withdraw");a.peerAvailable=true;
    a.rpc.ResetClient();Check(a.rpc.Begin(stores[a.owner].bag,0,false,{a.id,7,++a.ticket}),"old session queues its handshake");
    tasks.clear(); // The SDK explicitly discards queued callbacks on session change.
    a.peer+=1000;a.rpc.ResetClient();const auto priorResults=a.results.size();
    Inspect(a,stores[a.owner].bag);
    Check(a.results.size()>priorResults&&(a.results.back().reply.flags&HasSnapshot)&&!a.rpc.Pending(),"a new session drains orphaned value jobs after the SDK drops old callbacks");
    b.rpc.ResetClient();Inspect(b,stores[b.owner].bag);stores[b.owner].uncertain=true;Withdraw(b);
    Check(b.results.back().reply.flags&Halted,"unknown native result halts this server owner");
    b.rpc.ResetClient();b.peer+=1000;stores[b.owner].uncertain=false;Inspect(b,stores[b.owner].bag);Check(b.results.back().reply.flags&Halted,"server owner halt survives reconnect with a new peer handle");
    a.rpc.ResetClient();Inspect(a,stores[a.owner].bag);
    Check(a.rpc.Begin(stores[a.owner].bag,1,true,{a.id,7,++a.ticket}),"disconnect test queues mutation");
    const auto beforeDisconnect=stores[a.owner].outputs;
    Network::ConnectionEvent disconnected{.structSize=Network::ConnectionEventSize,.channel=host.channel,.peer=a.peer,.state=Network::ConnectionState::Disconnected};
    host.registration.connectionState(&host.context,&disconnected,host.registration.userData);Drain();Check(stores[a.owner].outputs==beforeDisconnect,"disconnected queued peer is invalidated before item work");
    for(auto client:{&a,&loop}) {
        client->rpc.ResetClient();auto& store=stores[client->owner];store.credits=0;store.space=false;
        Inspect(*client,store.bag);const auto before=store.credits;
        Check(client->rpc.Begin(store.bag,0,true,{client->id,7,++client->ticket},true),"deposit intent accepted with fresh lease");
        const auto deposit=client->lastRequest;Receive(*client,deposit);Drain();
        Check(deposit.action==Action::Deposit&&store.credits==before+25,"hosted and offline deposits run once without a free cell or prior credits");
        Receive(*client,deposit);Drain();Check(store.credits==before+25,"completed deposit replay returns cached result");
        Inspect(*client,store.bag);store.cursor=true;
        Check(client->rpc.Begin(store.bag,0,true,{client->id,7,++client->ticket},true),"cursor change tested at host");Drain();
        Check(store.credits==before+25&&!(client->results.back().reply.flags&Success),"occupied cursor prevents deposit at host");store.cursor=false;
        Inspect(*client,store.bag);++store.bag.itemSeed;
        Check(client->rpc.Begin(store.bag,0,true,{client->id,7,++client->ticket},true),"replacement tested at host");Drain();
        Check(store.credits==before+25&&client->results.back().reply.status==Status::Stale,"bag replacement refuses deposit lease");
    }
    a.rpc.ResetClient();Inspect(a,stores[a.owner].bag);Check(a.rpc.Begin(stores[a.owner].bag,1,true,{a.id,7,++a.ticket}),"unload test queues mutation");
    host.rpc.Stop();Drain();Check(stores[a.owner].outputs==beforeDisconnect,"stopped host discards queued mutation");
    a.rpc.Stop();b.rpc.Stop();loop.rpc.Stop();dedicated.rpc.Stop();
    std::puts("Real RPC: separate client/host and offline loopback, 32 gem/storage cases, authenticated peer ownership, authoritative leases/space/credits, duplicate/stale/malformed refusal, timeout/disconnect/unload and owner halt passed; owned SDK/native substitutes only.");
}
