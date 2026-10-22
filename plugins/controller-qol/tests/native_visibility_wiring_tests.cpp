#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include "policy.h"
#include "client_loot_policy.h"
static void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static std::string Source(const char* name){std::ifstream f(std::filesystem::path(__FILE__).parent_path().parent_path()/"src"/name);Check(bool(f),"source fixture readable");return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 const auto source=Source("plugin_main.cpp");
 auto a=source.find("static bool BlockClientPickup(");auto z=source.find("static bool ProjectGuidedArrowPacket(",a);auto client=source.substr(a,z-a);
 Check(client.find("const auto settingsSnapshot=ReadSettings();")<client.find("!settingsSnapshot.blockFilteredPickup") &&
     client.find("!settingsSnapshot.blockFilteredPickup")<client.find("QolClientLoot::HiddenForPickup(guid)"),"current client setting checked before provider");
 Check(client.find("!ControllerQoL::IsControllerUiActive()")<client.find("QolClientLoot::HiddenForPickup(guid)"),"client controller scope retained");
 Check(client.find("QolClientLoot::IsGoldCode(code)")<client.find("QolClientLoot::HiddenForPickup(guid)"),"client gold exemption retained");
 Check(client.find("const bool blocked=!PlacardOverlay::IsPlacardActive(guid) ||\n            QolClientLoot::HiddenForPickup(guid);")!=std::string::npos,"client combines QOL and provider filtering");
 a=source.find("__declspec(noinline) bool __fastcall HookPickup(");z=source.find("static bool InstallPickupCalls(",a);auto native=source.substr(a,z-a);
 Check(native.find("!LocalActor(player,actorId)")<native.find("QolClientLoot::HiddenForPickup(guid)"),"remote actor forwarding retained");
 Check(native.find("TakeNativePickupPermit(actorId,guid)")<native.find("QolClientLoot::HiddenForPickup(guid)"),"one-use native completion permits retained");
 Check(native.find("(PlacardOverlay::IsPlacardActive(guid) && !QolClientLoot::CachedHiddenForPickup(guid))")!=std::string::npos,"native path uses copied provider visibility");
 Check(native.find("QolGoldPickup::IsGold(g_PluginContext,player,guid)")<native.rfind("return false;"),"native gold exemption retained");
 const auto loot=Source("client_loot.cpp");a=loot.find("bool HiddenForPickup(");z=loot.find("bool Ready()",a);auto query=loot.substr(a,z-a);
 Check(query.find("!Validate() || !ReadPickupCode(guid,code)")!=std::string::npos,"native profile and live identity required");
 Check(query.find("!HiddenCandidate(Candidate{guid,code})")!=std::string::npos,"same guarded visibility service reused");
 a=loot.find("bool CachedHiddenForPickup(");z=loot.find("bool Ready()",a);auto cached=loot.substr(a,z-a);
 Check(cached.find("ReadPickupCode")==std::string::npos && cached.find("HiddenCandidate")==std::string::npos && cached.find("Validate()")==std::string::npos,"native server guard never calls UI/native visibility readers");
 Check(cached.find("found->second.Current(localSession.generation,GetTickCount64())")!=std::string::npos,"copied proof is session-scoped and recent");
 Check(loot.find("else (void)HiddenForPickup(guid);")!=std::string::npos,"potentially hidden UI labels retain complete proof refresh");
 Check(loot.find("QolLootVisibility::CannotHideLabel(context,labelUnit,guid)")!=std::string::npos,"only provider no-Hide proof avoids native query");
 Check(source.find("QolClientLoot::Track(guid,true,unit);")!=std::string::npos,"only synchronous qualified label item passed to preflight");
 QolClientLoot::VisibilityProof proof{7,1000,123};
 Check(proof.Current(7,1000) && proof.Current(7,1250),"fresh proof accepted");
 Check(!proof.Current(7,1251) && !proof.Current(8,1001) && !proof.Current(7,999),"expired, old-session and future proof fail open");
 // The existing policy must block unmodified A on a hidden placard, allow
 // visible items, and retain modifier behavior independently of this bridge.
 Check(Probe::BlockNativePickup(false,false,true,false),"hidden A blocked with shortcuts disabled");
 Check(Probe::BlockNativePickup(true,false,true,false),"hidden A blocked with shortcuts enabled");
 Check(!Probe::BlockNativePickup(true,false,true,true),"visible A allowed");
 Check(!Probe::BlockNativePickup(false,false,false,false),"disabled filter does not block A");
 Check(Probe::BlockNativePickup(true,true,false,true),"held shortcut policy retained");
 std::puts("Native/client visibility wiring, settings, local actor, gold, completion permit and unmodified A policies verified.");
}
