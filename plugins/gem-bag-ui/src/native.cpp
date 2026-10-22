#include "native.h"
#include "core_compatibility.h"
#include "native_input_profile.h"
#include "native_identify_profile.h"
#include "filter_focus_signatures.h"
#include "storage_focus_profile.h"
#include "materials_signatures.h"
#include "profile.h"
#include "server_frame.h"
#include "admission.h"
#include "catalog.h"
#include "deposit.h"
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#include <vector>
#include <bit>

namespace GemBag::Native {
namespace {
uintptr_t core{};
bool admitted{};
using SetList=bool(__fastcall*)(uint8_t,void*,int32_t,int32_t,uint32_t);
SetList setList{};
#ifdef GEMBAG_NATIVE_TESTS
Testing::Publisher testPublisher{};
#endif
constexpr unsigned char HostHash[]={0x93,0x02,0x1d,0xad,0x48,0x53,0x3b,0xcf,0xca,0x8a,0x95,0xc6,0x9a,0x0c,0xc0,0x0c,0xdb,0x90,0x08,0xa3,0xce,0x6c,0xeb,0xa1,0xd0,0x4c,0x7a,0xca,0x71,0x98,0x5a,0x10};
constexpr unsigned char SetterBytes[]={0x56,0x57,0x55,0x53,0x48,0x83,0xec,0x48,0x44,0x89,0xc6};
template<class T> bool Read(uintptr_t address,T& value) noexcept {
    SIZE_T got{};return address>=0x10000 && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),&value,sizeof(value),&got) && got==sizeof(value);
}
bool Bytes(uintptr_t address,const unsigned char* value,size_t size) noexcept {
    unsigned char out[2048]{};SIZE_T got{};return size<=sizeof(out) && ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),out,size,&got) && got==size && !std::memcmp(value,out,size);
}
bool SetterOwned() {return Bytes(core+0x831ed0,Profile::SetList,sizeof(Profile::SetList))&&Bytes(core+0x3d95c0,Profile::SetListInner,sizeof(Profile::SetListInner));}
bool PublisherOwned(const D2RL::PluginContext* c) {
    uintptr_t target{},rva{};
    return Read(core+0x6ffda8,target)&&Read(core+0x6ffdb0,rva)&&rva==0x535f60&&target==c->exeBase+rva &&
        Bytes(core+0x429600,Profile::PublishMove,sizeof(Profile::PublishMove))&&
        Bytes(core+0x424fd0,Profile::PublishEdit,sizeof(Profile::PublishEdit))&&
        c->CheckExpectedBytes(rva,Profile::NativePublish,sizeof(Profile::NativePublish));
}
struct Record {uint32_t layer,id;int32_t value;uint32_t padding;};
bool Stat(uintptr_t list,unsigned offset,unsigned creditStat,int& value,uint64_t* fingerprint=nullptr) {
    uintptr_t pointer{};uint64_t count{};value=0;
    if(!Read(list+offset,pointer)||!Read(list+offset+8,count)||count>511)return false;
    uint64_t previous{},other=14695981039346656037ull;bool first=true;
    for(size_t i=0;i<count;++i) {
        Record r{};if(!Read(pointer+i*sizeof(r),r)||r.id>=512||r.layer>65535)return false;
        const uint64_t key=(uint64_t{r.id}<<32)|r.layer;
        if(!first && key<=previous)return false;first=false;previous=key;
        if(r.id==creditStat && !r.layer)value=r.value;
        else for(auto b:std::bit_cast<std::array<unsigned char,16>>(r)){other^=b;other*=1099511628211ull;}
    }
    if(fingerprint)*fingerprint=other;
    return true;
}
struct Detail {Snapshot snapshot;uintptr_t contribution{};int value{};unsigned sourceIndex{UINT32_MAX};uint64_t fingerprint{};std::shared_ptr<const Catalog::Definition> definition;};
bool Capture(void* native,const Info& expected,Detail& out,unsigned hint=UINT32_MAX,uint64_t fingerprint=0) {
    const auto unit=reinterpret_cast<uintptr_t>(native);
    std::array<uint32_t,3> id{};uintptr_t data{},inventory{},grids{},root{};uint32_t magic{},flags{},gridCount{};uint8_t page{},bank{};
    if(!Read(unit,id)||id[0]!=4||id[1]!=expected.classId||id[2]!=expected.runtimeId ||
       !Read(unit+0x10,data)||!Read(data+0xa0,inventory)||!Read(inventory,magic)||magic!=0x01020304 ||
       !Read(data+0x55,page)||page!=expected.inventoryPage || !Read(unit+0x1bd,bank)||bank<1||bank>3 ||
       !Read(inventory+0x20,grids)||!Read(inventory+0x28,gridCount)||gridCount>32 ||
       !Read(unit+0x88,root)||!Read(root+0x1c,flags)||(flags&0x80000000u)==0)return false;
    uintptr_t owner{};if(!Read(root+0xa0,owner)||owner!=unit)return false;
    out.definition=Catalog::Find(bank,expected);if(!out.definition)return false;
    const auto creditStat=out.definition->creditStat;
    int total{},base{};
    if(!Stat(root,0xa8,creditStat,total)||!Stat(root,0x30,creditStat,base)||base || total<0||total>65535)return false;
    uintptr_t child{};if(!Read(root+0x90,child))return false;
    std::array<uintptr_t,64> seen{};size_t count{};
    while(child) {
        if(count==seen.size())return false;
        for(size_t i=0;i<count;++i)if(seen[i]==child)return false;
        seen[count++]=child;uintptr_t parent{};int value{};uint32_t cf{},state{};
        uint64_t other{};
        if(!Read(child+0x78,parent)||parent!=root||!Read(child+0x1c,cf)||!Read(child+0x20,state)||!Stat(child,0x30,creditStat,value,&other))return false;
        uint64_t entries{};if(!Read(child+0x38,entries))return false;
        const bool emptySource=!total && cf==0x40 && !state &&
            (hint==UINT32_MAX?!entries:(count-1==hint&&other==fingerprint));
        if(value || emptySource) {
            uintptr_t childOwner{};int32_t expiry{};
            if(out.contribution||cf!=0x40||state||!Read(child,childOwner)||childOwner!=unit||!Read(child+0x18,expiry)||expiry!=-1)return false;
            out.contribution=child;out.value=value;out.sourceIndex=static_cast<unsigned>(count-1);out.fingerprint=other;
        }
        if(!Read(child+0x68,child))return false;
    }
    if(total!=out.value || (total && !out.contribution))return false;
    if(hint!=UINT32_MAX && (out.sourceIndex!=hint || out.fingerprint!=fingerprint))return false;
    const unsigned index=static_cast<unsigned>(page)+2;
    if(index>=gridCount)return false;
    const auto grid=grids+index*32;
    uint8_t width{},height{};uintptr_t cells{};
    if(!Read(grid+0x10,width)||!Read(grid+0x11,height)||!Read(grid+0x18,cells)||!width||!height||width>16||height>16 ||
        expected.x<0||expected.y<0||expected.x>=width||expected.y>=height)return false;
    std::array<uintptr_t,256> occupancy{};SIZE_T got{};
    const size_t size=static_cast<size_t>(width)*height*sizeof(uintptr_t);
    if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(cells),occupancy.data(),size,&got)||got!=size ||
       occupancy[static_cast<size_t>(expected.y)*width+expected.x]!=unit)return false;
    out.snapshot={expected,static_cast<unsigned>(total),FreeCell(width,height,occupancy),bank,width,height,out.definition->fingerprint,out.definition->mask};
    return true;
}
struct CaptureJob {const Info* expected;Detail result;bool ok{};};
void __cdecl CaptureCallback(const D2RL::PluginContext*,void* item,void* opaque) noexcept {
    auto& job=*static_cast<CaptureJob*>(opaque);try{job.ok=Capture(item,*job.expected,job.result);job.result.contribution=0;}catch(...){job.ok=false;}
}
bool DetailFor(const D2RL::PluginContext* c,const D2RL::ItemService* items,D2RL::ItemHandle handle,const Info& expected,Detail& out) {
    Info current{.structSize=sizeof(Info)};
    if(!admitted || !SetterOwned() || items->getItemInfo(c,handle,&current)!=D2RL::Items::Result::Success || !Same(current,expected))return false;
    CaptureJob job{&current};
    if(items->editNativeItem(c,handle,CaptureCallback,&job)!=D2RL::Items::Result::Success || !job.ok)return false;
    if(!job.result.definition||!Catalog::Recheck(c,*job.result.definition))return false;
    out=job.result;return true;
}
// Same full stored-item publication used by Core's normal edit and shared
// move paths. The actual inventory owner is essential for Shared Stash.
bool SendPublication(const D2RL::PluginContext* c,uintptr_t game,uintptr_t owner,void* item,const Info& bag) noexcept {
    using Publish=void(__fastcall*)(void*,void*,void*,uint32_t,uint32_t,uint32_t,uint8_t,uint8_t,uint32_t);
    __try {
        const auto position=(static_cast<uint32_t>(bag.y)<<16)|static_cast<uint32_t>(bag.x);
        reinterpret_cast<Publish>(c->exeBase+0x535f60)(reinterpret_cast<void*>(game),reinterpret_cast<void*>(owner),item,1,2,0,static_cast<uint8_t>(bag.inventoryPage),0,position);
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct PublicationJob {const Info* expected;bool send{},ok{};};
void __cdecl PublicationCallback(const D2RL::PluginContext* c,void* item,void* opaque) noexcept {
    auto& job=*static_cast<PublicationJob*>(opaque);
    try {
        Detail before{};
        if(!PublisherOwned(c)||!Capture(item,*job.expected,before))return;
        const auto unit=reinterpret_cast<uintptr_t>(item);
        uintptr_t data{},inventory{},owner{},game{},activeGame{},ownerGame{},control{},ownerInventory{};uint32_t ownerType{},refs{};
        const auto scopedGame=ServerFrame::CurrentGame(c);
        if(!Read(unit+0x10,data)||!Read(data+0xa0,inventory)||!Read(inventory+8,owner)||!Read(owner,ownerType)||ownerType ||
            !Read(unit+0xd8,game)||!game||!Read(c->exeBase+0x2a9e020,activeGame)||
            game!=(scopedGame?scopedGame:activeGame) ||
            !Read(owner+0xd8,ownerGame)||ownerGame!=game||!Read(owner+0xa0,control))return;
        if(control&&!Read(control+8,refs))return;
        if(!Read(owner+(control&&refs?0x98:0x90),ownerInventory)||ownerInventory!=inventory)return;
        if(!job.send){job.ok=true;return;}
        if(!SendPublication(c,game,owner,item,*job.expected))return;
        Detail after{};
        job.ok=Capture(item,*job.expected,after)&&after.snapshot.credits==before.snapshot.credits;
    }catch(...){job.ok=false;}
}
bool Publication(const D2RL::PluginContext* c,const D2RL::ItemService* items,const Info& bag,bool send) {
#ifdef GEMBAG_NATIVE_TESTS
    if(testPublisher)return testPublisher(c,items,bag,send);
#endif
    PublicationJob job{&bag,send};
    return items->editNativeItem(c,bag.handle,PublicationCallback,&job)==D2RL::Items::Result::Success&&job.ok;
}
struct WriteJob {const Info* expected;int before{},after{};unsigned source{};uint64_t fingerprint{},definition{};bool ok{},uncertain{};};
bool CallSetter(uint8_t bank,uintptr_t list,unsigned creditStat,int value) noexcept {
    __try {return setList(bank,reinterpret_cast<void*>(list),static_cast<int32_t>(creditStat),value,0);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __cdecl WriteCallback(const D2RL::PluginContext*,void* item,void* opaque) noexcept {
    auto& job=*static_cast<WriteJob*>(opaque);
    try {
        Detail before{};if(!Capture(item,*job.expected,before,job.source,job.fingerprint)||before.snapshot.definition!=job.definition||before.value!=job.before||!before.contribution||
            before.sourceIndex!=job.source||before.fingerprint!=job.fingerprint)return;
        job.uncertain=true;
        CallSetter(static_cast<uint8_t>(before.snapshot.bank),before.contribution,before.definition->creditStat,job.after);
        Detail after{};
        job.ok=Capture(item,*job.expected,after,job.source,job.fingerprint) && after.value==job.after;
        job.uncertain=!job.ok;
    }catch(...){job.uncertain=true;}
}
bool Change(const D2RL::PluginContext* c,const D2RL::ItemService* items,const Info& bag,const Catalog::Definition& definition,int before,int after,unsigned source,uint64_t fingerprint,bool& uncertain) {
    if(!SetterOwned()||!Catalog::Recheck(c,definition))return false;
    WriteJob job{&bag,before,after,source,fingerprint,definition.fingerprint};
    const auto r=items->editNativeItem(c,bag.handle,WriteCallback,&job);uncertain=job.uncertain;
    return r==D2RL::Items::Result::Success && job.ok;
}
}
bool Initialize(const D2RL::PluginContext* c) {
    admitted=false;if(!c)return false;
    Admission::Report report(c);
    const auto module=GetModuleHandleW(L"D2RCore.dll");core=reinterpret_cast<uintptr_t>(module);
    report.Module(reinterpret_cast<HMODULE>(c->exeBase),"D2RLoader.exe",HostHash);
    report.Module(module,"D2RCore.dll",QolNativeProfile::CoreHash);
    if(report.Failures())return false;
    const auto entry=GetProcAddress(module,"SetWideListStat");
    if(!entry)return report.Fail("export=SetWideListStat unavailable; win32=%lu",GetLastError());
    if(reinterpret_cast<uintptr_t>(entry)!=core+0x831ed0)return report.Fail("export=SetWideListStat unexpected target; expected_rva=0x831ED0 actual_offset=0x%llX",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(entry)-core));
    report.Bytes(core,0x831ed0,SetterBytes,"D2RCore/SetWideListStat");
    constexpr unsigned char selector[]={0x8b,0x56,0x1c,0x85,0xd2,0xb8,0xa8,0,0,0,0x41,0xb9,0x30,0,0,0,0x4c,0x0f,0x48,0xc8};
    report.Bytes(core,0x3d8c32,selector,"D2RCore/stat-list-selector");
    report.Bytes(c->exeBase,0xce500,QolMaterials::Signatures::UiMode,"executable/UI-mode",true);
    report.Bytes(c->exeBase,0x14f1e0,QolNativeIdentifyProfile::Cursor,"executable/cursor-item",true);
    report.Bytes(c->exeBase,0x14f210,QolNativeIdentifyProfile::Mode,"executable/input-mode",true);
    if(report.Failures())return false;
    c->LogInfo("[GemBag/Admission] Native engine/provider verified; compatible bag definitions will be discovered from loaded tables.");
    setList=reinterpret_cast<SetList>(entry);admitted=true;return true;
}
bool Inspect(const D2RL::PluginContext* c,const D2RL::ItemService* items,D2RL::ItemHandle handle,const Info& expected,Snapshot& out,std::string& why) {
    Detail d{};
    if(!DetailFor(c,items,handle,expected,d)){why="The bag could not be safely read in this game.";return false;}
    out=d.snapshot;
    why=!out.credits?"The Gem Bag is empty.":!out.cell?"No free space beside the Gem Bag.":"Choose a gem. Each withdrawal costs one credit.";
    return true;
}
bool Execute(const D2RL::PluginContext* c,const D2RL::ItemService* items,D2RL::PlayerHandle player,const Info& expected,unsigned gem,Snapshot& result,std::string& why,bool& halted) {
    if(gem>=Gems.size())return false;
    struct Backend {
        const D2RL::PluginContext* c;const D2RL::ItemService* items;D2RL::PlayerHandle player;const Info& bag;unsigned gem;
        Snapshot& result;std::string& why;bool& halted;Detail detail{};D2RL::ItemHandle output{};bool uncertainCreation{};
        bool Preflight() {
            if(!Supported(bag)||bag.quantity>1||!DetailFor(c,items,bag.handle,bag,detail)||!detail.snapshot.credits||!detail.snapshot.cell) {
                why="The bag is empty, moved, or its storage has no free space.";return false;
            }
            if(bag.container==Container::SharedStash && (!(items->capabilities&D2RL::Items::ItemServiceCapabilityBit(D2RL::Items::ItemServiceCapability::SharedStashWrite)))) {
                why="This loader cannot write this Shared Stash page.";return false;
            }
            if(!(detail.definition->mask&(1u<<gem))){why="This bag has no compatible recipe for that gem.";return false;}
            if(!Publication(c,items,bag,false)){why="The bag cannot be refreshed safely in this storage.";return false;}
            return true;
        }
        bool Debit() {
            bool uncertain{};const bool ok=Change(c,items,bag,*detail.definition,static_cast<int>(detail.snapshot.credits),static_cast<int>(detail.snapshot.credits)-1,detail.sourceIndex,detail.fingerprint,uncertain);
            if(!ok && uncertain)Halt();return ok;
        }
        bool Create() {
            const auto cell=*detail.snapshot.cell;
            const D2RL::Items::ItemCreateSpec spec{.structSize=D2RL::Items::ItemCreateSpecSize,.code=detail.definition->choices[gem].code,.quality=D2RL::Items::Quality::Normal,
                .qualityRecordId=D2RL::Items::RandomQualityRecord,.itemLevel=bag.itemLevel?bag.itemLevel:1,
                .quantity=D2RL::Items::DefaultValue,.durability=D2RL::Items::DefaultValue,.stateFlags=D2RL::Items::ItemStateIdentified,
                .destination={.structSize=D2RL::Items::ItemDestinationSize,.container=bag.container,.placement=D2RL::Items::Placement::Exact,
                    .x=cell.x,.y=cell.y,.sharedStashPage=bag.sharedStashPage}};
            const auto created=items->createItem(c,player,&spec,&output);
            if(created!=D2RL::Items::Result::Success||!output) {
                uncertainCreation=output||created==D2RL::Items::Result::CallbackFault||created==D2RL::Items::Result::Success;
                if(uncertainCreation)Halt();else why="The gem could not be placed. Your credit was restored.";
                return false;
            }
            return true;
        }
        bool Verify() {
            Info out{.structSize=sizeof(Info)};Detail now{};const auto cell=*detail.snapshot.cell;
            return items->getItemInfo(c,output,&out)==D2RL::Items::Result::Success && out.code==detail.definition->choices[gem].code && out.classId==detail.definition->choices[gem].classId &&
                out.quality==D2RL::Items::Quality::Normal && out.quantity<=1 && (out.stateFlags&D2RL::Items::ItemStateIdentified) &&
                out.container==bag.container && (bag.container!=Container::SharedStash||out.sharedStashPage==bag.sharedStashPage) &&
                out.x==static_cast<int>(cell.x)&&out.y==static_cast<int>(cell.y) && DetailFor(c,items,bag.handle,bag,now) && now.snapshot.credits+1==detail.snapshot.credits;
        }
        bool Publish() {return Publication(c,items,bag,true);}
        bool Remove() {return items->destroyItem(c,player,output,D2RL::Items::SocketedItemPolicy::RejectIfNotEmpty)==D2RL::Items::Result::Success;}
        bool Restore() {
            if(uncertainCreation)return false;
            bool uncertain{};const bool ok=Change(c,items,bag,*detail.definition,static_cast<int>(detail.snapshot.credits)-1,static_cast<int>(detail.snapshot.credits),detail.sourceIndex,detail.fingerprint,uncertain);
            return ok && Publish();
        }
        void Halt() {halted=true;why="Withdrawal stopped after an uncertain native result. Check the plugin log before continuing.";c->LogError("[GemBag] Compensation or native mutation could not be confirmed; withdrawals latched off for this session.");}
    } backend{c,items,player,expected,gem,result,why,halted};
    const bool ok=Withdraw(backend);
    std::string status;
    Inspect(c,items,expected.handle,expected,result,status);
    if(ok)why=std::string("Withdrew ")+Gems[gem].name+" into "+Destination(expected)+".";
    else if(why.empty())why="Withdrawal refused; the bag or destination changed.";
    return ok;
}
bool Deposit(const D2RL::PluginContext* c,const D2RL::ItemService* items,const D2RL::InventoryService* inventory,D2RL::PlayerHandle player,
    const Info& bag,Snapshot& result,std::string& why,bool& halted) {
    Detail start{};
    if(!Supported(bag)||bag.quantity>1||!DetailFor(c,items,bag.handle,bag,start)||start.sourceIndex==UINT32_MAX||start.definition->deposits.empty()||
        !Publication(c,items,bag,false)) {why="Compatible deposits or the bag credit source are unavailable.";return false;}
    if(bag.container==Container::SharedStash&&!D2RL::HasItemServiceCapability(items,D2RL::Items::ItemServiceCapability::SharedStashWrite)) {
        why="This loader cannot write this Shared Stash page.";return false;
    }
    struct Scan {const Info& bag;std::vector<Info> items;bool refused{};} scan{bag};
    scan.items.reserve(256);
    const D2RL::Inventory::ItemFilter filter{.structSize=D2RL::Inventory::ItemFilterSize,.containerMask=D2RL::Items::ContainerBit(bag.container)};
    const auto scanned=inventory->forEachInventoryItem(c,player,&filter,[](const D2RL::PluginContext*,const Info* item,void* data) noexcept {
        auto& s=*static_cast<Scan*>(data);
        if(item&&DepositPage(s.bag,*item)) {
            if(s.items.size()==256||std::any_of(s.items.begin(),s.items.end(),[&](const Info& i){return i.runtimeId==item->runtimeId;})) {
                s.refused=true;return D2RL::Inventory::IterationAction::Stop;
            }
            s.items.push_back(*item); // Capacity reserved before entering the noexcept callback.
        }
        return D2RL::Inventory::IterationAction::Continue;
    },&scan);
    if(scanned!=D2RL::Inventory::Result::Success||scan.refused){why="Storage enumeration could not be verified.";return false;}
    std::sort(scan.items.begin(),scan.items.end(),[](const Info& a,const Info& b){return a.y!=b.y?a.y<b.y:a.x<b.x;});
    unsigned consumed{},credits{},skipped{};bool failed{};
    for(const auto& input:scan.items) {
        const auto rule=std::find_if(start.definition->deposits.begin(),start.definition->deposits.end(),[&](const auto& r){return r.classId==input.classId&&r.code==input.code;});
        if(rule==start.definition->deposits.end())continue;
        Detail before{};Info current{.structSize=sizeof(Info)};
        if(!DetailFor(c,items,bag.handle,bag,before)||before.snapshot.definition!=start.snapshot.definition||
            items->getItemInfo(c,input.handle,&current)!=D2RL::Items::Result::Success||!Same(current,input)||current.quantity!=input.quantity||!DepositPage(bag,current)) {
            failed=true;why="An item changed; deposit processing stopped.";break;
        }
        const auto quantity=static_cast<unsigned>(input.quantity);
        // Check the worst case before drawing. Never reroll a cluster to fit.
        if(uint64_t{rule->maximum}*quantity>65535-before.snapshot.credits||quantity>4096-consumed){++skipped;continue;}
        unsigned amount{};
        for(unsigned n=0;n<quantity;++n) {
            const auto range=rule->maximum-rule->minimum+1;
            uint32_t random{};
            if(range>1) {
                const uint64_t limit=(uint64_t{1}<<32)/range*range;
                unsigned tries{};
                do {
                    if(++tries>32||BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&random),sizeof(random),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0) {
                        why="Server random source unavailable; deposits stopped.";failed=true;break;
                    }
                }while(random>=limit);
                if(failed)break;
            }
            amount+=rule->minimum+random%range;
        }
        if(failed)break;
        struct Backend {
            const D2RL::PluginContext* c;const D2RL::ItemService* items;D2RL::PlayerHandle player;
            const Info& bag;const Info& input;const Detail& before;unsigned amount;bool& halted;
            D2RL::Items::Result consumed=D2RL::Items::Result::Unavailable;
            const char* stage="credit";bool restored{},staged{};Info stagedInfo{};
            D2RL::Items::Result moved=D2RL::Items::Result::Unavailable;
            bool Identity(const Info& now,const Info& expected) {
                return now.handle==expected.handle&&Same(now,expected)&&now.quantity==expected.quantity&&
                    now.quality==expected.quality&&now.stateFlags==expected.stateFlags&&now.socketCount==expected.socketCount&&
                    now.socketedItemCount==expected.socketedItemCount;
            }
            bool MoveInput(bool back) {
                using namespace D2RL::Items;
                if(!D2RL::HasItemServiceField(items,static_cast<uint32_t>(offsetof(D2RL::ItemService,executeExistingItemTransaction)+sizeof(items->executeExistingItemTransaction)))||!items->executeExistingItemTransaction)return false;
                Info current{.structSize=sizeof(Info)};
                if(items->getItemInfo(c,input.handle,&current)!=Result::Success||!Identity(current,back?stagedInfo:input)){Halt();return false;}
                ExistingItemOperation op{};op.structSize=ExistingItemOperationSize;op.kind=ExistingItemOperationKind::Move;op.item=input.handle;
                op.move.destination.structSize=ItemDestinationSize;op.move.destination.container=back?Container::SharedStash:Container::Inventory;
                op.move.destination.placement=back?Placement::Exact:Placement::Automatic;
                op.move.destination.sharedStashPage=back?input.sharedStashPage:UINT32_MAX;
                if(back){op.move.destination.x=input.x;op.move.destination.y=input.y;}
                const ExistingItemTransaction txn{.structSize=ExistingItemTransactionSize,.player=player,.operationCount=1,.operations=&op};
                ExistingItemTransactionResult result{.structSize=ExistingItemTransactionResultSize};
                moved=items->executeExistingItemTransaction(c,&txn,&result);
                Info after{.structSize=sizeof(Info)};
                const bool readable=items->getItemInfo(c,input.handle,&after)==Result::Success;
                if(moved!=Result::Success) {
                    if(moved==Result::CallbackFault||result.committedOperationCount||!readable||!Identity(after,current))Halt();
                    return false;
                }
                Info expected=input;
                if(!back){expected.container=Container::Inventory;expected.inventoryPage=0;expected.sharedStashPage=after.sharedStashPage;expected.x=after.x;expected.y=after.y;}
                if(result.flags||result.committedOperationCount!=1||!readable||!Identity(after,expected)||after.x<0||after.y<0){Halt();return false;}
                staged=!back;if(staged)stagedInfo=after;
                return true;
            }
            bool Credit() {
                bool uncertain{};
                const bool ok=Change(c,items,bag,*before.definition,before.value,before.value+static_cast<int>(amount),before.sourceIndex,before.fingerprint,uncertain);
                if(uncertain)Halt();return ok;
            }
            bool Consume(){
                if(input.container==Container::SharedStash){stage="stage-to-inventory";if(!MoveInput(false))return false;}
                stage="consume";consumed=items->destroyItem(c,player,input.handle,D2RL::Items::SocketedItemPolicy::RejectIfNotEmpty);return consumed==D2RL::Items::Result::Success;
            }
            bool Unchanged() {
                if(halted||consumed==D2RL::Items::Result::CallbackFault)return false;
                if(staged){stage="restore-shared-item";if(!MoveInput(true))return false;}
                Info now{.structSize=sizeof(Info)};
                return items->getItemInfo(c,input.handle,&now)==D2RL::Items::Result::Success&&Same(now,input)&&now.quantity==input.quantity&&DepositPage(bag,now);
            }
            bool Restore() {
                bool uncertain{};
                restored=Change(c,items,bag,*before.definition,before.value+static_cast<int>(amount),before.value,before.sourceIndex,before.fingerprint,uncertain)&&Publication(c,items,bag,true);
                return restored;
            }
            bool Verify() {
                stage="verify-consumption";Info now{.structSize=sizeof(Info)};const auto status=items->getItemInfo(c,input.handle,&now);Detail after{};
                return (status==D2RL::Items::Result::NotFound||status==D2RL::Items::Result::StaleHandle)&&
                    DetailFor(c,items,bag.handle,bag,after)&&after.value==before.value+static_cast<int>(amount);
            }
            bool Publish(){stage="publish";return Publication(c,items,bag,true);}
            void Halt(){halted=true;c->LogError("[GemBag/Deposit] Uncertain mutation/publication; all bag mutations stopped for this owner.");}
        } backend{c,items,player,bag,input,before,amount,halted};
        if(!DepositOne(backend)){
            failed=true;
            char line[384];std::snprintf(line,sizeof(line),"[GemBag/Deposit] Refused stage=%s sdkDestroyResult=%u sdkMoveResult=%u item=%u bag=%u container=%u inventoryPage=%u sharedPage=%u quantity=%u creditsBefore=%u attemptedCredit=%u restored=%u halted=%u.",
                backend.stage,static_cast<unsigned>(backend.consumed),static_cast<unsigned>(backend.moved),input.runtimeId,bag.runtimeId,static_cast<unsigned>(input.container),static_cast<unsigned>(input.inventoryPage),input.sharedStashPage,quantity,before.snapshot.credits,amount,backend.restored?1u:0u,halted?1u:0u);
            c->LogWarn(line);
            why=halted?"Deposit outcome uncertain; further bag actions stopped.":backend.restored?(std::string_view(backend.stage)=="stage-to-inventory"?"Shared deposit needs free inventory space and a writable stash page; credits restored.":"Gem removal refused; this item's credits were restored."):"Bag credit update refused; no gem was consumed.";
            break;
        }
        consumed+=quantity;credits+=amount;
        char line[160];std::snprintf(line,sizeof(line),"[GemBag/Deposit] item=%u class=%u quantity=%u recipe=%u credits=%u->%u.",input.runtimeId,input.classId,quantity,rule->recipe,before.snapshot.credits,before.snapshot.credits+amount);c->LogInfo(line);
    }
    std::string ignored;Inspect(c,items,bag.handle,bag,result,ignored);
    if(!failed) {
        why="Deposited "+std::to_string(consumed)+" gems/clusters (+"+std::to_string(credits)+").";
        if(skipped)why+=" "+std::to_string(skipped)+" stacks left: capacity limit.";
        if(!consumed&&!skipped)why="No compatible gems or clusters on this storage page.";
    } else if(consumed)why="Deposited "+std::to_string(consumed)+" before stopping. "+why;
    return !failed;
}
bool SafeUi(const D2RL::PluginContext* c) noexcept {
    if(!admitted||!c)return false;
    using Mode=bool(__fastcall*)(int);
    __try {const auto mode=reinterpret_cast<Mode>(c->exeBase+0xce500);return !mode(0xb)&&!mode(0x17)&&CursorFree(c);}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool CursorFree(const D2RL::PluginContext* c) noexcept {
    __try {
        const auto cursor=reinterpret_cast<void*(__fastcall*)()>(c->exeBase+0x14f1e0)();
        const auto mode=reinterpret_cast<int(__fastcall*)()>(c->exeBase+0x14f210)();
        return !cursor&&(mode==0||mode==7||mode==8||mode==9);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Controller(const D2RL::PluginContext* c) noexcept {
    uintptr_t manager{};unsigned mode{};return c && Read(c->exeBase+0x3440170,manager)&&manager&&Read(manager+0xdc,mode)&&mode==1;
}
bool Focus(const D2RL::PluginContext* c,uint32_t& id,uint32_t& cls) noexcept {
    if(!Controller(c)||!SafeUi(c) || !c->CheckExpectedBytes(0x2c49f0,QolNativeIdentifyProfile::Lookup,sizeof(QolNativeIdentifyProfile::Lookup)))return false;
    __try {
        uintptr_t manager{},focus{},grid{},table{};Cell cell{};
        if(!Read(c->exeBase+0x3440170,manager)||!Read(manager+0xd0,focus)||!Read(focus+0x190,grid)||!Read(grid,table)||
            *reinterpret_cast<const uintptr_t*>(table+0xc8)!=c->exeBase+0x2c49f0||!Read(grid+0x544,cell)||cell.x>255||cell.y>255)return false;
        const auto unit=reinterpret_cast<uintptr_t(__fastcall*)(void*,const Cell*)>(c->exeBase+0x2c49f0)(reinterpret_cast<void*>(grid),&cell);
        std::array<uint32_t,3> identity{};
        if(!Read(unit,identity)||identity[0]!=4||!identity[2])return false;
        cls=identity[1];id=identity[2];return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
#ifdef GEMBAG_NATIVE_TESTS
void Testing::Configure(uintptr_t image,Testing::Setter setter,Testing::Publisher publisher) {
    core=image;setList=setter;testPublisher=publisher;admitted=true;
}
bool TestCapture(void* unit,const Info& expected,Snapshot& snapshot,unsigned& source,uint64_t& fingerprint,bool useHint) {
    Detail result{};
    const bool ok=Capture(unit,expected,result,useHint?source:UINT32_MAX,fingerprint);
    if(ok){snapshot=result.snapshot;source=result.sourceIndex;fingerprint=result.fingerprint;}
    return ok;
}
#endif
}
