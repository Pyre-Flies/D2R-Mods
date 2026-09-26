#include "vendor_compatibility.h"
#include <array>
int main() {
    std::array<unsigned char,sizeof(QolVendor::TransactionBytes)> code{};
    std::memcpy(code.data(),QolVendor::TransactionBytes,code.size());
    if(!QolVendor::TransactionBodyMatches(code.data())) return 1;
    for(auto offset:QolVendor::RelocatedCalls) for(size_t i=1;i<=4;++i) code[offset+i]^=0x5A;
    if(!QolVendor::TransactionBodyMatches(code.data())) return 2;
    code[0x218]^=1;
    if(QolVendor::TransactionBodyMatches(code.data())) return 3;
    code[0x218]^=1;code[0x150]^=1;
    if(QolVendor::TransactionBodyMatches(code.data())) return 4;
    if(QolVendor::ValidateTransaction(0)) return 5;
}
