#include "glyph_calls.h"
#include "glyph_policy.h"
#include "header_policy.h"
#include "range_input_policy.h"
#include <cstdlib>
#include <cstdio>
void check(bool b){if(!b)std::exit(1);}
int main(){
 check(QolRangePolicy::Mask(true,0x81995a,0x81995a,0,0x800)==0x200);
 check(QolRangePolicy::Mask(false,0x81995a,0x81995a,0,0x800)==0x800);
 check(QolRangePolicy::Mask(true,0x81995a,0x81995a,8,0x800)==0x800);
 check(QolRangePolicy::Mask(true,0x81995a,0x81995a,0,0x100)==0x100);
 // IRR's scoped adapter handles its own query without forwarding. Its other
 // calls, native input sampling and unrelated consumers retain their masks.
 check(QolRangePolicy::Mask(true,0x1234,0x81995a,0,0x800)==0x800);
 check(QolHeaderPolicy::RangeLabel("\xEE\x80\x8E" "Show Ranges"));
 check(!QolHeaderPolicy::RangeLabel("\xEE\x80\x8E" "Other Action"));

 const std::string_view header[]={"Text","Legend","legendBG","Anchor","ControllerOverlay"};
 check(QolHeaderPolicy::Scope(header,5));
 check(!QolHeaderPolicy::Scope(header+1,4));
 const std::string_view unrelated[]={"Text","Legend","legendBG","Anchor","OtherPlugin"};
 check(!QolHeaderPolicy::Scope(unrelated,5));
 check(QolHeaderPolicy::Slot("\xEE\x80\x91" "Drop")==1);
 check(QolHeaderPolicy::Slot("\xEE\x80\x90" "Close Menu")==-1);
 check(QolHeaderPolicy::Fresh(1000,900));check(!QolHeaderPolicy::Fresh(1000,749));
 check(!QolHeaderPolicy::Fresh(10,100));check(!QolHeaderPolicy::Fresh(10,0));
 check(std::string_view(QolHeaderPolicy::Modifier("LB"))=="\xEE\x80\xA7");

 const std::string_view chronicle[]={"TabLeftIndicator","ChronicleTabs","ChroniclePanel"};
 if(QolGlyphPolicy::Classify(chronicle,3,true,true,false,false,true)!=QolGlyphPolicy::Hint::SubLeft) return 40;
 if(QolGlyphPolicy::Classify(chronicle,3,true,true,false,false,false)!=QolGlyphPolicy::Hint::None) return 41;
 const std::string_view chronicleRight[]={"TabRightIndicator","ChronicleTabs","ChroniclePanel"};
 if(QolGlyphPolicy::Classify(chronicleRight,3,true,true,false,false,true)!=QolGlyphPolicy::Hint::SubRight) return 42;
 const std::string_view options[]={"TabLeftIndicator","OptionsTabs","SettingsPanel"};
 check(QolGlyphPolicy::Classify(options,3,true,true,false,true)==QolGlyphPolicy::Hint::SubLeft);
 check(QolGlyphPolicy::Classify(options,3,true,true,false,false)==QolGlyphPolicy::Hint::None);
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
