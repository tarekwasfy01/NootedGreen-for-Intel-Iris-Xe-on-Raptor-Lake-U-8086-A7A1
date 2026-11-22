#include "../NootedGreen/HPHDMI/HPHDMIScanout.hpp"
#include <cassert>
struct Io {
 unsigned polls=0,elapsed=0,changeAt=1;uint32_t start=0;bool absent=false;
 void delay(unsigned us){elapsed+=us;}
 uint32_t frame(){++polls;return absent?0xffffffffu:start+(polls>=changeAt);}
};
int main(){
 assert(!HPHDMI::nativeBRunning(0,0)); // measured v28 failure
 assert(HPHDMI::nativeBRunning(0xc0000000,0x90030000)); // measured Linux HDMI
 assert(!HPHDMI::nativeBRunning(0xc0000000,0x92030000)); // DP mode
 assert(!HPHDMI::nativeBRunning(0xc0000000,0x80030000)); // different DDI
 assert(!HPHDMI::nativeBRunning(0x80000000,0x90030000)); // not active
 assert(!HPHDMI::nativeBRunning(0xffffffff,0x90030000));
 uint32_t last=0;
 Io good;assert(HPHDMI::waitNativeBFrame(good,0,last)&&last==1);
 Io late;late.changeAt=250;assert(HPHDMI::waitNativeBFrame(late,0,last)&&late.elapsed==25000);
 Io stalled;stalled.changeAt=251;assert(!HPHDMI::waitNativeBFrame(stalled,0,last)&&stalled.elapsed==25000&&stalled.polls==250);
 Io absent;absent.absent=true;assert(!HPHDMI::waitNativeBFrame(absent,0,last)&&absent.polls==1);
 Io invalid;assert(!HPHDMI::waitNativeBFrame(invalid,0xffffffff,last)&&invalid.polls==0);
}
