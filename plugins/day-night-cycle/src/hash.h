#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <cstdio>
#include <cstring>

namespace DayNight {
inline bool FileHash(HMODULE module,const char* expected) noexcept {
    if(!module || !expected) return false;
    wchar_t path[32768]{};
    const auto length=GetModuleFileNameW(module,path,32768);
    if(!length || length>=32768) return false;
    const auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    bool match=false;
    try {
        if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0) {
            DWORD size{},got{};
            if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)>=0) {
                std::vector<unsigned char> object(size),block(65536);
                if(BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0) {
                    bool valid=true;
                    for(;;) {
                        DWORD count{};
                        if(!ReadFile(file,block.data(),static_cast<DWORD>(block.size()),&count,nullptr)) {valid=false;break;}
                        if(!count) break;
                        if(BCryptHashData(hash,block.data(),count,0)<0) {valid=false;break;}
                    }
                    unsigned char digest[32]{};
                    if(valid && BCryptFinishHash(hash,digest,32,0)>=0) {
                        char text[65]{};
                        for(unsigned i=0;i<32;++i) std::snprintf(text+2*i,3,"%02X",digest[i]);
                        match=std::strcmp(text,expected)==0;
                    }
                }
            }
        }
    } catch(...) { }
    if(hash) BCryptDestroyHash(hash);
    if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    return match;
}
}
