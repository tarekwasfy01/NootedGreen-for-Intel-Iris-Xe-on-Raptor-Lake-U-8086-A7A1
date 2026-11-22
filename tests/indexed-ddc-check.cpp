#include "../NootedGreen/HPHDMI/HPHDMIIndexedDdc.hpp"
#include <array>
#include <vector>
#include <utility>
#include <cassert>
struct Io {
 std::array<uint8_t,256> bytes{};
 std::vector<std::pair<uint32_t,uint32_t>> writes;
 unsigned offset=0,count=0,readCount=0,delayed=0,pos=0;
 bool started=false,stopped=false,busy=false,missing=false,error=false,stall=false,stuckStop=false;
 uint32_t command=0;
 uint32_t read(uint32_t r) {
  ++readCount;
  if(r==0xc5100)return 2;
  if(r==0xc5108) {
   if(missing)return 0xffffffff;
   if(busy&&!started)return 1u<<9;
   if(stopped)return stuckStop?1u<<9:0;
   if(!started)return 0;
   if(error)return 1u<<10;
   if(stall)return 1u<<9;
   return pos<count?1u<<11:1u<<14;
  }
  assert(r==0xc510c&&pos<count);
  uint32_t word=0;for(unsigned n=0;n<4&&pos<count;++n,++pos)word|=uint32_t(bytes[offset+pos])<<(8*n);
  return word;
 }
 void write(uint32_t r,uint32_t v) {
  assert(r==0xc5100||r==0xc5104||r==0xc5108||r==0xc5110||r==0xc5120);
  writes.emplace_back(r,v);
  if(r==0xc5104&&(v&(1u<<30))&&(v&(1u<<26))) {
   started=true;command=v;offset=(v>>8)&255;count=(v>>16)&511;
  }
  if(r==0xc5104&&(v&(1u<<27)))stopped=true;
 }
 void delay(unsigned us){delayed+=us;}
};
int main() {
 std::array<uint8_t,128> out{};
 for(unsigned offset: {0u,16u,128u}) {
  Io io;for(unsigned n=0;n<256;++n)io.bytes[n]=uint8_t(n);
  unsigned length=offset==16?16:128;
  assert(HPHDMI::indexedDdcRead(io,offset,out.data(),length)==HPHDMI::IndexedResult::Ready);
  assert((io.command&0xff)==0xa1&&((io.command>>8)&255)==offset);
  for(unsigned n=0;n<length;++n)assert(out[n]==uint8_t(n+offset));
  assert(io.stopped&&io.writes.back()==std::make_pair(0xc5108u,1u<<15));
 }
 for(unsigned scenario=0;scenario<5;++scenario) {
  Io io;io.busy=scenario==0;io.missing=scenario==1;io.error=scenario==2;io.stall=scenario==3;io.stuckStop=scenario==4;
  auto result=HPHDMI::indexedDdcRead(io,0,out.data(),128);
  assert(result!=HPHDMI::IndexedResult::Ready);
  if(scenario<2)assert(io.writes.empty());
  else assert(io.stopped);
  assert(io.delayed<=60000&&io.readCount<6100);
 }
 Io invalid;assert(HPHDMI::indexedDdcRead(invalid,255,out.data(),2)==HPHDMI::IndexedResult::Invalid&&invalid.writes.empty());
}
