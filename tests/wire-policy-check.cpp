#include "../NootedGreen/HPHDMI/HPHDMIWirePolicy.hpp"
#include <cassert>
int main() {
 for(unsigned e=0;e<32;++e)for(unsigned b=0;b<32;++b)for(unsigned r=0;r<32;++r) {
  const bool expected=(e==0||e==1)&&(b==0||b==2)&&(r==0||r==1);
  assert(HPHDMI::rgb8Request(e,b,r)==expected);
 }
 assert(HPHDMI::rgb8Request(1,2,1)); // explicit RGB / 8bpc / SDR
 assert(!HPHDMI::rgb8Request(8,2,1)); // YCbCr420
 assert(!HPHDMI::rgb8Request(1,4,1)); // 10bpc
 assert(!HPHDMI::rgb8Request(1,2,2)); // HDR10
}
