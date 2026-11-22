// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
namespace HPHDMI {
// Preserve the firmware handoff for built-in FB0. For external paths, never
// report that registers are current when the qualified HDMI pipe is still off.
inline uint8_t modesetDecision(bool enabled,uint32_t fb,uint32_t ddi,uint32_t pipe,
                               uint32_t transConf,uint8_t nativeResult) {
 if(!enabled)return nativeResult;
 if(fb==0)return 0;
 // v28 evidence: native returns false with TRANS_CONF_B=0. This skips
 // LightUpTMDS, leaving B's transcoder/plane off despite a successful return.
 // True requests Apple's full modeset; it is NOT a synthetic success result.
 // Caller qualifies the already admitted HP PLL0 test before enabling this.
 if(fb==1&&ddi==1&&pipe==1&&transConf!=0xffffffffu&&!(transConf&0xc0000000u))return 1;
 return nativeResult;
}
}
