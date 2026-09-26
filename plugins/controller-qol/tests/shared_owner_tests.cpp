#include "shared_owner_compatibility.h"
#include <array>
#include <cstdio>
int main() {
    std::array<unsigned char,sizeof(SharedOwnerBytes)> code{};
    std::memcpy(code.data(),SharedOwnerBytes,code.size());
    if(!QolShared::OwnerBodyMatches(code.data())) return 1;
    for(int i=0x5F;i<0x63;++i) code[i]^=0x55;
    if(!QolShared::OwnerBodyMatches(code.data())) return 2;
    code[0x5E]^=1;
    if(QolShared::OwnerBodyMatches(code.data())) return 3;
    code[0x5E]^=1;code[0x80]^=1;
    if(QolShared::OwnerBodyMatches(code.data())) return 4;
    if(QolShared::ValidateOwner(0)) return 5;
    std::puts("Only call relocation can vary; opcode/body changes and unreadable owner rejected.");
}
