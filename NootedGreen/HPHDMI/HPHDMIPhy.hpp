// SPDX-License-Identifier: MIT
// HP VBT-selected HDMI combo-PHY B signal plan, derived from Linux v6.12.
// The caller owns the PHY/power/clock locks and must keep DDI B disabled.
#pragma once
#include <stdint.h>
namespace HPHDMI {
// Io supplies read(address) and write(address,value). No built-in hardware access.
template<class Io> bool programHdmiSignal(Io &io, bool ownsResources) {
    constexpr uint32_t Base=0x6c000;
    if (!ownsResources || (io.read(0x64100)&0x80000000u)) return false;
    // Fail before writes on inaccessible lane state.
    constexpr uint32_t words[]={2,4,5,7};
    for(uint32_t lane=0;lane<4;++lane)
      for(uint32_t dw : words)
        if(io.read(Base+0x880+lane*0x100+dw*4)==0xffffffffu) return false;
    auto rmw=[&](uint32_t reg,uint32_t mask,uint32_t bits) {
        io.write(reg,(io.read(reg)&~mask)|bits);
    };
    // Native HDMI has no DP common keeper.
    io.write(Base+0x604,io.read(Base+0x804)&~(1u<<26));
    for(uint32_t lane=0;lane<4;++lane)
      rmw(Base+0x890+lane*0x100,1u<<31,(lane==1||lane==2)?1u<<31:0);
    rmw(Base+0x14,0,3); // SUS clock config, Linux step 3.
    io.write(Base+0x694,io.read(Base+0x894)&~(1u<<31));
    uint32_t value=io.read(Base+0x894);
    value &= ~((7u<<18)|(7u<<3)|(1u<<30)|(1u<<29)|(1u<<31));
    value |= (2u<<18)|(6u<<3)|(1u<<29);
    io.write(Base+0x694,value);
    for(uint32_t lane=0;lane<4;++lane) {
      const uint32_t ln=Base+0x880+lane*0x100;
      rmw(ln+8,(1u<<15)|(7u<<11)|0xffu,0x9098u); // VBT entry 0: swing A.
      rmw(ln+16,(0x3fu<<12)|(0x3fu<<6)|0x3fu,0x3fu);
      rmw(ln+28,0x7fu<<24,0x60u<<24);
    }
    io.write(Base+0x694,value|(1u<<31));
    return true;
}
}
