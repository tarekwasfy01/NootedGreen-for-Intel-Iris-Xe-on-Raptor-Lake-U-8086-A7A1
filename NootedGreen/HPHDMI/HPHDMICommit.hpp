// SPDX-License-Identifier: MIT
#pragma once
#include "HPHDMIClock.hpp"
#include "HPHDMIPhy.hpp"
#include "HPHDMIOutput.hpp"
namespace HPHDMI {
// Donor adapter must reserve resources through the display driver's lifecycle.
// A boot argument, workloop pointer or register readback cannot replace reserve().
class Commit {
public:
 enum class Result : uint32_t { Ready, Busy, Ownership, Edid, Mode, Power, Clock, Phy, Plane, Output, Cleanup };
 template<class Adapter> Result enable(Adapter &io, const Output::Admission &requested) {
  if(active)return Result::Busy;
  if(requested.width!=1920||requested.height!=1080||requested.clockKHz!=148500||requested.bpc!=8)
   return Result::Mode;
  if(!io.inDisplayGate()||!io.reserveNativeHdmiB())return Result::Ownership;
  reserved=true;
  if(!io.readAndValidateLiveEdid())return unwind(io,Result::Edid);
  if(!io.acquirePowerB())return unwind(io,Result::Power);
  powered=true;
  if(clock.acquire(io,true)!=Clock::Result::Ready)return unwind(io,Result::Clock);
  clocked=true;
  // Adapter snapshots all PHY registers and restores them only while B is idle.
  if(!io.snapshotPhyB())return unwind(io,Result::Phy);
  phySaved=true;
  if(!programHdmiSignal(io,true))return unwind(io,Result::Phy);
  if(!io.nativePlaneAndDbufCommitted())return unwind(io,Result::Plane);
  Output::Admission a=requested;
  a.ownsResources=a.liveEdid=a.clockReady=a.phyReady=a.planeReady=a.dbufReady=true;
  if(output.enable(io,a)!=Output::Result::Ready)return unwind(io,Result::Output);
  active=true;return Result::Ready;
 }
 template<class Adapter> Result disable(Adapter &io) {
  if(!io.inDisplayGate())return Result::Ownership;
  if(active) {
   if(!output.disable(io,true))return Result::Cleanup;
   active=false;
  }
  return unwind(io,Result::Ready);
 }
 // Protect only registers held by this commit, never panel-A or shared PLLs.
 bool protects(uint32_t reg) const {
  return active&&(reg==0x64100||reg==0x46144||reg==0x61400||
   reg==0x61200||(reg>=0x61220&&reg<=0x6123c&&!(reg&3)));
 }
 bool running() const{return active;}
private:
 Clock clock;Output output;
 bool active=false,reserved=false,powered=false,clocked=false,phySaved=false;
 template<class Adapter> Result unwind(Adapter &io,Result reason) {
  // Do not release clock/power while an output still uses them.
  if(io.read(0x64100)&0x80000000u)return Result::Cleanup;
  if(phySaved){if(!io.restorePhyB())return Result::Cleanup;phySaved=false;}
  if(clocked){if(clock.release(io,true)!=Clock::Result::Ready)return Result::Cleanup;clocked=false;}
  if(powered){io.releasePowerB();powered=false;}
  if(reserved){io.releaseNativeHdmiB();reserved=false;}
  return reason;
 }
};
}
