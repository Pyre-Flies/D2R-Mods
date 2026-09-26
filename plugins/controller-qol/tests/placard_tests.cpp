#include "placard_text.h"
#include "placard_call.h"
#include <cstdio>
#include <cwchar>
#include <cstdint>
#define CHECK(x) do {if(!(x)){std::printf("Failure line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
    using QolPlacardText::Lease;
    char label[128]="\xff" "c4Rare Sword [Missing]";
    char baseline[128];std::memcpy(baseline,label,sizeof(label));
    Lease lease;
    CHECK(lease.Apply(label,128,"[X] ",4));
    CHECK(std::strcmp(label,"[X] \xff" "c4Rare Sword [Missing]")==0);
    CHECK(lease.Apply(label,128,"[A] ",4)); // replaces only our own previous hint
    CHECK(std::strcmp(label,"[A] \xff" "c4Rare Sword [Missing]")==0);
    CHECK(lease.Restore(label,128) && std::strcmp(label,baseline)==0);
    // Foreign text that resembles a QOL hint is not stripped.
    std::strcpy(label,"[A] Chronicle text");lease={};
    CHECK(lease.Restore(label,128) && std::strcmp(label,"[A] Chronicle text")==0);
    CHECK(lease.Apply(label,128,"[X] ",4));
    CHECK(lease.Restore(label,128) && std::strcmp(label,"[A] Chronicle text")==0);
    // Another owner rebuilt the buffer; do not restore an obsolete label.
    CHECK(lease.Apply(label,128,"[B] ",4));std::strcpy(label,"New Chronicle name");
    CHECK(!lease.Restore(label,128));CHECK(std::strcmp(label,"New Chronicle name")==0);
    wchar_t wide[64]=L"\xff" L"c2Charm [Missing]";Lease w;
    CHECK(w.Apply(wide,64,L"[Y] ",4));CHECK(w.Restore(wide,sizeof(wide)));
    CHECK(std::wcscmp(wide,L"\xff" L"c2Charm [Missing]")==0);
    // Capacity includes the terminator. Never truncate somebody else's suffix.
    char tight[8]="Sword";Lease t;
    CHECK(!t.Apply(tight,8,"[X] ",4) && std::strcmp(tight,"Sword")==0);
    char noTerminator[8];std::memset(noTerminator,'z',8);
    CHECK(!t.Apply(noTerminator,8,"[A] ",4));
    char exact[10]="Sword";CHECK(t.Apply(exact,10,"[A] ",4));
    CHECK(std::strcmp(exact,"[A] Sword")==0 && t.Restore(exact,10));
    // Verify call target and relative encoding without changing the shared entry.
    int32_t displacement;std::memcpy(&displacement,QolPlacardCall::Expected+1,4);
    CHECK(static_cast<int64_t>(QolPlacardCall::Site+5)+displacement==QolPlacardCall::Target);
    unsigned char encoded[5]{};
    CHECK(QolGlyphCalls::Encode(QolPlacardCall::Site,QolPlacardCall::Target,encoded));
    CHECK(std::memcmp(encoded,QolPlacardCall::Expected,5)==0);
    CHECK(std::memcmp(QolPlacardCall::Witness+sizeof(QolPlacardCall::Witness)-5,encoded,5)==0);
    std::puts("Placard ownership, narrow/wide formatting preservation, capacity and call-target tests passed.");
}
