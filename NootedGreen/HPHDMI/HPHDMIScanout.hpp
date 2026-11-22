// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
namespace HPHDMI {
inline bool nativeBRunning(uint32_t pipe,uint32_t transcoder) {
 if(pipe==0xffffffffu||transcoder==0xffffffffu)return false;
 // Enabled + active pipe; enabled transcoder, DDI B, HDMI mode.
 return (pipe&0xc0000000u)==0xc0000000u &&
        (transcoder&0xf7000000u)==0x90000000u;
}
template<class Io> bool waitNativeBFrame(Io &io,uint32_t first,uint32_t &last) {
 last=first;
 if(first==0xffffffffu)return false;
 // 1080p60 is the only admitted test. Observe <=25 ms, never synthesize VBL.
 for(unsigned n=0;n<250;++n) {
  io.delay(100);last=io.frame();
  if(last==0xffffffffu)return false;
  if(last!=first)return true;
 }
 return false;
}
}
