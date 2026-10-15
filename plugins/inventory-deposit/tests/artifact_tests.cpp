#include <D2RLPlugin/api.h>
#include <D2RLPlugin/resource.h>
#include "button_art.h"
#include "button_art_resource.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s (Win32=%lu)\n",why,GetLastError());std::exit(1);}}
template<class T>T Read(const unsigned char* bytes,std::size_t offset){
    T value{};std::memcpy(&value,bytes+offset,sizeof(value));return value;
}
void CheckSprite(HMODULE module,unsigned id,uint16_t frameWidth,uint32_t frameHeight){
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(id),MAKEINTRESOURCEW(10));
    Check(resource!=nullptr,"custom sprite RCDATA exists in the plugin DLL");
    const std::size_t size=SizeofResource(module,resource);
    const uint32_t width=4u*frameWidth,height=frameHeight;
    const std::size_t expected=40u+static_cast<std::size_t>(width)*height*4u;
    Check(size==expected && size<=InventoryDepositButtonArt::MaximumByteCount,
        "sprite contains exactly the header and four uncompressed RGBA frames");
    const auto bytes=static_cast<const unsigned char*>(LockResource(LoadResource(module,resource)));
    Check(bytes && !std::memcmp(bytes,"SpA1",4) && Read<uint16_t>(bytes,4)==31 &&
        Read<uint16_t>(bytes,6)==frameWidth && Read<uint32_t>(bytes,8)==width &&
        Read<uint32_t>(bytes,12)==height && Read<uint32_t>(bytes,16)==0 &&
        Read<uint32_t>(bytes,20)==4,"sprite header matches the converter's four-frame format and dimensions");
    for(std::size_t i=24;i<40;++i)Check(bytes[i]==0,"unused sprite header fields stay zero");
    for(unsigned frame=0;frame<4;++frame){
        bool transparent{},visible{},differsFromNormal{};
        for(uint32_t y=0;y<height;++y){
            const auto normal=bytes+40u+static_cast<std::size_t>(y)*width*4u;
            const auto row=normal+static_cast<std::size_t>(frame)*frameWidth*4u;
            if(frame && std::memcmp(row,normal,static_cast<std::size_t>(frameWidth)*4u)!=0)differsFromNormal=true;
            for(uint32_t x=0;x<frameWidth;++x){
                const auto alpha=row[x*4u+3u];
                transparent=transparent || alpha==0;
                visible=visible || alpha!=0;
            }
        }
        Check(transparent && visible,"each button frame has visible art and transparent background pixels");
        Check(!frame || differsFromNormal,"hover, pressed and disabled artwork differ from the normal frame");
    }
}
int main(int argc,char** argv){
    Check(argc==2,"DLL path required");
    auto image=LoadLibraryExA(argv[1],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(image!=nullptr,"artifact opens as data/image resource");
    auto resource=FindResourceA(image,MAKEINTRESOURCEA(D2RL_PLUGIN_MANIFEST_RESOURCE_ID),MAKEINTRESOURCEA(10));
    Check(resource && SizeofResource(image,resource)==sizeof(DWORD),"one DWORD ABI manifest");
    auto abi=static_cast<const DWORD*>(LockResource(LoadResource(image,resource)));
    Check(abi && *abi==D2RL_PLUGIN_ABI_VERSION,"manifest matches loader ABI");
    auto config=FindResourceA(image,MAKEINTRESOURCEA(D2RL_PLUGIN_CONFIG_RESOURCE_ID),MAKEINTRESOURCEA(10));
    Check(config!=nullptr,"standalone configuration embedded");
    auto data=static_cast<const char*>(LockResource(LoadResource(image,config)));
    Check(data!=nullptr,"embedded config readable");
    std::string_view text(data,SizeofResource(image,config));
    Check(text.find("[inventory_deposit]")!=text.npos && text.find("enabled = true")!=text.npos,"independent defaults");
    CheckSprite(image,INVENTORY_DEPOSIT_BUTTON_SPRITE_RESOURCE_ID,144,136);
    CheckSprite(image,INVENTORY_DEPOSIT_BUTTON_LOWEND_RESOURCE_ID,72,68);
    FreeLibrary(image);
    auto module=LoadLibraryExA(argv[1],nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
    Check(module!=nullptr,"DLL and Windows dependencies load without Controller QOL");
    using GetInfo=const D2RL::PluginInfo*(*)() noexcept;
    auto getInfo=reinterpret_cast<GetInfo>(GetProcAddress(module,"D2RLoaderGetPluginInfo"));
    Check(getInfo && GetProcAddress(module,"D2RLoaderLoadPlugin") && GetProcAddress(module,"D2RLoaderUnloadPlugin"),"three lifecycle exports");
    const auto* info=getInfo();
    Check(info && info->infoSize==D2RL::PluginInfoSize && info->abiVersion==D2RL_PLUGIN_ABI_VERSION,"SDK metadata ABI");
    Check(!std::strcmp(info->id,"inventory-deposit") && !std::strcmp(info->version,"1.3.1+rev.13"),"standalone identity and stable version");
    Check((static_cast<unsigned>(info->flags)&static_cast<unsigned>(D2RL::PluginFlags::Shared))!=0,"valid shared plugin role");
    // No load callback invocation outside the actual game/loader host.
    FreeLibrary(module);std::puts("Standalone DLL ABI and custom art resource admission, metadata, dependencies and exports passed.");
}
