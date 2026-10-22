#include "native.h"
#include "profile.h"
#include "data_fixture.h"
#include "deposit_fixture.h"
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace GemBag::Native {bool TestCapture(void*,const Info&,Snapshot&,unsigned&,uint64_t&,bool);}
void Check(bool b,const char* name){if(!b){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
template<size_t N,class T> void Put(std::array<std::byte,N>& a,size_t offset,const T& v){Check(offset+sizeof(v)<=N,"fixture bounds");std::memcpy(a.data()+offset,&v,sizeof(v));}
template<class T> uintptr_t Address(T& v){return reinterpret_cast<uintptr_t>(&v);}
struct Record {uint32_t layer,id;int32_t value;uint32_t pad;};
struct Fixture {
    std::array<std::byte,512> unit{};
    std::array<std::byte,256> data{},root{},child{};
    std::array<std::byte,128> inventory{};
    std::array<std::byte,288> grids{};
    std::array<uintptr_t,256> cells{};
    std::array<Record,2> modifiers{{{0,386,1,0},{0,400,7,0}}};
    Record aggregate{0,386,1,0};
    GemBag::Info info{};
    Fixture(GemBag::Container container,unsigned w,unsigned h,unsigned bagClass=746,unsigned credit=386,unsigned code=GemBag::BagCode) {
        info.handle=1;info.runtimeId=74;info.classId=bagClass;info.code=code;info.container=container;info.x=1;info.y=1;
        modifiers[0].id=aggregate.id=credit;modifiers[1].id=500;
        info.inventoryPage=container==GemBag::Container::Cube?3:container==GemBag::Container::Inventory?0:4;info.sharedStashPage=0;
        Put(unit,0,std::array<uint32_t,3>{4,bagClass,74});Put(unit,0x10,Address(data));Put(unit,0x88,Address(root));Put(unit,0x1bd,uint8_t{3});
        Put(data,0xa0,Address(inventory));Put(data,0x55,static_cast<uint8_t>(info.inventoryPage));
        Put(inventory,0,uint32_t{0x01020304});Put(inventory,0x20,Address(grids));Put(inventory,0x28,uint32_t{9});
        const size_t grid=(static_cast<size_t>(info.inventoryPage)+2)*32;
        Put(grids,grid+0x10,static_cast<uint8_t>(w));Put(grids,grid+0x11,static_cast<uint8_t>(h));Put(grids,grid+0x18,Address(cells));cells[w+1]=Address(unit);
        Put(root,0xa0,Address(unit));Put(root,0x1c,uint32_t{0x80000000});Put(root,0x90,Address(child));Put(root,0xa8,Address(aggregate));Put(root,0xb0,uint64_t{1});
        Put(child,0,Address(unit));Put(child,0x1c,uint32_t{0x40});Put(child,0x18,int32_t{-1});Put(child,0x78,Address(root));Put(child,0x30,Address(modifiers));Put(child,0x38,uint64_t{2});
    }
    bool Capture(GemBag::Native::Snapshot& out,unsigned& source,uint64_t& fingerprint,bool hinted=false) {
        return GemBag::Native::TestCapture(unit.data(),info,out,source,fingerprint,hinted);
    }
};
namespace Creation {
using namespace D2RL;
using namespace GemBag;
Fixture* fixture{};
Items::ItemCreateSpec requested{};
Info output{};
bool exists{},clearIdentified{},depositPublishFailure{};
unsigned destroyMode{},moveMode{},moves{};bool depositing{};
unsigned creates{},removes{},publications{};
TestData::Fixture data;
bool __fastcall Setter(uint8_t bank,void* list,int32_t stat,int32_t value,uint32_t layer) {
    Check(fixture&&bank==3&&list==fixture->child.data()&&stat==static_cast<int32_t>(data.stat)&&!layer,"actual debit uses the discovered stat and owned source");
    fixture->modifiers[0].value=fixture->aggregate.value=value;return true;
}
bool Publisher(const PluginContext*,const ItemService*,const Info& bag,bool send) {
    Check(bag.handle==fixture->info.handle,"publication retains the selected bag");
    if(send)++publications;return !(send&&depositPublishFailure);
}
ServiceQueryResult __cdecl Query(const PluginContext*,ServiceId id,uint32_t version,const void** service) noexcept {
    if(id!=DataTableService::Id||version!=DataTableService::AbiVersion)return ServiceQueryResult::UnknownService;
    *service=&TestData::Service;return ServiceQueryResult::Success;
}
ItemService items{.serviceSize=ItemServiceSize,.serviceVersion=ItemService::AbiVersion,
    .getItemInfo=[](const PluginContext*,ItemHandle handle,Info* info) noexcept {
        if(handle==fixture->info.handle){*info=fixture->info;return Items::Result::Success;}
        if(handle!=2||!exists)return Items::Result::NotFound;*info=output;return Items::Result::Success;
    },
    .createItem=[](const PluginContext*,PlayerHandle player,const Items::ItemCreateSpec* spec,ItemHandle* handle) noexcept {
        Check(player==1&&!exists,"create exactly one output for the selected player");requested=*spec;++creates;exists=true;
        output={.structSize=sizeof(Info)};output.handle=2;output.runtimeId=80;output.code=spec->code;output.quality=spec->quality;
        for(auto id:data.output)if(Catalog::Read<uint32_t>(data.Row(0,id),0x80)==spec->code)output.classId=id;
        output.container=spec->destination.container;output.sharedStashPage=spec->destination.sharedStashPage;
        output.x=static_cast<int32_t>(spec->destination.x);output.y=static_cast<int32_t>(spec->destination.y);
        output.stateFlags=clearIdentified?0:spec->stateFlags;*handle=2;return Items::Result::Success;
    },
    .destroyItem=[](const PluginContext*,PlayerHandle player,ItemHandle handle,Items::SocketedItemPolicy policy) noexcept {
        Check(player==1&&handle==2&&exists&&policy==Items::SocketedItemPolicy::RejectIfNotEmpty,"remove only failed output before refund");
        Check(!depositing||output.container!=Container::SharedStash,"shared inputs must be staged before destroy");
        if(destroyMode==1)return Items::Result::PolicyRejected;
        if(destroyMode==2)return Items::Result::CallbackFault;
        exists=false;++removes;return Items::Result::Success;
    },
    .editNativeItem=[](const PluginContext* context,ItemHandle handle,Items::NativeItemEditCallback callback,void* job) noexcept {
        Check(handle==fixture->info.handle,"native edit binds the owned bag");callback(context,fixture->unit.data(),job);return Items::Result::Success;
    },
    .executeExistingItemTransaction=[](const PluginContext*,const Items::ExistingItemTransaction* txn,Items::ExistingItemTransactionResult* result) noexcept {
        Check(txn->player==1&&txn->operationCount==1&&txn->operations[0].item==2,"stage exactly the selected item");
        const auto& op=txn->operations[0];const bool back=op.move.destination.container==Container::SharedStash;
        result->committedOperationCount=0;
        if(moveMode==1&&!back)return Items::Result::PolicyRejected;
        if(moveMode==2&&back)return Items::Result::PolicyRejected;
        if(moveMode==3&&!back)return Items::Result::CallbackFault;
        if(back){Check(op.move.destination.placement==Items::Placement::Exact,"compensation uses exact source cell");output.container=Container::SharedStash;output.inventoryPage=4;output.sharedStashPage=op.move.destination.sharedStashPage;output.x=static_cast<int>(op.move.destination.x);output.y=static_cast<int>(op.move.destination.y);}
        else {Check(op.move.destination.placement==Items::Placement::Automatic,"stage uses loader occupancy check");output.container=Container::Inventory;output.inventoryPage=0;output.sharedStashPage=UINT32_MAX;output.x=3;output.y=2;}
        ++moves;result->committedOperationCount=1;return Items::Result::Success;
    },
    .capabilities=Items::ItemServiceCapabilityBit(Items::ItemServiceCapability::SharedStashWrite)};
void Run() {
    auto image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x840000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(image!=nullptr,"owned Core witness allocation");
    std::memcpy(image+0x831ed0,Profile::SetList,sizeof(Profile::SetList));
    std::memcpy(image+0x3d95c0,Profile::SetListInner,sizeof(Profile::SetListInner));
    Native::Testing::Configure(reinterpret_cast<uintptr_t>(image),Setter,Publisher);
    const PluginApi api{.apiSize=PluginApiSize,.queryService=Query};
    const PluginContext context{.contextSize=sizeof(PluginContext),.abiVersion=D2RL_PLUGIN_ABI_VERSION,.api=&api};
    for(bool relocated:{false,true}) {
      data=relocated?TestData::Fixture(915,421,450):TestData::Fixture();TestData::active=&data;
      if(relocated){data.Set(0,data.bag,0x80,TestData::Fixture::ItemsCode("pou"));data.Set(0,data.output[5],0x80,TestData::Fixture::ItemsCode("gpr"));}
      Check(Catalog::Refresh(&context,1),"real creation discovers the current compiled data");
      for(auto container:{Container::Inventory,Container::Cube,Container::PersonalStash,Container::SharedStash})for(unsigned gem=0;gem<Gems.size();++gem)for(bool broken:{false,true}) {
        Fixture current(container,12,8,data.bag,data.stat,Catalog::Read<uint32_t>(data.Row(0,data.bag),0x80));fixture=&current;current.aggregate.value=current.modifiers[0].value=2;
        exists=false;creates=removes=publications=0;clearIdentified=broken;
        Native::Snapshot result{};std::string why;bool halted{};
        const auto success=Native::Execute(&context,&items,1,current.info,gem,result,why,halted);
        Check(requested.stateFlags==Items::ItemStateIdentified&&requested.quality==Items::Quality::Normal,"every gem explicitly requests identified normal creation");
        Check(requested.code==Catalog::Read<uint32_t>(data.Row(0,data.output[gem]),0x80)&&requested.destination.container==container&&requested.destination.placement==Items::Placement::Exact,"actual creation preserves the discovered gem and exact storage");
        if(broken)Check(!success&&!exists&&removes==1&&current.aggregate.value==2&&!halted,"unidentified provider output is removed and its credit restored");
        else Check(success&&exists&&(output.stateFlags&Items::ItemStateIdentified)&&creates==1&&!removes&&current.aggregate.value==1&&!halted,"identified output spends exactly one credit");
        Check(publications==1&&current.modifiers[1].value==7,"bag refreshed once with unrelated modifier preserved");
    }
    }
    data=TestData::Fixture();TestData::AddDeposits(data);TestData::active=&data;
    Check(Catalog::Refresh(&context,1),"deposit definitions published");
    const InventoryService inventoryService{.serviceSize=InventoryServiceSize,.serviceVersion=InventoryService::AbiVersion,
        .forEachInventoryItem=[](const PluginContext* c,PlayerHandle player,const Inventory::ItemFilter* filter,Inventory::ItemCallback callback,void* state) noexcept {
            Check(player==1&&(filter->containerMask&Items::ContainerBit(output.container)),"deposit enumerates requested owner and storage");
            callback(c,&fixture->info,state);if(exists)callback(c,&output,state);return Inventory::Result::Success;
        }};
    for(auto container:{Container::Inventory,Container::Cube,Container::PersonalStash,Container::SharedStash})for(unsigned kind=0;kind<2;++kind)for(unsigned failure=0;failure<8;++failure) {
        Fixture current(container,12,8);fixture=&current;current.aggregate.value=current.modifiers[0].value=failure==4?65530:5;
        current.cells.fill(Address(current.unit)); // Full grids must still accept deposits.
        output=current.info;output.handle=2;output.runtimeId=80;output.classId=kind?900:data.output[0];output.code=Catalog::Read<uint32_t>(data.Row(0,output.classId),0x80);
        output.quantity=kind?3:6;output.quality=Items::Quality::Normal;output.x=0;output.y=0;
        exists=true;depositing=true;creates=removes=publications=moves=0;destroyMode=(failure==1||failure==6)?1:failure==2?2:0;depositPublishFailure=failure==3;moveMode=failure==5?1:failure==6?2:failure==7?3:0;
        Native::Snapshot result{};std::string why;bool halted{};
        const bool success=Native::Deposit(&context,&items,&inventoryService,1,current.info,result,why,halted);
        if(failure==0&&(!success||exists))std::fprintf(stderr,"Deposit container=%u kind=%u success=%u exists=%u credits=%d halted=%u reason=%s\n",static_cast<unsigned>(container),kind,success,exists,current.aggregate.value,halted,why.c_str());
        if(failure==0)Check(success&&!exists&&!halted&&removes==1&&!creates&&current.aggregate.value>5&&current.aggregate.value<=(kind?95:11),"actual deposit adds stack credits and destroys once without requiring space");
        if(failure==1)Check(!success&&exists&&!halted&&current.aggregate.value==5,"clean destroy refusal restores actual bag credit");
        if(failure==1)Check(why=="Gem removal refused; this item's credits were restored.","clean removal refusal reports confirmed compensation");
        if(failure==2)Check(!success&&exists&&halted&&current.aggregate.value>5,"uncertain destruction halts without refunding blindly");
        if(failure==3)Check(!success&&!exists&&halted&&current.aggregate.value>5,"post-consumption publication failure keeps authoritative credits and halts");
        if(failure==4)Check(success&&exists&&!halted&&current.aggregate.value==65530&&!removes,"cap skips entire stack before random sampling or consumption");
        if(failure==5&&container==Container::SharedStash)Check(!success&&exists&&!halted&&current.aggregate.value==5&&moves==0,"full staging inventory refuses and refunds");
        if(failure==6&&container==Container::SharedStash)Check(!success&&exists&&halted&&moves==1&&output.container==Container::Inventory&&current.aggregate.value>5,"failed source restoration halts without unsafe refund");
        if(failure==7&&container==Container::SharedStash)Check(!success&&exists&&halted&&!moves&&current.aggregate.value>5,"uncertain staging halts");
        if(failure==1&&container==Container::SharedStash)Check(moves==2&&output.container==Container::SharedStash&&output.x==0&&output.y==0,"clean consumption refusal restores exact original location");
        if(failure==0&&container==Container::SharedStash)Check(moves==1,"successful shared deposit stages once");
        Check(current.modifiers[1].value==7,"deposits preserve unrelated bag modifier");
    }
    destroyMode=0;depositPublishFailure=false;
    Native::Testing::Configure(0,nullptr,nullptr);VirtualFree(image,0,MEM_RELEASE);
    std::puts("Actual withdrawal creation/verification: all eight gems in four stores, identified state and failed-output compensation; SDK/setter/publisher substitutes.");
}
}
int main() {
    using namespace GemBag;
    TestData::active=&Creation::data;const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.queryService=Creation::Query};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.api=&api};Check(Catalog::Refresh(&context,1),"capture fixtures discover bag definitions");
    unsigned n=0;
    for(auto container:{Container::Inventory,Container::Cube,Container::PersonalStash,Container::SharedStash})for(unsigned gem=0;gem<8;++gem) {
        Fixture f(container,12,8);Native::Snapshot out{};unsigned source=UINT32_MAX;uint64_t fingerprint{};
        Check(f.Capture(out,source,fingerprint)&&out.credits==1&&out.cell&&out.cell->x==0&&out.cell->y==0,"owned one-credit bag and actual free cell");
        // The native list setter removes a zero-valued entry. An unrelated
        // modifier remains, so the emptied credit source cannot be found by
        // guessing that its entire list is empty.
        f.modifiers[0]=f.modifiers[1];Put(f.child,0x38,uint64_t{1});f.aggregate.value=0;
        Check(f.Capture(out,source,fingerprint,true)&&out.credits==0,"last-credit source survives removal through metadata witness");
        f.cells.fill(Address(f.unit));Check(f.Capture(out,source,fingerprint,true)&&!out.cell,"full actual grid refuses placement");
        Put(f.root,0xa0,uintptr_t{0x10000});Check(!f.Capture(out,source,fingerprint,true),"foreign stat owner refused");++n;
    }
    Fixture f(Container::Cube,12,8);Native::Snapshot out{};unsigned source=UINT32_MAX;uint64_t fingerprint{};
    Check(f.Capture(out,source,fingerprint),"baseline witness");
    f.modifiers[0]=f.modifiers[1];Put(f.child,0x38,uint64_t{1});f.aggregate.value=0;f.modifiers[0].value=8;
    Check(!f.Capture(out,source,fingerprint,true),"changed non-credit source fingerprint refused");
    Fixture cycle(Container::Cube,12,8);Put(cycle.child,0x68,Address(cycle.child));
    Check(!cycle.Capture(out,source,fingerprint),"cyclic modifier lists refused");
    Fixture invalid(Container::Cube,12,8);Put(invalid.grids,5*32+0x10,uint8_t{17});
    Check(!invalid.Capture(out,source,fingerprint),"out-of-range dimensions refused");
    Creation::Run();
    std::printf("%u native-layout storage/gem cases plus source/lifetime refusal checks passed.\n",n);
}
