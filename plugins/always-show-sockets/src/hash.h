#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <cstdio>
#include <cstring>
namespace Sockets {
inline bool Hash(HMODULE module,const char* expected) noexcept {
    wchar_t path[32768]{};
    const auto length=GetModuleFileNameW(module,path,32768);
    if(!length || length>=32768) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    bool match=false;
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0) {
        DWORD objectSize{},got{};
        if(BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&got,0)>=0) {
            try {
                std::vector<unsigned char> object(objectSize),block(65536);
                if(BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0) {
                    DWORD count{}; bool valid=true;
                    for(;;) {
                        if(!ReadFile(file,block.data(),static_cast<DWORD>(block.size()),&count,nullptr)) {valid=false;break;}
                        if(!count) break;
                        if(BCryptHashData(hash,block.data(),count,0)<0) {valid=false;break;}
                    }
                    unsigned char digest[32]{};
                    if(valid && BCryptFinishHash(hash,digest,32,0)>=0) {
                        char text[65]{};
                        for(unsigned i=0;i<32;++i) std::snprintf(text+i*2,3,"%02X",digest[i]);
                        match=std::strcmp(text,expected)==0;
                    }
                    BCryptDestroyHash(hash); hash=nullptr;
                }
            } catch(...) { if(hash) BCryptDestroyHash(hash); }
        }
        BCryptCloseAlgorithmProvider(alg,0);
    }
    CloseHandle(file); return match;
}
}
