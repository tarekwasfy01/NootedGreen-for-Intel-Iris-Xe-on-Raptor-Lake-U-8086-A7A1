// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
namespace HPHDMI {
// Exact donor's DDI-B clock routine names PLL1. Translate that routine only;
// never translate the DDI number or the panel's clock calls.
inline unsigned long pll0Register(unsigned long reg) {
 switch(reg) {
  case 0x46014:return 0x46010;
  case 0x16428c:return 0x164284;
  case 0x164290:return 0x164288;
  default:return reg;
 }
}
inline uint32_t pll0RouteWrite(uint32_t requested,uint32_t live) {
 // Own only B's PLL selector bits 3:2 and clock-gate bit11.
 constexpr uint32_t mask=(3u<<2)|(1u<<11);
 return (live&~mask)|(requested&(1u<<11));
}
inline bool pll0Admission(uint32_t route,uint32_t a,uint32_t b,uint32_t p0,uint32_t p1,uint32_t dssm) {
 if(route==0xffffffffu||a==0xffffffffu||b==0xffffffffu||p0==0xffffffffu||p1==0xffffffffu||dssm==0xffffffffu)return false;
 return (a&0x80000000u)&&!(route&(1u<<10))&&(route&3u)==1u&&
        !(b&0x80000000u)&&!(p0&0x80000000u)&&
        (p1&0xc0000000u)==0xc0000000u&&((dssm>>29)&7u)==2u;
}
inline bool pll0Reuse(bool owned,uint32_t route,uint32_t a,uint32_t b,uint32_t p0,uint32_t p1,uint32_t dssm) {
 if(!owned||route==0xffffffffu||a==0xffffffffu||b==0xffffffffu||p0==0xffffffffu||p1==0xffffffffu||dssm==0xffffffffu)return false;
 return (a&0x80000000u)&&!(route&(1u<<10))&&(route&3u)==1u&&
        (b&0x80000000u)&&!(route&(1u<<11))&&((route>>2)&3u)==0u&&
        (p0&0xc0000000u)==0xc0000000u&&(p1&0xc0000000u)==0xc0000000u&&
        ((dssm>>29)&7u)==2u;
}

}
