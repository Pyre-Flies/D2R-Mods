// Optional local replay. Extracted game tables remain outside the repository.
#pragma once
#include <fstream>
inline void AuditActiveWords(const char* path){
    std::ifstream input(path,std::ios::binary);check(input.good());char magic[4]{};input.read(magic,4);
    const bool catalog=std::memcmp(magic,"IRR2",4)==0;check(catalog || std::memcmp(magic,"IRR1",4)==0);
    const unsigned tableCount=catalog?6u:4u;
    std::vector<std::vector<unsigned char>> bytes(tableCount);std::vector<ContributionDefinitions::Table> tables;
    const unsigned strides[]={232,448,232,48,140,200};
    for(unsigned n=0;n<tableCount;++n){unsigned count{},stride{};input.read(reinterpret_cast<char*>(&count),4);input.read(reinterpret_cast<char*>(&stride),4);
        check(input.good() && count>0 && count<=32768 && stride==strides[n]);bytes[n].resize(static_cast<std::size_t>(count)*stride);
        input.read(reinterpret_cast<char*>(bytes[n].data()),static_cast<std::streamsize>(bytes[n].size()));check(input.good());tables.push_back({bytes[n].data(),count,stride});}
    check(input.peek()==std::char_traits<char>::eof());
    unsigned enabled=0,resolved=0,ambiguous=0,unmatched=0,total=0,scalar=0;
    for(std::size_t word=0;word<tables[0].count;++word){
        const auto* row=tables[0].Row(word);if(!row[0x44] || !Affixes::Read<unsigned short>(row,0x46))continue;++enabled;
        std::vector<Contributions::Socket> sockets;
        for(unsigned n=0;n<6;++n){const int id=Affixes::Read<int>(row,0x5c+n*4);if(id>0){Contributions::Socket s;s.identity.classId=static_cast<unsigned>(id);sockets.push_back(s);}}
        check(!sockets.empty());std::vector<Affixes::Rolled> definitions;unsigned specCount=0;
        for(unsigned n=0;n<7;++n){const auto id=Affixes::Read<unsigned>(row,0x74+n*16);if(id==UINT32_MAX)continue;++specCount;
            if(id>=tables[3].count)continue;auto d=roll(id,Affixes::Read<int>(row,0x7c+n*16),Affixes::Read<int>(row,0x80+n*16),"[RW]");
            put(d.row.bytes.data(),0x28,Affixes::Read<int>(row,0x78+n*16));definitions.push_back(d);}
        const auto properties=std::span(reinterpret_cast<const Affixes::Property*>(tables[3].rows),tables[3].count);
        total+=specCount;scalar+=static_cast<unsigned>(PropertyRolls::Compile(definitions,properties).variables.size());
        bool matched=false;unsigned hits=0;
        for(std::size_t host=0;host<tables[1].count;++host){
            const auto* item=tables[1].Row(host);const int p=Affixes::Read<std::int16_t>(item,0x12e),s=Affixes::Read<std::int16_t>(item,0x130);
            bool allowed=false,denied=false,valid=true;
            for(unsigned n=0;n<9;++n){const int type=Affixes::Read<std::int16_t>(row,0x4a+n*2);if(!type)continue;
                const auto result=ContributionDefinitions::Type(tables[2],p,s,static_cast<unsigned>(type));if(!result){valid=false;break;}
                if(n<6)allowed|=*result;else denied|=*result;}
            if(!valid || !allowed || denied)continue;matched=true;
            const auto result=RunewordSources::Resolve(tables[0],tables[1],tables[2],static_cast<unsigned>(host),2,0x04000000,sockets,tables[3].count,32768);
            if(!result){++ambiguous;continue;}
            check(result->size()==specCount);
            unsigned at=0;for(unsigned n=0;n<7;++n){const auto id=Affixes::Read<unsigned>(row,0x74+n*16);if(id==UINT32_MAX)continue;
                const auto& spec=(*result)[at++];check(spec.property==id && spec.parameter==Affixes::Read<int>(row,0x78+n*16) &&
                    spec.low==Affixes::Read<int>(row,0x7c+n*16) && spec.high==Affixes::Read<int>(row,0x80+n*16));}
            ++hits;
        }
        if(hits)++resolved;else if(!matched)++unmatched;
    }
    std::printf("Active compiled audit: %u enabled recipes; %u resolve; %u without eligible base; %u refused recipe/base matches; %u specs; %u linked scalar variables.\n",enabled,resolved,unmatched,ambiguous,total,scalar);
    if(!catalog)return;
    const auto properties=std::span(reinterpret_cast<const Affixes::Property*>(tables[3].rows),tables[3].count);
    const auto groups=std::span(reinterpret_cast<const Affixes::PropertyGroup*>(tables[5].rows),tables[5].count);
    unsigned modeled=0,blocked=0,incomplete=0,randomSpecs=0;
    for(std::size_t n=0;n<tables[4].count;++n){Affixes::Rolled d{};d.id=static_cast<unsigned>(n+1);std::memcpy(d.row.bytes.data(),tables[4].Row(n),140);
        auto model=PropertyRolls::Compile(std::span(&d,1),properties,groups);check(model.variables.size()<=128);
        modeled+=!model.variables.empty();blocked+=!model.blocked.empty();incomplete+=!model.complete;
        for(unsigned spec=0;spec<3;++spec){const int id=d.row.Number(0x24+spec*16);if(id<0 || static_cast<std::size_t>(id)>=properties.size())continue;
            const auto& p=properties[static_cast<std::size_t>(id)].bytes;
            for(unsigned slot=0;slot<7;++slot)if(p[0x18+slot]==12){
                ++randomSpecs;auto isolated=d;for(unsigned j=0;j<3;++j)put(isolated.row.bytes.data(),0x24+j*16,-1);
                std::memcpy(isolated.row.bytes.data()+0x24,d.row.bytes.data()+0x24+spec*16,16);
                const int low=isolated.row.Number(0x2c),high=isolated.row.Number(0x30),bonus=isolated.row.Number(0x28);
                const auto stat=Affixes::Read<std::uint16_t>(p.data(),0x20+slot*2);
                check(Affixes::RandomLayerSpec(bonus,low,high,static_cast<unsigned>(low)));
                for(int layer:{low,(low+high)/2,high})check(PropertyRolls::LayerBounds(isolated,properties,stat,static_cast<unsigned>(layer),true)==PropertyRolls::Interval{bonus,bonus});
            }
        }}
    std::printf("Full affix catalog: %zu compiled rows inspected; %u with linked scalar variables; %u with blocked scalar operations; %u malformed/unresolved models. Other operations have separate native paths.\n",tables[4].count,modeled,blocked,incomplete);
    std::printf("Random selector catalog: %u specifications verify fixed parameter bounds at lower, middle and upper selected layers.\n",randomSpecs);
}
