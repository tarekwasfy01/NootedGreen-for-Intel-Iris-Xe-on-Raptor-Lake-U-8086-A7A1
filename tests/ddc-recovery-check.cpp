#include "../NootedGreen/HPHDMI/HPHDMIDDC.hpp"
#include <array>
#include <algorithm>
#include <cassert>
struct Transport {
 std::array<uint8_t,256> bytes{};
 unsigned calls=0,fail=0;
 bool transfer(uint32_t offset,uint8_t *out,uint32_t count) {
  ++calls;assert(count==16&&offset+count<=bytes.size());
  if(calls==fail)return false;
  std::copy_n(bytes.data()+offset,count,out);return true;
 }
 void checksum(unsigned base) {bytes[base+127]=0;unsigned sum=0;for(unsigned n=0;n<128;++n)sum+=bytes[base+n];bytes[base+127]=uint8_t(-sum);}
};
int main() {
 std::array<uint8_t,128> out;out.fill(0x5a);const auto original=out;
 Transport t;
 assert(HPHDMI::recoverNativeEdid(t,0,out.data())==HPHDMI::DdcResult::InvalidBlock&&t.calls==0);
 assert(HPHDMI::recoverNativeEdid(t,3,out.data())==HPHDMI::DdcResult::InvalidBlock&&t.calls==0);
 assert(HPHDMI::recoverNativeEdid(t,1,nullptr)==HPHDMI::DdcResult::InvalidBlock&&t.calls==0);
 for(unsigned n=1;n<7;++n)t.bytes[n]=255;t.checksum(0);
 assert(HPHDMI::recoverNativeEdid(t,1,out.data())==HPHDMI::DdcResult::Ready&&t.calls==8);
 assert(std::equal(out.begin(),out.end(),t.bytes.begin()));
 for(unsigned fail=1;fail<=8;++fail) {
  auto broken=t;broken.calls=0;broken.fail=fail;out=original;
  assert(HPHDMI::recoverNativeEdid(broken,1,out.data())==HPHDMI::DdcResult::TransferFailed);
  assert(out==original&&broken.calls==fail);
 }
 out=original;t.bytes[16]^=1;t.calls=0;
 assert(HPHDMI::recoverNativeEdid(t,1,out.data())==HPHDMI::DdcResult::BadChecksum&&out==original);
 t.bytes[16]^=1;t.bytes[0]=1;t.checksum(0);
 assert(HPHDMI::recoverNativeEdid(t,1,out.data())==HPHDMI::DdcResult::BadHeader&&out==original);
 t.bytes[128]=2;t.checksum(128);t.calls=0;
 assert(HPHDMI::recoverNativeEdid(t,2,out.data())==HPHDMI::DdcResult::Ready&&t.calls==8);
 assert(std::equal(out.begin(),out.end(),t.bytes.begin()+128));
}
