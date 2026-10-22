#include "data_fixture.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

void Check(bool value,const char* name){if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
using namespace GemBag;
Catalog::Definitions Build(const TestData::Fixture& fixture) {
    Catalog::Definitions out;std::string why;Check(Catalog::Build(fixture.Views(),fixture.bank,out,why),why.c_str());
    if(out.empty())std::fprintf(stderr,"Catalog refusal: %s\n",why.c_str());return out;
}
int main(int argc,char** argv) {
    if(argc==2) {
        for(unsigned bank=1;bank<=3;++bank) {
            TestData::Fixture f;f.bank=bank;const char* names[]={"Items","ItemTypes","Gems","Properties","ItemStatCost","CubeMain"};
            for(unsigned i=0;i<6;++i){std::ifstream in(std::filesystem::path(argv[1])/(std::to_string(bank)+"-"+names[i]+".bin"),std::ios::binary);
                Check(in.good(),"recorded table fixture exists");f.bytes[i]={std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};}
            const auto out=Build(f);
            Check(bank==1?out.empty():out.size()==1&&out[0].mask==255&&out[0].creditStat==386&&out[0].property==396,"actual captured normal-package definitions");
            if(bank!=1)Check(out[0].choices[0].nameId==51371&&out[0].choices[5].nameId==51376&&out[0].choices[7].nameId==51517,"captured item names agree with the package's actual localization IDs");
            std::printf("Captured bank %u: %zu compatible bags, no mod-name/TXT/hash dependency.\n",bank,out.size());
        }
        return 0;
    }
    TestData::Fixture baseline;auto out=Build(baseline);
    Check(out.size()==1&&out[0].mask==255&&out[0].bagClass==746&&out[0].creditStat==386,"sampled recipe contract is recognized");
    TestData::Fixture capturedName;
    std::memcpy(capturedName.Row(0,capturedName.output[0]),TestData::RecordedAmethyst.data(),TestData::RecordedAmethyst.size());
    capturedName.bytes[1].resize(111*232);std::memcpy(capturedName.Row(1,97),"gema",4);
    out=Build(capturedName);Check(out.size()==1&&out[0].choices[0].nameId==51371,"real compiled item row resolves its name rather than the unrelated 65535 field");
    TestData::Fixture otherNames=baseline;
    for(unsigned item:{otherNames.bag,850u,otherNames.output[0]})otherNames.Set(0,item,0xfc,uint16_t{41000});
    out=Build(otherNames);Check(out[0].choices[0].nameId==41000&&out[0].fingerprint==Build(baseline)[0].fingerprint,"client and host may have different localized name references without changing withdrawal authority");
    otherNames.Row(0,otherNames.output[0])[0xfe]^=1;
    Check(Build(otherNames)[0].fingerprint!=Build(baseline)[0].fingerprint,"fingerprint exclusion is only the two name bytes, not adjacent gameplay fields");
    TestData::Fixture relocated(915,421,450);out=Build(relocated);
    Check(out.size()==1&&out[0].bagClass==915&&out[0].creditStat==421&&out[0].property==450,"bag class, stat and property may move independently");
    relocated.Set(0,relocated.bag,0x80,TestData::Fixture::ItemsCode("pou"));
    relocated.Set(0,relocated.output[5],0x80,TestData::Fixture::ItemsCode("gpr"));out=Build(relocated);
    Check(out.size()==1&&out[0].bagCode==TestData::Fixture::ItemsCode("pou")&&out[0].choices[5].code==TestData::Fixture::ItemsCode("gpr"),"bag and gem codes are discovered");
    TestData::Fixture seven;seven.Row(5,7)[0]=0;out=Build(seven);Check(out.size()==1&&out[0].mask==127,"a missing optional gem does not disable supported gems");
    TestData::Fixture vanilla;for(unsigned i=0;i<8;++i)vanilla.Row(5,i)[0]=0;Check(Build(vanilla).empty(),"no bag recipes leaves a generic/vanilla install inactive");
    TestData::Fixture alteredGem;alteredGem.Row(2,0)[0x38]^=1;
    Check(Build(alteredGem)[0].fingerprint!=Build(baseline)[0].fingerprint,"client/host fingerprint includes gem effect definitions");
    for(unsigned kind=0;kind<13;++kind) {
        TestData::Fixture f;
        switch(kind) {
        case 0:f.Row(5,0)[1]=1;break; // Ladder restriction.
        case 1:f.Row(5,0)[2]=2;break; // Difficulty restriction.
        case 2:f.Set(5,0,0x10,1u);break; // Season.
        case 3:f.Row(5,0)[0x77+10]=50;break; // Random debit.
        case 4:f.Set(5,0,0x77+6,int16_t{-2});break;
        case 5:f.Row(5,0)[0x1b]=2;break; // Cost quantity.
        case 6:f.Set(5,0,0x77+12,f.property);break; // Extra effect.
        case 7:f.Row(4,f.stat)[0x14]=8;break; // Fixed point.
        case 8:f.Row(4,f.stat)[0x50]=8;break; // Derived stat.
        case 9:f.Row(3,f.property)[0x19]=1;break; // Secondary property effect.
        case 10:f.Row(0,f.output[0])[0x11e]=2;break;
        case 11:f.Set(0,f.output[1],0x80,Gems[0].code);break; // Ambiguous code.
        case 12:f.Set(2,1,0x28,f.output[0]);break; // Ambiguous gem identity.
        }
        Check(Build(f).empty(),"unsupported or ambiguous withdrawal mechanics refuse the bag");
    }
    TestData::Fixture duplicate;const auto copy=std::vector<unsigned char>(duplicate.bytes[5].begin(),duplicate.bytes[5].begin()+364);
    duplicate.bytes[5].insert(duplicate.bytes[5].end(),copy.begin(),copy.end());
    for(unsigned n=0;n<3;++n)duplicate.Set(5,8,0x5a+n*91,8u);
    Check(Build(duplicate).empty(),"duplicate gem choice refuses instead of picking an arbitrary recipe");
    auto views=baseline.Views();views[1].revision=2;std::string why;
    Check(!Catalog::Build(views,3,out,why)&&out.empty(),"mixed revisions refuse atomically");
    views=baseline.Views();views[5].rowSize=360;Check(!Catalog::Build(views,3,out,why),"unknown compiled ABI refuses");
    const D2RL::PluginApi api{.apiSize=D2RL::PluginApiSize,.queryService=[](const D2RL::PluginContext*,D2RL::ServiceId id,uint32_t,const void** service) noexcept {
        if(id!=D2RL::DataTableService::Id)return D2RL::ServiceQueryResult::UnknownService;*service=&TestData::Service;return D2RL::ServiceQueryResult::Success;}};
    const D2RL::PluginContext context{.contextSize=sizeof(D2RL::PluginContext),.api=&api};TestData::active=&baseline;
    Check(Catalog::Refresh(&context,1),"game-thread discovery publishes owned definitions");
    Info bag{};bag.classId=baseline.bag;bag.code=BagCode;auto definition=Catalog::Find(3,bag);
    Check(definition&&Catalog::Candidate(bag)&&Catalog::Recheck(&context,*definition),"only discovered bags activate and current witnesses agree");
    baseline.Set(0,baseline.output[0],0xfc,uint16_t{41000});
    Check(!Catalog::Recheck(&context,*definition),"localization-neutral wire fingerprint does not relax exact local witness revalidation");
    baseline.Set(0,baseline.output[0],0xfc,uint16_t{1000});
    baseline.Row(5,0)[0x77+10]=99;Check(!Catalog::Recheck(&context,*definition),"same-revision foreign recipe changes refuse before debit");
    baseline.Row(5,0)[0x77+10]=100;baseline.revision=2;Check(!Catalog::Recheck(&context,*definition),"stale table revisions refuse before debit");
    baseline.revision=1;baseline.bytes[0].resize(baseline.bytes[0].size()+448);Check(!Catalog::Recheck(&context,*definition),"same-revision table growth refuses before debit");
    Catalog::Stop();Check(!Catalog::Candidate(bag)&&!Catalog::Find(3,bag),"stop discards activation identities");
    std::puts("Catalog: runtime definitions, relocated identities/codes, optional gems, vanilla inactivity, ambiguous/restricted mechanics, stale revisions and mutation witnesses passed.");
}
