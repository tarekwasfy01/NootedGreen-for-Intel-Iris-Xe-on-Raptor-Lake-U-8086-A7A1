// SPDX-License-Identifier: MIT
#pragma once
#include "HPHDMISink.hpp"
#include "HPHDMIDDC.hpp"
#include "HPHDMIPhy.hpp"
#include "HPHDMIClock.hpp"
#include "HPHDMIOutput.hpp"
#include "HPHDMICommit.hpp"
namespace HPHDMI {
enum class Blocker : uint32_t {
 None=0, MissingDonorOwnership=1, EdidUnavailable=2, UnsupportedMode=3,
 MissingPowerIntegration=4, MissingModeCommit=5
};
// Runtime capability contract, independent of OpenCore boot arguments.
// A boot argument must never be treated as evidence of hardware ownership.
struct Capabilities {
 bool donorOwnership=false, edid=false, power=false, modeCommit=false;
 uint32_t width=0,height=0,clockKHz=0,bpc=0;
 Blocker blocker() const {
  if(!donorOwnership)return Blocker::MissingDonorOwnership;
  if(!edid)return Blocker::EdidUnavailable;
  if(width!=1920||height!=1080||clockKHz!=148500||bpc!=8)
   return Blocker::UnsupportedMode;
  if(!power)return Blocker::MissingPowerIntegration;
  if(!modeCommit)return Blocker::MissingModeCommit;
  return Blocker::None;
 }
};
}
