// SPDX-License-Identifier: MIT
// Apple framebuffer ConnectorEntry for the HP's built-in HDMI socket.
// The successful TGL DDI-B recipe uses HDMI type 0x800 and only the HDMI
// capability flag 0x2. Do not add the DP/MST-like 0x200 flag here.
#pragma once
#include <stdint.h>
namespace HPHDMI {
struct NativePortConfig {
 uint32_t ddi,ddc,type,typeC,connector,flags;
};
static_assert(sizeof(NativePortConfig)==24,"TGL PortConfig layout");
inline NativePortConfig nativeHdmiB() {return {1,2,2,0,0x800,0x2};}
inline bool isNativeHdmiB(const NativePortConfig &c) {
 return c.ddi==1&&c.ddc==2&&c.type==2&&c.typeC==0&&c.connector==0x800&&c.flags==0x2;
}
}
