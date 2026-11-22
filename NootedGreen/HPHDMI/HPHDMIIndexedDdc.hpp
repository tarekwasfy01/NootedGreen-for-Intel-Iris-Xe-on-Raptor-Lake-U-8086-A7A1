// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
namespace HPHDMI {
// Register protocol cross-checked against Linux intel_gmbus.c index_xfer,
// read_chunk and do_gmbus_xfer. Deliberately restricted to HP DDI-B EDID.
enum class IndexedResult : uint32_t { Ready, Invalid, Busy, Nak, Timeout, Inaccessible };
template<class Io> IndexedResult indexedDdcRead(Io &io,uint32_t offset,uint8_t *out,uint32_t length) {
 if(!out||!length||length>128||offset>255||length>256-offset)return IndexedResult::Invalid;
 constexpr uint32_t reg0=0xc5100,reg1=0xc5104,reg2=0xc5108,reg3=0xc510c;
 constexpr uint32_t active=1u<<9,nak=1u<<10,ready=1u<<11,phase=1u<<14;
 constexpr uint32_t swReady=1u<<30,clear=1u<<31,stop=1u<<27;
 uint32_t status=io.read(reg2);
 if(status==0xffffffffu)return IndexedResult::Inaccessible;
 if(status&active)return IndexedResult::Busy; // do not reset a live transaction
 unsigned budget=5000; // <=50 ms of polling for the entire data/phase transfer
 auto wait=[&](uint32_t mask)->IndexedResult {
  do {
   status=io.read(reg2);
   if(status==0xffffffffu)return IndexedResult::Inaccessible;
   if(status&nak)return IndexedResult::Nak;
   if(status&mask)return IndexedResult::Ready;
   if(!budget--)break;
   io.delay(10);
  }while(true);
  return IndexedResult::Timeout;
 };
 io.write(0xc5110,0); // polling, no GMBUS interrupt requested
 io.write(reg1,clear);io.write(reg1,0);
 io.write(0xc5120,0); // exclude a stale two-byte index from a previous user
 io.write(reg0,2); // fixed VBT/Linux pin, 100 kHz
 (void)io.read(reg0);
 io.write(reg1,swReady|(1u<<25)|(1u<<26)|(length<<16)|(offset<<8)|0xa1u);
 IndexedResult result=IndexedResult::Ready;
 for(uint32_t pos=0;pos<length&&result==IndexedResult::Ready;) {
  result=wait(ready);if(result!=IndexedResult::Ready)break;
  uint32_t word=io.read(reg3);
  for(unsigned n=0;n<4&&pos<length;++n,++pos)out[pos]=static_cast<uint8_t>(word>>(8*n));
 }
 if(result==IndexedResult::Ready)result=wait(phase);
 // STOP is a separate cycle, including on error. Bound idle wait to 10 ms.
 io.write(reg1,swReady|stop);
 bool idle=false;
 for(unsigned n=0;n<=1000;++n) {
  status=io.read(reg2);
  if(status==0xffffffffu){result=IndexedResult::Inaccessible;break;}
  if(!(status&active)){idle=true;break;}
  if(n<1000)io.delay(10);
 }
 if(!idle&&result==IndexedResult::Ready)result=IndexedResult::Timeout;
 if(result!=IndexedResult::Ready&&idle){io.write(reg1,clear);io.write(reg1,0);}
 io.write(reg0,0);
 io.write(reg2,1u<<15); // release GMBUS INUSE, as in HP reference transport
 return result;
}
}
