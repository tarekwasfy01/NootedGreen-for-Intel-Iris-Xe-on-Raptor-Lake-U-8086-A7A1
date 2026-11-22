// SPDX-License-Identifier: MIT
// HP250 measured route: DDI A uses PLL1; HDMI DDI B uses PLL0.
// Call only under donor display-workloop and clock-resource ownership locks.
#pragma once
#include <stdint.h>
namespace HPHDMI {
class Clock {
public:
 enum class Result { Ready, NoOwnership, Busy, Inaccessible, ReferenceMismatch,
                     PllConfigurationRequired, ClockWriteFailed, NotAcquired };
 template<class Io> Result acquire(Io &io,bool ownsResources) {
  if(acquired) return Result::Busy;
  if(!ownsResources) return Result::NoOwnership;
  uint32_t route=io.read(Route),ddi=io.read(0x64100),pll=io.read(0x46010);
  if(route==0xffffffffu||ddi==0xffffffffu||pll==0xffffffffu) return Result::Inaccessible;
  // Never reroute an enabled B output or share PLL0 with enabled A/C/E ports.
  if(ddi&Enable) return Result::Busy;
  constexpr uint32_t ports[]={0x64000,0x64200,0x64700,0x64800};
  constexpr uint32_t shifts[]={0,4,6,8};
  for(uint32_t n=0;n<4;++n) {
   uint32_t ctl=io.read(ports[n]);
   if(ctl==0xffffffffu)return Result::Inaccessible;
   if((ctl&Enable)&&((route>>shifts[n])&3u)==0) return Result::Busy;
  }
  if((io.read(0x51004)&(7u<<29))!=(2u<<29)) return Result::ReferenceMismatch;
  // Reuse only an already-locked PLL with the exact HP 1080p60 configuration.
  // Programming an unlocked PLL requires the donor's allocator and power API.
  if((pll&0xcc000000u)!=0xcc000000u || io.read(0x164284)!=0x001001d0u ||
     io.read(0x164288)!=0x00000e84u) return Result::PllConfigurationRequired;
  savedBits=route&Mask;
  // Linux requires two writes: select first, then ungate B.
  io.write(Route,route&~Select);
  uint32_t current=io.read(Route);
  io.write(Route,current&~Off);
  if(io.read(Route)&Mask) {
   uint32_t now=io.read(Route);io.write(Route,(now&~Mask)|savedBits);
   return Result::ClockWriteFailed;
  }
  acquired=true;return Result::Ready;
 }
 template<class Io> Result release(Io &io,bool ownsResources) {
  if(!acquired)return Result::NotAcquired;
  if(!ownsResources)return Result::NoOwnership;
  if(io.read(0x64100)&Enable)return Result::Busy; // DDI must stop before clock.
  uint32_t route=io.read(Route);
  if(route==0xffffffffu)return Result::Inaccessible;
  // Refuse to overwrite a route changed by another owner.
  if(route&Mask)return Result::Busy;
  io.write(Route,route|Off);
  route=io.read(Route);
  io.write(Route,(route&~Mask)|savedBits);
  if((io.read(Route)&Mask)!=savedBits)return Result::ClockWriteFailed;
  acquired=false;return Result::Ready;
 }
 bool ready() const {return acquired;}
private:
 static constexpr uint32_t Route=0x164280,Select=3u<<2,Off=1u<<11,
                           Mask=Select|Off,Enable=1u<<31;
 uint32_t savedBits=0;
 bool acquired=false;
};
}
