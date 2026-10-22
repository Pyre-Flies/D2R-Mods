#include "rpc.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cstdio>

namespace GemBag::Rpc {
namespace {
using namespace D2RL;
using namespace Protocol;
uint64_t Token() {
    uint64_t value{};
    return BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&value),sizeof(value),BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0?value:0;
}
Reply Refusal(const Request& request,Status status,const char* text) {
    Reply reply{};reply.request=request;reply.status=status;
    std::snprintf(reply.message,sizeof(reply.message),"%s",text);return reply;
}
bool CursorEmpty(const PluginContext* context,const InventoryService* inventory,PlayerHandle owner) {
    ItemHandle held{};const auto result=inventory->getCursorItem(context,owner,&held);
    return (result==Inventory::Result::Success&&!held)||result==Inventory::Result::NotFound;
}
bool Resolve(const PluginContext* context,const InventoryService* inventory,PlayerHandle owner,const Target& target,Info& bag) {
    struct Match {Target target{};Info bag{};unsigned count{};} match{target};
    const Inventory::ItemFilter filter{.structSize=Inventory::ItemFilterSize,.containerMask=Items::ContainerBit(static_cast<Container>(target.container))};
    const auto result=inventory->forEachInventoryItem(context,owner,&filter,[](const PluginContext*,const Info* item,void* data) noexcept {
        auto& found=*static_cast<Match*>(data);
        if(item&&Supported(*item)&&Identify(*item)==found.target){found.bag=*item;++found.count;}
        return Inventory::IterationAction::Continue;
    },&match);
    if(result!=Inventory::Result::Success||match.count!=1)return false;
    bag=match.bag;return true;
}
}
uint64_t Service::Now() const {return clock?clock():GetTickCount64();}
bool Service::Start(const PluginContext* c,const NetworkService* n,const ThreadService* t,const InventoryService* inv,const ItemService* it,ResultCallback result) {
    if(!c||!HasNetworkServiceField(n,NetworkServiceRequiredSize)||!n->registerChannel||!n->unregisterChannel||!n->getChannelInfo||
        !n->connectToHost||!n->sendToHost||!n->sendToClient||!n->getPeerPlayer||!t||!t->runOnGameThread||!t->runOnUiThread||
        !inv||!inv->getCursorItem||!inv->forEachInventoryItem)return false;
    context=c;network=n;threads=t;inventory=inv;items=it;callback=result;
    {std::lock_guard lock(guard);stopping=false;}
    const Network::ChannelRegistration registration{.structSize=Network::ChannelRegistrationSize,.localChannelId=1,.compatibilityToken=Compatibility,
        .hostMessage=Host,.clientMessage=Client,.connectionState=Connection,.userData=this};
    if(network->registerChannel(context,&registration,&channel)!=Network::Result::Success||!channel){Stop();return false;}
    if(callback)ResetClient();
    return true;
}
void Service::Stop() {
    Network::ChannelHandle old{};
    {std::lock_guard lock(guard);stopping=true;old=channel;channel=0;++epoch;phase=Phase::Idle;jobs.clear();replies.clear();}
    if(old&&network)network->unregisterChannel(context,old);
    std::lock_guard running(execution);
    std::lock_guard lock(guard);peers.clear();haltedOwners.clear();
}
void Service::ResetClient() {
    const auto nonce=Token();
    std::lock_guard lock(guard);clientSession=nonce;serverSession=sequence=lease=0;
    phase=Phase::Idle;pending={};intent={};uncertain=false;replies.clear();nextConnect=0;
}
bool Service::Pending() {std::lock_guard lock(guard);return phase!=Phase::Idle||uncertain;}
void Service::EnableScopedPump(bool available) {std::lock_guard lock(guard);scopedPump=available;}
void Service::PumpScopedGame() {
    size_t count{};
    {std::lock_guard lock(guard);if(stopping||!scopedPump)return;count=static_cast<size_t>(std::count_if(jobs.begin(),jobs.end(),[](const Job& job){return job.scoped;}));}
    // A peer in another hosted game stays queued for its own frame. Do not
    // repeatedly take the same deferred job during this game's update.
    for(size_t index=0;index<count;++index)Execute(true);
}
bool Service::Begin(const Info& bag,unsigned gem,bool withdraw,Ticket ticket,bool deposit) {
    {
        std::lock_guard lock(guard);
        if(stopping||!channel||!callback||!clientSession||phase!=Phase::Idle||uncertain||!Valid(Identify(bag))||gem>=Gems.size()||(withdraw&&!lease))return false;
        if(deposit&&(!withdraw||gem))return false;
        intent={bag,gem,withdraw,ticket,deposit};phase=Phase::Connect;started=Now();pending={};
    }
    Tick();return true;
}
void Service::Fail(const char* text,bool unknown) {
    Result result{};ResultCallback notify{};
    {
        std::lock_guard lock(guard);if(stopping||phase==Phase::Idle)return;
        result.ticket=intent.ticket;result.reply=Refusal(pending,unknown?Status::Halted:Status::Unavailable,text);
        if(unknown){uncertain=true;result.reply.flags=Halted;phase=Phase::Uncertain;}
        else {phase=Phase::Idle;serverSession=lease=0;}
        notify=callback;
    }
    if(notify)notify(result);
}
void Service::Tick() {
    for(;;) {
        Reply reply{};
        {std::lock_guard lock(guard);if(stopping)return;if(replies.empty())break;reply=replies.front();replies.pop_front();}
        Deliver(reply);
    }
    Phase current{};uint64_t age{};
    {std::lock_guard lock(guard);if(stopping||phase==Phase::Idle||phase==Phase::Uncertain)return;current=phase;age=Now()-started;}
    if(age>8000) {bool mutation{};{std::lock_guard lock(guard);mutation=phase==Phase::Action&&intent.withdraw;}
        Fail(mutation?"Server reply delayed; withdrawal outcome is uncertain. Do not retry in this session.":"Gem Bag host did not respond. Reopen the picker to reconnect.",mutation);return;}
    Network::ChannelInfo info{.structSize=Network::ChannelInfoSize};
    if(network->getChannelInfo(context,channel,&info)!=Network::Result::Success)return;
    if(info.connectionState!=Network::ConnectionState::Connected) {
        if(current!=Phase::Connect){bool mutation{};{std::lock_guard lock(guard);mutation=current==Phase::Action&&intent.withdraw;}
            Fail(mutation?"Disconnected during withdrawal; check the bag after rejoining.":"Gem Bag host connection was lost.",mutation);return;}
        bool connect{};{std::lock_guard lock(guard);const auto now=Now();if(now>=nextConnect){nextConnect=now+1000;connect=true;}}
        if(connect) {
            const auto result=network->connectToHost(context,channel);
            if(result==Network::Result::Incompatible||result==Network::Result::Unsupported||result==Network::Result::WrongRole)
                Fail("This host does not provide a compatible Gem Bag service.",false);
        }
        return;
    }
    if(current!=Phase::Connect)return;
    Request request{};
    {
        std::lock_guard lock(guard);if(stopping||phase!=Phase::Connect)return;
        request.clientSession=clientSession;request.sequence=++sequence;
        if(!serverSession){request.action=Action::Hello;phase=Phase::Hello;}
        else {
            request.serverSession=serverSession;request.target=Identify(intent.bag);request.action=intent.deposit?Action::Deposit:intent.withdraw?Action::Withdraw:Action::Inspect;
            request.lease=intent.withdraw?lease:0;request.gem=intent.withdraw?static_cast<uint16_t>(intent.gem):0;phase=Phase::Action;
        }
        pending=request;
    }
    const auto sent=network->sendToHost(context,channel,RequestMessage,&request,sizeof(request));
    if(sent!=Network::Result::Success)Fail("The Gem Bag request could not be sent.",sent==Network::Result::CallbackFault&&(request.action==Action::Withdraw||request.action==Action::Deposit));
}
void Service::Deliver(Reply reply) {
    Result result{};ResultCallback notify{};bool hello{};
    {
        std::lock_guard lock(guard);
        if(stopping||phase==Phase::Idle||!Valid(reply)||reply.request!=pending)return;
        if(phase==Phase::Uncertain)return; // No new mutation until a new client session.
        if(reply.request.action==Action::Hello&&(reply.flags&Success)&&reply.serverSession) {
            serverSession=reply.serverSession;phase=Phase::Connect;started=Now();hello=true;
        } else {
            if(reply.request.action!=Action::Hello&&reply.serverSession!=serverSession)return;
            result={intent.ticket,reply};notify=callback;
            if(reply.status==Status::Stale||reply.status==Status::Unavailable)serverSession=0;
            lease=(reply.flags&HasSnapshot)?reply.lease:0;
            uncertain|=(reply.flags&Halted)!=0;phase=Phase::Idle;
        }
    }
    if(notify)notify(result);
    if(hello)Tick();
}
void __cdecl Service::Ui(const PluginContext*,void* data) noexcept {
    try {static_cast<Service*>(data)->Tick();}catch(...){ }
}
void __cdecl Service::Client(const PluginContext*,Network::ChannelHandle handle,uint16_t message,const void* data,uint32_t size,void* opaque) noexcept {
    try {
        auto& self=*static_cast<Service*>(opaque);Reply reply{};
        if(message!=ReplyMessage||!Copy(data,size,reply))return;
        {std::lock_guard lock(self.guard);if(self.stopping||handle!=self.channel||!self.callback||self.replies.size()>=16)return;self.replies.push_back(reply);}
        self.threads->runOnUiThread(self.context,Ui,&self);
    }catch(...){ }
}
void __cdecl Service::Connection(const PluginContext*,const Network::ConnectionEvent* event,void* opaque) noexcept {
    try {
        auto& self=*static_cast<Service*>(opaque);
        if(!Network::HasConnectionEventField(event,Network::ConnectionEventRequiredSize))return;
        bool client{};
        {
            std::lock_guard lock(self.guard);if(self.stopping||event->channel!=self.channel)return;
            if(event->state==Network::ConnectionState::Disconnected&&event->peer) {
                self.peers.erase(event->peer);
                std::erase_if(self.jobs,[&](const Job& job){return job.peer==event->peer;});
            }
            client=self.callback!=nullptr;
        }
        if(client)self.threads->runOnUiThread(self.context,Ui,&self);
    }catch(...){ }
}
void __cdecl Service::Host(const PluginContext*,Network::ChannelHandle handle,Network::PeerHandle peer,uint16_t message,const void* data,uint32_t size,void* opaque) noexcept {
    try {
        auto& self=*static_cast<Service*>(opaque);Request request{};
        if(!peer||message!=RequestMessage||!Copy(data,size,request))return;
        {std::lock_guard lock(self.guard);if(self.stopping||handle!=self.channel)return;}
        self.Receive(peer,request);
    }catch(...){ }
}
void Service::Receive(Network::PeerHandle handle,const Request& request) {
    Reply reply=Refusal(request,Status::Stale,"Stale Gem Bag session or request; reopen the picker.");
    bool respond{},queue{},scheduleFailed{};Job job{};
    {
        std::lock_guard lock(guard);if(stopping)return;
        auto found=peers.find(handle);
        if(found==peers.end()&&request.action==Action::Hello) {
            const auto now=Now();
            std::erase_if(peers,[&](const auto& entry){return !entry.second.pending&&now-entry.second.lastSeen>60000;});
            if(peers.size()<64) {Peer peer{};peer.epoch=++epoch;peer.clientSession=request.clientSession;found=peers.emplace(handle,peer).first;}
        }
        if(found==peers.end())respond=true;
        else {
            auto& peer=found->second;peer.lastSeen=Now();reply.serverSession=peer.serverSession;
            if(request.action==Action::Hello&&request.clientSession!=peer.clientSession&&!peer.pending) {
                peer=Peer{};peer.epoch=++epoch;peer.clientSession=request.clientSession;peer.lastSeen=Now();
            }
            auto cached=std::find_if(peer.recent.begin(),peer.recent.end(),[&](const Cached& entry){return entry.request.sequence==request.sequence;});
            if(peer.clientSession!=request.clientSession||(request.action!=Action::Hello&&peer.serverSession!=request.serverSession))respond=true;
            else if(cached!=peer.recent.end()) {
                if(cached->request!=request)respond=true;
                else if(cached->done){reply=cached->reply;respond=true;}
                // An exact in-flight duplicate is deliberately not scheduled.
            } else if(request.sequence<=peer.highest)respond=true;
            else {
                peer.highest=request.sequence;
                if(peer.pending||jobs.size()>=64) {reply=Refusal(request,Status::Busy,"A Gem Bag request is already running.");reply.serverSession=peer.serverSession;respond=true;}
                else {
                    while(peer.recent.size()>=64&&peer.recent.front().done)peer.recent.pop_front();
                    peer.recent.push_back({request,{},false});peer.pending=true;job={handle,peer.epoch,request,Now(),false};jobs.push_back(job);queue=true;
                }
            }
        }
        // The service only queues callbacks. Keep routing and publication under
        // this lock so an older game callback cannot take a half-routed job.
        if(queue) {
        const auto scheduled=threads->runOnGameThread(context,Game,this);
        if(scheduled!=Threads::Result::Success) {
            if(scheduled==Threads::Result::Unavailable&&scopedPump)jobs.back().scoped=true;
            else {
                jobs.pop_back();scheduleFailed=true;
                char reason[96];std::snprintf(reason,sizeof(reason),"Host game queue refused (%u); no qualified frame bridge is available.",static_cast<unsigned>(scheduled));
                reply=Refusal(request,Status::Unavailable,reason);reply.serverSession=peers.at(job.peer).serverSession;
            }
        }
        }
    }
    if(respond)network->sendToClient(context,channel,handle,ReplyMessage,&reply,sizeof(reply));
    if(scheduleFailed)Send(job,reply);
}
void __cdecl Service::Game(const PluginContext*,void* data) noexcept {
    try {
        auto& self=*static_cast<Service*>(data);
        // Session changes can discard SDK callbacks while copied value jobs
        // remain. A later callback drains a bounded batch, so expired peers
        // cannot strand the new session's handshake behind orphaned jobs.
        for(unsigned count=0;count<64;++count) {
            {std::lock_guard lock(self.guard);if(self.stopping||self.jobs.empty())return;}
            self.Execute();
        }
    }catch(...){ }
}
void Service::Execute(bool scoped) {
    Job job{};Reply reply{};PlayerHandle owner{};bool attempted{};
    {
        std::lock_guard running(execution);
        Peer state{};
        {std::lock_guard lock(guard);if(stopping||jobs.empty())return;
            const auto queued=std::find_if(jobs.begin(),jobs.end(),[&](const Job& entry){return entry.scoped==scoped;});
            if(queued==jobs.end())return;job=*queued;jobs.erase(queued);
            const auto found=peers.find(job.peer);if(found==peers.end()||found->second.epoch!=job.epoch)return;state=found->second;}
        reply=Refusal(job.request,Status::Refused,"The bag, cursor or destination changed.");reply.serverSession=state.serverSession;
        try {
            const auto& request=job.request;
            const auto resolved=network->getPeerPlayer(context,channel,job.peer,&owner);
            if(scoped&&resolved==Network::Result::NotFound&&Now()-job.queuedAt<7000) {
                std::lock_guard lock(guard);const auto found=peers.find(job.peer);
                if(!stopping&&found!=peers.end()&&found->second.epoch==job.epoch)jobs.push_back(job);
                return;
            }
            if(resolved!=Network::Result::Success||!owner) {
                reply.status=Status::Unavailable;std::snprintf(reply.message,sizeof(reply.message),"The host could not resolve the sending player.");
            } else if(request.action==Action::Hello) {
                const auto nonce=Token();
                if(nonce) {
                    std::lock_guard lock(guard);const auto found=peers.find(job.peer);
                    if(found==peers.end()||found->second.epoch!=job.epoch||stopping)return;
                    auto& peer=found->second;peer.owner=owner;peer.serverSession=nonce;peer.lease=0;peer.bag={};
                    reply.serverSession=nonce;reply.flags=Success;reply.status=Status::Ok;std::snprintf(reply.message,sizeof(reply.message),"Gem Bag host ready.");
                }
            } else if(state.owner!=owner||!state.serverSession||state.serverSession!=request.serverSession)reply.status=Status::Stale;
            else {
                Info bag{};const auto nextLease=Token();
                if(!nextLease)reply.status=Status::Unavailable;
                else if(!CursorEmpty(context,inventory,owner))std::snprintf(reply.message,sizeof(reply.message),"Put down the item on your cursor first.");
                else if(!Resolve(context,inventory,owner,request.target,bag))reply.status=Status::Stale;
                else if(request.action!=Action::Inspect&&(!state.lease||state.lease!=request.lease||!Same(bag,state.bag)))reply.status=Status::Stale;
                else {
                    bool blocked{};{std::lock_guard lock(guard);blocked=haltAll||haltedOwners.contains(owner);}
                    Native::Snapshot snapshot{};std::string why;bool nativeHalted{},ok{};
                    if(blocked) {nativeHalted=true;why="Host withdrawals stopped after an uncertain result.";}
                    else if(request.action==Action::Inspect)ok=Native::Inspect(context,items,bag.handle,bag,snapshot,why);
                    else if(!Native::Inspect(context,items,bag.handle,bag,snapshot,why)||snapshot.definition!=state.definition) {
                        why="Bag definitions changed; reopen the picker before withdrawing.";
                    } else {attempted=true;ok=request.action==Action::Deposit?
                        Native::Deposit(context,items,inventory,owner,bag,snapshot,why,nativeHalted):
                        Native::Execute(context,items,owner,bag,request.gem,snapshot,why,nativeHalted);}
                    reply.status=nativeHalted?Status::Halted:ok?Status::Ok:Status::Refused;
                    reply.flags=(ok?Success:0)|(nativeHalted?Halted:0);std::snprintf(reply.message,sizeof(reply.message),"%s",why.c_str());
                    {
                        std::lock_guard lock(guard);const auto found=peers.find(job.peer);
                        if(nativeHalted){if(haltedOwners.size()<1024)haltedOwners.insert(owner);else haltAll=true;}
                        if(found==peers.end()||found->second.epoch!=job.epoch||stopping)return;
                        auto& peer=found->second;peer.lease=0;peer.definition=0;peer.bag={};
                        if(snapshot.bag.handle&&Identify(snapshot.bag)==request.target) {
                            peer.bag=snapshot.bag;peer.lease=nextLease;peer.definition=snapshot.definition;reply.lease=nextLease;reply.flags|=HasSnapshot;
                            reply.definition=snapshot.definition;reply.bank=snapshot.bank;reply.gemMask=snapshot.gemMask;
                            reply.credits=snapshot.credits;reply.width=snapshot.width;reply.height=snapshot.height;
                            if(snapshot.cell){reply.flags|=HasCell;reply.cellX=snapshot.cell->x;reply.cellY=snapshot.cell->y;}
                        }
                    }
                    char line[256];std::snprintf(line,sizeof(line),"[GemBag/Host] action=%u peer=%llu sequence=%llu gem=%u success=%u credits=%u: %s",static_cast<unsigned>(request.action),job.peer,request.sequence,request.gem,ok?1u:0u,reply.credits,reply.message);context->LogInfo(line);
                }
            }
        } catch(...) {
            reply=Refusal(job.request,attempted?Status::Halted:Status::Refused,"Host Gem Bag action failed; no automatic retry.");reply.serverSession=state.serverSession;
            if(attempted){reply.flags=Halted;std::lock_guard lock(guard);if(owner&&haltedOwners.size()<1024)haltedOwners.insert(owner);else haltAll=true;}
        }
    }
    Send(job,reply);
}
void Service::Send(const Job& job,const Reply& reply) {
    {
        std::lock_guard lock(guard);if(stopping)return;
        const auto found=peers.find(job.peer);if(found==peers.end()||found->second.epoch!=job.epoch)return;
        auto& peer=found->second;
        const auto cached=std::find_if(peer.recent.begin(),peer.recent.end(),[&](const Cached& entry){return entry.request==job.request;});
        if(cached==peer.recent.end())return;
        cached->reply=reply;cached->done=true;peer.pending=false;
    }
    network->sendToClient(context,channel,job.peer,ReplyMessage,&reply,sizeof(reply));
}
}
