#pragma once
#include <stdint.h>
namespace HPHDMI {
// IOGraphicsTypes.h bit masks, not literal bit depths. Zero requests automatic
// selection; preserve it during enumeration, but reject explicit YCbCr/HDR/6/10bpc.
inline bool rgb8Request(uint16_t encoding,uint16_t bpc,uint16_t range) {
 return (encoding==0u||encoding==1u)&&(bpc==0u||bpc==2u)&&(range==0u||range==1u);
}
}
