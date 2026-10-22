#include "native_fixture.h"
#include <cstring>

int main(int argc,char** argv) {
    Fixture::NativePath path;
    path.Flags(1,0,0);
    if(argc==2 && !std::strcmp(argv[1],"--unpatched")) {
        if(!path.Run({0.25f,3})) {
            std::fprintf(stderr,"RED: normal mode suppresses the native lighting update in the witnessed path.\n");
            return 1;
        }
        return 0;
    }
    constexpr std::array<unsigned char,2> bools{0,1};
    constexpr std::array<unsigned char,4> safetyValues{0,1,2,3};
    for(auto noDebug:bools) for(auto safety:safetyValues) for(auto overrideTime:bools) {
        path.Flags(noDebug,safety,overrideTime);
        Check(path.Run({0.25f,3})==(!noDebug && !(safety&1) && !overrideTime),"original debug-only suppression reproduced");
    }
    Check(path.patch.Install(path.PatchAddress(),path.before,path.after)==DayNight::PatchResult::Applied,"patch publication");
    for(auto noDebug:bools) for(auto safety:safetyValues) for(auto overrideTime:bools) {
        path.Flags(noDebug,safety,overrideTime);
        Check(path.Run({0.75f,4})==(!(safety&1) && !overrideTime),"normal-mode update with both independent guards preserved");
        Check(path.NoDebug()==noDebug && path.Safety()==safety,"debug and safety flags unchanged");
    }
    path.Flags(1,0,0);
    for(int period=0;period<6;++period) for(float blend:{0.0f,0.25f,0.5f,0.75f})
        Check(path.Run({blend,period}),"changing native time inputs reach the renderer");
    Check(path.patch.Stop()==DayNight::PatchResult::Restored,"original patch restoration");
    Check(DayNight::Bytes(path.PatchAddress(),path.before),"exact original bytes restored");
    path.Flags(1,0,0);
    Check(!path.Run({0.5f,2}),"unload restores original normal-mode suppression");
    std::puts("PASS: original suppression; patched native forwarding; 16 gate combinations per mode; all six periods; exact unload restoration.");
}
