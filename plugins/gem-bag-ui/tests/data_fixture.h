#pragma once
#include "catalog.h"
#include "recorded_data.h"

namespace TestData {
using namespace GemBag;
struct Fixture {
    unsigned bag=746,stat=386,property=396,bank=3;uint64_t revision=1;
    std::array<std::vector<unsigned char>,6> bytes;
    std::array<unsigned,8> output{692,694,696,698,702,700,704,794};
    template<class T> void Set(unsigned table,unsigned index,size_t offset,T value) {std::memcpy(Row(table,index)+offset,&value,sizeof(value));}
    unsigned char* Row(unsigned table,unsigned index){return bytes[table].data()+static_cast<size_t>(index)*Catalog::Sizes[table];}
    Fixture(unsigned b=746,unsigned s=386,unsigned p=396):bag(b),stat(s),property(p) {
        const unsigned counts[]={1000,16,8,512,512,8};
        for(unsigned i=0;i<bytes.size();++i)bytes[i].resize(static_cast<size_t>(counts[i])*Catalog::Sizes[i]);
        constexpr const char* types[]={"gema","gemt","gems","geme","gemd","gemr","gemz","gemo"};
        auto item=[&](unsigned id,unsigned code,unsigned type,unsigned name){
            Set(0,id,0x80,code);Row(0,id)[0x11e]=Row(0,id)[0x11f]=1;
            Set(0,id,0x12e,static_cast<int16_t>(type));Set(0,id,0xfc,static_cast<uint16_t>(name));
            Set(0,id,0xbc,uint16_t{0xffff}); // Observed non-name field, not an SDK name ID.
        };
        item(bag,ItemsCode("bag"),9,2000);
        std::memcpy(Row(4,stat),RecordedStat.data(),RecordedStat.size());Set(4,stat,0,static_cast<uint16_t>(stat));
        std::memcpy(Row(3,property),RecordedProperty.data(),RecordedProperty.size());Set(3,property,0,property);Set(3,property,0x20,static_cast<uint16_t>(stat));
        for(unsigned i=0;i<8;++i) {
            item(output[i],Gems[i].code,i+1,1000+i);item(850+i,ItemsCode("gr0")+((i+1)<<16),10,3000+i);
            std::memcpy(Row(1,i+1),types[i],4);Set(2,i,0x28,output[i]);
            auto row=Row(5,i);std::memcpy(row,RecordedRecipe.data(),RecordedRecipe.size());
            Set(5,i,4,stat);Set(5,i,0x16,bag);Set(5,i,0x20,850+i);Set(5,i,0x77,property);
            for(unsigned n=0;n<3;++n)Set(5,i,0x5a+n*91,i);
            Set(5,i,0xbb,850+i);Set(5,i,0x116,output[i]);
        }
    }
    template<size_t N> static unsigned ItemsCode(const char(&text)[N]){return D2RL::Items::MakeItemCode(text);}
    Catalog::Views Views() const {
        Catalog::Views views{};
        for(unsigned i=0;i<views.size();++i)views[i]={.structSize=D2RL::DataTables::TableViewSize,.tableId=Catalog::Ids[i],
            .bank=static_cast<D2RL::DataTables::Bank>(bank),.revision=revision,.rows=bytes[i].data(),
            .rowCount=static_cast<uint32_t>(bytes[i].size()/Catalog::Sizes[i]),.rowSize=Catalog::Sizes[i]};
        return views;
    }
};
inline Fixture* active{};
inline D2RL::DataTableService Service{.serviceSize=D2RL::DataTableServiceSize,.serviceVersion=D2RL::DataTableService::AbiVersion,
    .getTable=[](const D2RL::PluginContext*,D2RL::DataTables::Bank bank,D2RL::DataTables::TableId id,D2RL::DataTables::TableView* view) noexcept {
        if(!active||static_cast<unsigned>(bank)!=active->bank)return D2RL::DataTables::Result::Unavailable;
        const auto found=std::find(Catalog::Ids.begin(),Catalog::Ids.end(),id);if(found==Catalog::Ids.end())return D2RL::DataTables::Result::NotFound;
        *view=active->Views()[static_cast<size_t>(found-Catalog::Ids.begin())];return D2RL::DataTables::Result::Success;
    }};
}
