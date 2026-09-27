#include "belt_compatibility.h"
#include "belt_signatures.h"
#include <array>
#include <cstring>
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}}
int main(){
    using namespace QolBelt::Signatures;
    std::array<unsigned char,1024> memory{};
    auto reset=[&]{std::memcpy(memory.data(),PlaceStored,sizeof(PlaceStored));};
    auto address=reinterpret_cast<uintptr_t>(memory.data());
    reset();Check(QolBeltCompat::Stored(address),"original function works without Auto Belt Refill");
    memory[100]^=1;Check(!QolBeltCompat::Stored(address),"changed original body rejected");
    reset();memory[0]=0xe9;int32_t relative=700-5;std::memcpy(memory.data()+1,&relative,4);
    Check(!QolBeltCompat::Stored(address),"unrecognized relay rejected");
    memory[700]=0xff;memory[701]=0x25;uintptr_t target=reinterpret_cast<uintptr_t>(&main);std::memcpy(memory.data()+706,&target,8);
    Check(!QolBeltCompat::Stored(address),"unknown plugin destination rejected");
    Check(!QolBeltCompat::Stored(0),"unavailable native memory fails closed");
    reset();Check(QolBeltCompat::Match(address,PlaceStored,sizeof(PlaceStored)),"quiet read-only matching succeeds");
    memory[10]^=1;Check(!QolBeltCompat::Match(address,PlaceStored,sizeof(PlaceStored)),"quiet matching rejects changes");
    std::puts("Belt original/unknown-hook admission tests passed");
}
