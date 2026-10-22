#pragma once
#include <D2RLPlugin/api.h>
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace GemBag::Admission {
using Digest=std::array<unsigned char,32>;
struct HashResult {
    Digest digest{};
    const char* stage="open-file";
    DWORD error{};
    LONG cryptoStatus{};
    bool ok{};
};
inline HashResult ReadHash(const std::filesystem::path& path,DWORD sharing=FILE_SHARE_READ) noexcept {
    HashResult result;
    const auto file=CreateFileW(path.c_str(),GENERIC_READ,sharing,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){result.error=GetLastError();return result;}
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    result.stage="sha256-provider";result.cryptoStatus=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0);
    bool ok=result.cryptoStatus>=0;
    if(ok){result.stage="sha256-create";result.cryptoStatus=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0);ok=result.cryptoStatus>=0;}
    unsigned char buffer[65536];DWORD got{};
    while(ok) {
        result.stage="read-file";
        if(!ReadFile(file,buffer,sizeof(buffer),&got,nullptr)){result.error=GetLastError();ok=false;break;}
        if(!got)break;
        result.stage="sha256-update";result.cryptoStatus=BCryptHashData(hash,buffer,got,0);ok=result.cryptoStatus>=0;
    }
    if(ok){result.stage="sha256-finish";result.cryptoStatus=BCryptFinishHash(hash,result.digest.data(),static_cast<ULONG>(result.digest.size()),0);ok=result.cryptoStatus>=0;}
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);CloseHandle(file);
    result.ok=ok;return result;
}
template<size_t N> inline std::array<char,N*2+1> Hex(const unsigned char* bytes) noexcept {
    constexpr char digits[]="0123456789abcdef";std::array<char,N*2+1> result{};
    for(size_t i=0;i<N;++i){result[2*i]=digits[bytes[i]>>4];result[2*i+1]=digits[bytes[i]&15];}
    return result;
}
class Report {
public:
    explicit Report(const D2RL::PluginContext* owner):context(owner){}
    template<class... Args> bool Fail(const char* format,Args... args) noexcept {
        char detail[1536]{},line[1600]{};std::snprintf(detail,sizeof(detail),format,args...);
        std::snprintf(line,sizeof(line),"[GemBag/Admission] %s",detail);++failures;
        if(context)context->LogWarn(line);return false;
    }
    bool File(const std::filesystem::path& path,const char* label,const char* expected,DWORD sharing=FILE_SHARE_READ) noexcept {
        const auto result=ReadHash(path,sharing);
        if(!result.ok)return Fail("file=%s hash unavailable; stage=%s win32=%lu ntstatus=0x%08lX expected_sha256=%s",
            label,result.stage,result.error,static_cast<unsigned long>(result.cryptoStatus),expected);
        const auto actual=Hex<32>(result.digest.data());
        return !std::strcmp(actual.data(),expected)||Fail("file=%s SHA-256 mismatch; expected=%s actual=%s",label,expected,actual.data());
    }
    bool Module(HMODULE module,const char* label,const unsigned char(&expected)[32],std::filesystem::path* foundPath=nullptr) {
        if(!module)return Fail("module=%s unavailable; no module handle",label);
        wchar_t path[32768]{};const auto size=GetModuleFileNameW(module,path,static_cast<DWORD>(std::size(path)));
        if(!size||size>=std::size(path))return Fail("module=%s path unavailable; stage=module-path win32=%lu",label,size?ERROR_INSUFFICIENT_BUFFER:GetLastError());
        const std::filesystem::path file(path);if(foundPath)*foundPath=file;
        const auto digest=Hex<32>(expected);return File(file,label,digest.data(),FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);
    }
    template<size_t N> bool Bytes(uintptr_t base,uintptr_t rva,const unsigned char(&expected)[N],const char* label,bool sdkCheck=false) noexcept {
        if(sdkCheck&&context&&context->CheckExpectedBytes(rva,expected,N))return true;
        std::array<unsigned char,N> actual{};SIZE_T got{};
        const bool read=ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),actual.data(),N,&got)&&got==N;
        const DWORD error=read?0:GetLastError();
        if(!read)return Fail("check=%s rva=0x%llX expected_bytes=%zu memory unavailable; win32=%lu read=%zu",
            label,static_cast<unsigned long long>(rva),N,error,static_cast<size_t>(got));
        size_t offset{};while(offset<N&&actual[offset]==expected[offset])++offset;
        if(offset==N)return !sdkCheck||Fail("check=%s rva=0x%llX SDK expected-byte check refused although mapped bytes match; guard retained",
            label,static_cast<unsigned long long>(rva));
        constexpr size_t preview=N<16?N:16;const size_t start=offset+preview<=N?offset:N-preview;
        const auto expectedText=Hex<preview>(expected+start),actualText=Hex<preview>(actual.data()+start);
        return Fail("check=%s rva=0x%llX byte mismatch; first_offset=0x%zX preview_offset=0x%zX expected=%s actual=%s",
            label,static_cast<unsigned long long>(rva),offset,start,expectedText.data(),actualText.data());
    }
    unsigned Failures() const noexcept{return failures;}
private:
    const D2RL::PluginContext* context{};unsigned failures{};
};
}
