#include "glyph_calls.h"
#include "glyph_policy.h"
#include <cstdlib>
#include <cstdio>
void check(bool b){if(!b)std::exit(1);}
int main(){
 unsigned char bytes[5]{};
 for(unsigned i=0;i<2;++i){
  check(QolGlyphCalls::Encode(0x140000000+QolGlyphCalls::Sites[i],0x140902e20,bytes));
  check(!std::memcmp(bytes,QolGlyphCalls::Expected[i],5));
  check(QolGlyphCalls::Encode(0x140000000+QolGlyphCalls::Sites[i],0x130000000,bytes));
  int32_t displacement{};std::memcpy(&displacement,bytes+1,4);
  check(0x140000000+QolGlyphCalls::Sites[i]+5+displacement==0x130000000);
 }
 check(!QolGlyphCalls::Encode(0x140000000,0x7ff000000000,bytes));
 check(QolGlyphPolicy::Text(QolGlyphPolicy::Hint::SharedLeft)!=nullptr);
 check(QolGlyphPolicy::Text(QolGlyphPolicy::Hint::None)==nullptr);
 std::puts("Passed both native call targets, signed relay relocation, overflow refusal and scoped prompt policy.");
}
