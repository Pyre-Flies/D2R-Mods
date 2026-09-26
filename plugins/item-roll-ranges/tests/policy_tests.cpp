#include "policy.h"
#include <cstdio>
#include <cstdlib>
void check(bool value) { if (!value) { std::fputs("Policy failure\n", stderr); std::exit(1); } }
int main() {
    using namespace RollRanges;
    for (unsigned mask=0; mask<16; ++mask) {
        Panels p{(mask&1)!=0,(mask&2)!=0,(mask&4)!=0,(mask&8)!=0};
        check(Allowed(true,p)==(mask!=0)); check(!Allowed(false,p));
        for (unsigned original=0;original<16;++original) {
            check(Keyboard(original,false,false,true)==original);
            check(Keyboard(original,true,false,true)==(original&~2u));
            check(Keyboard(original,true,Allowed(true,p),true)==((original&~2u)|(mask?2u:0u)));
            check(Keyboard(original,true,true,false)==(original&~2u));
        }
    }
    check(Controller(0x12345678,false,false,false,false)==0x12345678);
    for (unsigned bits=0;bits<8;++bits)
        check(Controller(1,true,(bits&1)!=0,(bits&2)!=0,(bits&4)!=0)==(bits==7));
    // Release, disconnect, focus loss and closing a panel must remove ranges.
    check(Controller(1,true,true,true,true)==1);
    check(Controller(1,true,true,true,false)==0);
    check(Controller(1,true,true,false,true)==0);
    check(Controller(1,true,false,true,true)==0);
    std::puts("Passed keyboard/controller gating, all panel combinations, release/disconnect and unrelated callers.");
}
