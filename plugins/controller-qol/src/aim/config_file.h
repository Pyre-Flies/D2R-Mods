#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <atomic>

namespace Aim::ConfigFile {
inline std::filesystem::path Utf8Path(std::string_view text) {return std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(text.data()),text.size()));}
inline bool Read(const std::filesystem::path& path,std::string& text,std::size_t limit,bool missingOkay=false) {
    std::error_code error;
    if(!std::filesystem::exists(path,error)) {text.clear();return missingOkay && !error;}
    const auto size=std::filesystem::file_size(path,error);if(error || size>limit)return false;
    std::ifstream file(path,std::ios::binary);if(!file)return false;
    text.resize(static_cast<std::size_t>(size));file.read(text.data(),static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(file.gcount())==size && text.find('\0')==text.npos;
}
// The destination must still equal the caller's snapshot. Existing files are
// atomically replaced with a unique recoverable backup; never truncate in place.
inline bool Write(const std::filesystem::path& path,const std::string& expected,const std::string& text,bool backup=true) {
    std::error_code error;
    const bool existed=std::filesystem::exists(path,error);if(error)return false;
    std::string current;
    if(!Read(path,current,8*1024*1024,true) || current!=expected)return false;
    if(existed && current==text)return true;
    std::filesystem::create_directories(path.parent_path());
    static std::atomic<unsigned> sequence{};
    const auto suffix=L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L"."+std::to_wstring(sequence.fetch_add(1));
    auto temporary=path;temporary+=suffix+L".tmp";
    auto saved=path;saved+=suffix+L".bak";
    const auto file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD count{};
    const bool written=text.size()<=MAXDWORD && WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&count,nullptr) && count==text.size() && FlushFileBuffers(file);
    CloseHandle(file);
    bool done=false;
    if(written && Read(path,current,8*1024*1024,true) && current==expected) {
        done=existed?ReplaceFileW(path.c_str(),temporary.c_str(),backup?saved.c_str():nullptr,0,nullptr,nullptr)!=FALSE:
            MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_WRITE_THROUGH)!=FALSE;
    }
    if(!done)DeleteFileW(temporary.c_str());
    return done;
}
// Frequent R3 saves retain only the immediately preceding document. Migration
// writes keep using unique backups; this path never prunes those recovery files.
inline bool WriteRolling(const std::filesystem::path& path,const std::string& expected,const std::string& text) {
    std::string current;
    if(!Read(path,current,8*1024*1024) || current!=expected)return false;
    if(current==text)return true;
    auto previous=path;previous+=L".r3.bak";
    std::string oldBackup;
    if(!Read(previous,oldBackup,8*1024*1024,true) || !Write(previous,oldBackup,current,false))return false;
    return Write(path,expected,text,false);
}
}
