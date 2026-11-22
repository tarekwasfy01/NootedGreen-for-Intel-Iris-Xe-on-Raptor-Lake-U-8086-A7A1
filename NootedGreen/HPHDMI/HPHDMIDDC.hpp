// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <stddef.h>
namespace HPHDMI {
enum class DdcResult : uint32_t { Ready, InvalidBlock, TransferFailed, BadHeader, BadChecksum };
// Two EDID blocks need no E-DDC segment-pointer transaction. Never expose an
// incomplete block and never substitute a cached EDID from a different monitor.
inline DdcResult validateNativeEdid(uint32_t block,const uint8_t *bytes) {
 if(block==1) {
  constexpr uint8_t header[]={0,255,255,255,255,255,255,0};
  for(size_t n=0;n<8;++n)if(bytes[n]!=header[n])return DdcResult::BadHeader;
 }
 uint8_t sum=0;for(size_t n=0;n<128;++n)sum+=bytes[n];
 if(sum)return DdcResult::BadChecksum;
 return DdcResult::Ready;
}
template<class Transport> DdcResult readNativeEdid(Transport &transport,uint32_t block,uint8_t *out) {
 if(!out||block<1||block>2)return DdcResult::InvalidBlock;
 uint8_t bytes[128]={};
 if(!transport.transfer((block-1)*128,bytes,128))return DdcResult::TransferFailed;
 auto result=validateNativeEdid(block,bytes);
 if(result==DdcResult::Ready)for(size_t n=0;n<128;++n)out[n]=bytes[n];
 return result;
}
// One bounded recovery pass. Re-read every byte using separately addressed
// 16-byte transactions; never mix partial initial data or use a stored EDID.
template<class Transport> DdcResult recoverNativeEdid(Transport &transport,uint32_t block,uint8_t *out) {
 if(!out||block<1||block>2)return DdcResult::InvalidBlock;
 uint8_t bytes[128]={};
 for(uint32_t offset=0;offset<128;offset+=16)
  if(!transport.transfer((block-1)*128+offset,bytes+offset,16))return DdcResult::TransferFailed;
 auto result=validateNativeEdid(block,bytes);
 if(result==DdcResult::Ready)for(size_t n=0;n<128;++n)out[n]=bytes[n];
 return result;
}
}
