#include "cube_controls_policy.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
using namespace QolCubeControls;
unsigned checks{};
void Check(bool ok,const char* name){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}
Frame Ready(uint64_t now,uint16_t keys=0) {return {now,keys,false,true,true,true,true,false,false,{1,7,42,2,3}};}
int main() {
    for(bool view:{false,true})for(bool carried:{false,true})for(bool admitted:{false,true}) {
        const bool available=TransmuteAvailable(view,carried,admitted);
        Gesture g;auto f=Ready(100,static_cast<uint16_t>(B|0x100));f.modifier=true;f.cubeView=available;
        Check((g.Update(f).action==Action::Transmute)==(view || (carried && admitted)),"open Cube or admitted highlighted Cube can transmute");
        Check(g.Update(f).action==Action::None,"highlighted transmute never repeats while B is held");
    }
    for(bool held:{false,true})for(bool eligible:{false,true})for(bool matches:{false,true})
        for(bool modifier:{false,true})for(bool busy:{false,true}) {
            const auto expected=eligible && matches && !modifier && !busy?(held?Action::PickUp:Action::Open):Action::None;
            Check(NativeAction(held,eligible,matches,modifier,busy)==expected,"native route preserves other items, selection, modifier and competing operations");
        }
    Check(CursorAvailable(false,7),"empty controller cursor mode 7 permits Cube controls");
    for(int mode=-2;mode<=16;++mode) {
        Check(CursorAvailable(false,mode)==(mode==0 || mode==7),"only admitted empty cursor modes permit Cube controls");
        Check(!CursorAvailable(true,mode),"occupied cursor always refuses Cube controls");
    }
    for(uint64_t duration:{uint64_t{0},uint64_t{1},HoldMs-1,HoldMs,HoldMs+1,uint64_t{5000}}) {
        Gesture g;auto f=Ready(100,A);
        Check(g.Update(f).consumed==A,"Cube A press withheld");
        f.now+=duration;f.buttons=0;const auto release=g.Update(f);
        Check(release.action==(duration<HoldMs?Action::Open:Action::PickUp),"tap/hold threshold selects exactly one action");
        Check(g.Update(f).action==Action::None,"release never repeats action");
    }
    {
        Gesture g;g.Update(Ready(100,A));auto f=Ready(100+HoldMs,A);
        Check(g.Update(f).action==Action::PickUp,"long hold picks up before release");
        for(unsigned n=0;n<100;++n) {f.now+=16;Check(g.Update(f).action==Action::None,"continued hold never repeats pickup");}
        f.buttons=0;Check(g.Update(f).action==Action::None,"long hold release does not open Cube");
    }
    for(unsigned change=0;change<11;++change) {
        Gesture g;g.Update(Ready(100,A));auto f=Ready(200,A);
        switch(change) {
        case 0:f.focus=false;break;
        case 1:++f.identity.item;break;
        case 2:++f.identity.player;break;
        case 3:++f.identity.session;break;
        case 4:++f.identity.x;break;
        case 5:++f.identity.y;break;
        case 6:f.modifier=true;break;
        case 7:f.busy=true;break;
        case 8:f.ready=false;break;
        case 9:f.swap=false;break;
        default:f.buttons|=1;break;
        }
        Check(g.Update(f).consumed==A,"cancelled gesture owns A until release");
        Check(g.Update(Ready(800,A)).action==Action::None,"restoring focus/context cannot resume cancelled hold");
        Check(g.Update(Ready(900)).action==Action::None,"cancelled release cannot open or pick up");
        Check(g.Update(Ready(1000,A)).consumed==A,"fresh press can arm after release");
        Check(g.Update(Ready(1100)).action==Action::Open,"fresh tap succeeds after cancellation");
    }
    for(unsigned change=0;change<7;++change) {
        Gesture g;auto f=Ready(100,A);
        switch(change) {
        case 0:f.focus=false;break;case 1:f.modifier=true;break;
        case 2:f.busy=true;break;case 3:f.ready=false;break;
        case 4:f.swap=false;break;case 5:f.buttons|=B;break;default:f.buttons|=0x80;break;
        }
        Check(g.Update(f).consumed==0,"ineligible or chorded A preserves original input");
        Check(g.Update(Ready(800,A)).action==Action::None,"held A cannot arm after context changes");
    }
    {
        Gesture g;auto f=Ready(100,B);f.modifier=true;f.cubeView=true;
        Check(g.Update(f).action==Action::Transmute,"modifier+B transmutes in open Cube");
        for(unsigned n=0;n<100;++n) {f.now+=16;Check(g.Update(f).action==Action::None,"held B cannot repeat transmute");}
        f.modifier=false;f.cubeView=false;Check(g.Update(f).consumed==B,"owned B stays suppressed after modifier/panel release");
        f.buttons=0;g.Update(f);f.buttons=B;
        Check(g.Update(f).consumed==0,"bare B returns to native input after release");
    }
    for(unsigned change=0;change<5;++change) {
        Gesture g;auto f=Ready(100,B);f.modifier=true;f.cubeView=true;
        switch(change) {case 0:f.modifier=false;break;case 1:f.cubeView=false;break;case 2:f.ready=false;break;case 3:f.busy=true;break;default:f.transmute=false;break;}
        Check(g.Update(f).action==Action::None,"transmute requires all context guards");
    }
    for(const uint16_t extra:{uint16_t{1},uint16_t{0x40},uint16_t{0x80},uint16_t{0x200},uint16_t{0x400},uint16_t{0x800},uint16_t{0x4000},uint16_t{0x8000}}) {
        Gesture g;auto f=Ready(100,static_cast<uint16_t>(B|0x100|extra));f.modifier=true;f.cubeView=true;
        Check(g.Update(f).action==Action::None,"other buttons/triggers prevent transmute chord collision");
    }
    for(const uint16_t modifier:{uint16_t{0x100},uint16_t{0x40},uint16_t{0x80},uint16_t{0x400},uint16_t{0x500},uint16_t{0}}) {
        Gesture g;auto f=Ready(100,static_cast<uint16_t>(B|modifier));f.modifier=true;f.cubeView=true;f.modifierButtons=modifier;
        Check(g.Update(f).action==Action::Transmute,"configured modifier's own buttons are accepted");
    }
    {
        Gesture g;auto f=Ready(100,B);f.cubeView=true;g.Update(f);f.modifier=true;
        Check(g.Update(f).action==Action::None,"adding modifier to an already-held B cannot transmute");
        g={};g.Update(Ready(100,A));g.Cancel();Check(g.Update(Ready(800,A)).action==Action::None,"native reset cancels owned hold");
        g={};g.Update(Ready(100,A));Check(g.Update(Ready(99)).action==Action::None,"clock reversal cancels tap");
    }
    for(bool occupied:{false,true})for(int mode=-2;mode<=16;++mode)
        for(auto use:{CursorUse::Idle,CursorUse::TrackHold,CursorUse::CompleteHold,CursorUse::Description})for(bool armed:{false,true}) {
            const bool expected=!occupied && (mode==0 || mode==7 || (armed && ((mode==9 && (use==CursorUse::TrackHold || use==CursorUse::CompleteHold)) || (mode==8 && use==CursorUse::TrackHold))));
            Check(NativeCursorAvailable(occupied,mode,use,armed)==expected,"hold modes never broaden idle, targeting or occupied cursor admission");
            const bool description=!occupied && (mode==0 || mode==7 || mode==8 || mode==9);
            Check(ResolveCursorAvailable(occupied,mode,use,armed)==(use==CursorUse::Description?description:expected),"caption-only charging/completion admission cannot broaden native actions");
        }
    for(int mode:{7,8,9,7}) {
        Check(ResolveCursorAvailable(false,mode,CursorUse::Description,false),"caption remains corrected throughout a native tap or hold transition");
        Check(ResolveCursorAvailable(false,mode,CursorUse::Idle,false)==(mode==7),"display continuity never admits unowned pickup or transmute");
        const bool described=ResolveCursorAvailable(false,mode,CursorUse::Description,false);
        const bool actionable=ResolveCursorAvailable(false,mode,CursorUse::Idle,false);
        Check(CaptionFlags(described,true,true,false,true)==3,"primary wording and transmute row stay present through the same native tap/hold");
        Gesture g;auto f=Ready(100,static_cast<uint16_t>(B|0x100));f.modifier=true;f.cubeView=actionable;
        Check((g.Update(f).action==Action::Transmute)==(mode==7),"visible transmute caption cannot authorize transmute during native A animation");
    }
    for(bool described:{false,true})for(bool swap:{false,true})for(bool transmute:{false,true})
        for(bool view:{false,true})for(bool hover:{false,true}) {
            const auto flags=CaptionFlags(described,swap,transmute,view,hover);
            Check(static_cast<bool>(flags&1)==(described && swap),"primary header honors Cube context and swap setting");
            Check(static_cast<bool>(flags&2)==(transmute && (view || (described && hover))),"transmute header honors open view or qualified carried Cube and setting");
        }
    for(bool occupied:{false,true})for(int mode=-2;mode<=16;++mode) {
        const bool described=ResolveCursorAvailable(occupied,mode,CursorUse::Description,false);
        Check((CaptionFlags(described,true,true,false,true)!=0)==(!occupied && (mode==0 || mode==7 || mode==8 || mode==9)),"special cursor or vanished Cube removes both carried-Cube captions together");
    }
    {
        NativeHold h;const auto id=Ready(1).identity;
        h.Update(A,true,true,id);Check(h.armed && h.Matches(id),"bare A on idle carried Cube captures identity");
        h.Update(A,true,true,id);Check(h.armed,"charging preserves admitted press");
        h.Complete();Check(h.completed && !h.armed && h.Matches(id),"native completion is recorded before submission");
        h.Update(A,true,true,id);Check(h.completed && !h.armed,"continuing hold cannot re-arm or repeat");
        h.Update(0,true,true,id);Check(!h.Matches(id),"release forgets native completion");
        h.Update(A,true,true,id);Check(h.armed,"fresh A can start another native hold");
    }
    for(unsigned change=0;change<10;++change) {
        NativeHold h;const auto id=Ready(1).identity;auto next=id;
        h.Update(A,true,true,id);bool eligible=true,focus=true;uint16_t keys=A;
        switch(change) {
        case 0:++next.item;break;case 1:++next.player;break;case 2:++next.session;break;
        case 3:++next.x;break;case 4:++next.y;break;case 5:eligible=false;break;
        case 6:focus=false;break;case 7:keys|=B;break;case 8:h.Cancel();eligible=false;break;default:keys=0;break;
        }
        h.Update(keys,eligible,focus,next);Check(!h.Matches(id),"context changes invalidate native hold identity");
        if(keys&A) {h.Update(A,true,true,id);Check(!h.armed,"restoring context cannot resurrect cancelled native hold");}
    }
    {
        HoverRequest request;auto id=Ready(1).identity;request.Arm(100,id);
        Check(request.Matches(100,id) && request.Matches(5100,id),"owned hover intent survives bounded companion replay");
        Check(!request.Matches(99,id) && !request.Matches(5101,id),"clock reversal and expiry refuse hover replay");
        for(unsigned field=0;field<5;++field) {
            auto other=id;
            switch(field) {case 0:++other.session;break;case 1:++other.player;break;case 2:++other.item;break;case 3:++other.x;break;default:++other.y;break;}
            Check(!request.Matches(101,other),"hover replay requires original session owner Cube and cell");
        }
        Check(request.Begin(101,id),"owned hover event begins native submission");
        Check(!request.Matches(101,id) && !request.Begin(101,id),"nested event cannot duplicate in-flight native submission");
        request.End();Check(request.Begin(102,id),"distinct companion Convert event can finish its recipe");
        request.End();Check(!request.Begin(5101,id),"companion replay cannot extend original deadline");
        request.Cancel();Check(!request.Matches(101,id),"cancelled intent cannot submit another event");
    }
    // Sweep polling and press lengths: exactly one native action, never both.
    for(uint64_t duration=1;duration<1500;duration+=17) {
        Gesture g;unsigned opens{},pickups{};
        for(uint64_t elapsed=0;elapsed<=duration+32;elapsed+=16) {
            const auto d=g.Update(Ready(100+elapsed,elapsed<duration?A:0));
            opens+=d.action==Action::Open;pickups+=d.action==Action::PickUp;
        }
        Check(opens+pickups==1,"poll timing produces exactly one action");
        Check(opens==0 || pickups==0,"poll timing never opens and picks up together");
    }
    std::printf("%u Cube control checks passed\n",checks);
}
