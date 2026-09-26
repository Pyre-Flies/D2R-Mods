#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstring>

// Shared admission for every private D2RCore address. Never accept a new hash
// without recovering and reviewing the associated code and data RVAs.
namespace QolCore {
inline bool VerifyFileHash(HMODULE module, const unsigned char (&expected)[32]) noexcept {
    if (!module) return false;
    wchar_t path[32768]{};
    if (!GetModuleFileNameW(module,path,32768)) return false;
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE algorithm=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    unsigned char digest[32]{}; unsigned char buffer[65536]; DWORD got=0;
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok) ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    while(ok) {
        if(!ReadFile(file,buffer,sizeof(buffer),&got,nullptr)) {ok=false;break;}
        if(!got) break;
        ok=BCryptHashData(hash,buffer,got,0)>=0;
    }
    if(ok) ok=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash) BCryptDestroyHash(hash);
    if(algorithm) BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);
    return ok && std::memcmp(digest,expected,32)==0;
}
inline bool VerifyCore(HMODULE module) noexcept {
    constexpr unsigned char expected[32]={0xae,0x1e,0xa9,0xb7,0xf9,0x7a,0xf5,0xb8,0x9a,0x55,0x02,0x81,0xe6,0xa6,0xc6,0xb6,0xe9,0xc7,0x4e,0x73,0xac,0x87,0x59,0xe6,0x55,0x8b,0x40,0xe7,0x51,0x42,0x8c,0xd0};
    constexpr unsigned char query[]={0x55,0x56,0x48,0x83,0xec,0x28,0x48,0x8d,0x6c,0x24,0x20};
    unsigned char actual[sizeof(query)]{};
    SIZE_T read{};
    return VerifyFileHash(module,expected) &&
        ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<unsigned char*>(module)+0x456440,
            actual,sizeof(actual),&read) && read==sizeof(actual) &&
        std::memcmp(actual,query,sizeof(query))==0;
}

}
