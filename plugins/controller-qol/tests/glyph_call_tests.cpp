#include "glyph_calls.h"
#include "pickup_calls.h"
#include "glyph_policy.h"
#include "header_policy.h"
#include "range_input_policy.h"
#include <cstdlib>
#include <cstdio>
void check(bool b){if(!b)std::exit(1);}
int main(){
 char reorder[128]{};
 check(QolGlyphPolicy::ReorderText("\xEE\x80\x8D\xEE\x80\x81" "Move Rule Up",reorder,sizeof(reorder)));
 check(std::string_view(reorder)=="\xEE\x80\xA7\xEE\x80\x81" "Move Rule Up");
 check(!QolGlyphPolicy::ReorderText("Move Rule Up",reorder,sizeof(reorder)));
 check(!QolGlyphPolicy::ReorderText("\xEE\x80\x8D" "Long label",reorder,3));
 const std::string_view filterTabs[]={"TabLeftIndicator","ItemListTabs","LootFilterRuleDetailsPanel"};
 check(QolGlyphPolicy::Classify(filterTabs,3,true,true,false,false,false,true)==QolGlyphPolicy::Hint::SubLeft);
 check(QolGlyphPolicy::Classify(filterTabs,3,true,true)==QolGlyphPolicy::Hint::None);

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
 check(QolHeaderPolicy::Slot("\xEE\x80\x90" "Close Menu")==6);
 check(QolHeaderPolicy::Slot("\xEE\x80\x88" "Open Cube")==4);
 check(QolHeaderPolicy::Slot("\xEE\x80\x8E" "Show Ranges")==5);
 check(QolHeaderPolicy::Compact("\xEE\x80\x91" "Drop (Hold to Move to Private Stash)")!=nullptr);
 check(QolHeaderPolicy::Compact("\xEE\x80\x91" "Localized action")==nullptr);
 char lines[7][128]{};std::snprintf(lines[1],sizeof(lines[1]),"Transfer");
 check(QolHeaderPolicy::Compose(lines,false,"LB"));
 check(lines[1][0]==0 && std::string_view(lines[4]).find("Stash All")!=std::string_view::npos);
 std::snprintf(lines[1],sizeof(lines[1]),"Transfer");
 check(QolHeaderPolicy::Compose(lines,true,"LB") && lines[1][0]!=0);
 check(QolHeaderPolicy::Compose(lines,true,nullptr) && lines[4][0]==0);
 check(!QolHeaderPolicy::Compose(lines,false,nullptr) && lines[1][0]==0);
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
 // The Mod Information exception belongs to its exact tab ownership chain,
 // never to similarly named pages, other tabs, or the menu behind the shell.
 const std::string_view modInformationLeft[]={"TabLeftIndicator","OptionsTabs","modinfo_aa"};
 const std::string_view modInformationRight[]={"TabRightIndicator","OptionsTabs","modinfo_aa"};
 for(const auto* names : {modInformationLeft,modInformationRight}) {
  const auto expected=names==modInformationLeft?QolGlyphPolicy::Hint::SubLeft:QolGlyphPolicy::Hint::SubRight;
  check(QolGlyphPolicy::Classify(names,3,false,true,false,false,false,false,true)==expected);
  check(QolGlyphPolicy::Classify(names,3,true,true,false,false,false,false,false)==QolGlyphPolicy::Hint::None);
  check(QolGlyphPolicy::Classify(names,3,true,false,false,false,false,false,true)==QolGlyphPolicy::Hint::None);
  check(QolGlyphPolicy::Classify(names,2,true,true,false,false,false,false,true)==QolGlyphPolicy::Hint::None);
 }
 const std::string_view modInformationText[]={"Text","TabLeftIndicator","OptionsTabs","modinfo_aa","Root"};
 check(QolGlyphPolicy::Classify(modInformationText,5,false,true,false,false,false,false,true)==QolGlyphPolicy::Hint::SubLeft);
 const std::string_view unrelatedModInformation[][4]={
  {"TabLeftIndicator","OtherTabs","modinfo_aa","Root"},
  {"TabRightIndicator","OptionsTabs","modinfo_generalinformation","Root"},
  {"TabLeftIndicator","OptionsTabs","modinfo_aaa","Root"},
  {"TabRightIndicator","OptionsTabs","Modinfo_aa","Root"},
  {"TabLeftIndicator","modinfo_aa","OptionsTabs","Root"},
  {"TabRightIndicator","OptionsTabs","OtherPanel","modinfo_aa"},
  {"TabLeftIndicator","OtherWidget","OptionsTabs","modinfo_aa"},
  {"CycleLeftIndicator","OptionsTabs","modinfo_aa","Root"},
  {"CycleRightIndicator","OptionsTabs","modinfo_aa","Root"},
  {"OtherIndicator","OptionsTabs","modinfo_aa","Root"}
 };
 for(const auto& names : unrelatedModInformation)
  check(QolGlyphPolicy::Classify(names,4,true,true,false,false,false,false,true)==QolGlyphPolicy::Hint::None);
 check(QolGlyphPolicy::Classify(nullptr,0,true,true,false,false,false,false,true)==QolGlyphPolicy::Hint::None);
 check(QolGlyphPolicy::Classify(options,3,true,true,false,true,false,false,true)==QolGlyphPolicy::Hint::SubLeft);
 check(QolGlyphPolicy::Classify(filterTabs,3,true,true,false,false,false,true,true)==QolGlyphPolicy::Hint::SubLeft);
 check(QolGlyphPolicy::Classify(chronicleRight,3,true,true,false,false,true,false,true)==QolGlyphPolicy::Hint::SubRight);
 const std::string_view shared[]={"TabLeftIndicator","StashRightArrow","BankExpansionLayout"};
 check(QolGlyphPolicy::Classify(shared,3,true,true,true,false,false,false,true)==QolGlyphPolicy::Hint::SharedRight);
 unsigned char bytes[5]{};
 for(unsigned i=0;i<2;++i){
  check(QolGlyphCalls::Encode(0x140000000+QolGlyphCalls::Sites[i],0x140902e20,bytes));
  check(!std::memcmp(bytes,QolGlyphCalls::Expected[i],5));
  check(QolGlyphCalls::Encode(0x140000000+QolGlyphCalls::Sites[i],0x130000000,bytes));
  int32_t displacement{};std::memcpy(&displacement,bytes+1,4);
  check(0x140000000+QolGlyphCalls::Sites[i]+5+displacement==0x130000000);
 }
 check(!QolGlyphCalls::Encode(0x140000000,0x7ff000000000,bytes));
 for(const auto& call:QolPickupCalls::Calls){
  int32_t displacement{};std::memcpy(&displacement,call.expected+1,4);
  check(call.expected[0]==0xe8);
  check(0x140000000+call.rva+5+displacement==0x140000000+QolPickupCalls::Target);
 }
 check(QolGlyphPolicy::Text(QolGlyphPolicy::Hint::SharedLeft)!=nullptr);
 check(QolGlyphPolicy::Text(QolGlyphPolicy::Hint::None)==nullptr);
 std::puts("Passed both native call targets, signed relay relocation, overflow refusal and scoped prompt policy.");
}
