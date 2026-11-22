// SPDX-License-Identifier: MIT
// HP HDMI synthetic AUX endpoint. No MMIO; integration must serialize calls.
// Based on the endpoint method documented by NootedGreen-NP550XDA.
#pragma once
#include <stdint.h>
#include <stddef.h>
namespace HPHDMI {
class Sink {
public:
    static constexpr uint32_t Size = 0x2300;
    enum class Result { PassThrough, Success, InvalidRequest, NotReady };
    void reset() {
        for (uint32_t i=0;i<Size;++i) dpcd[i]=0;
        connected=false; edidReady=false; backendReady=false;
        dpcd[0]=0x12; dpcd[1]=0x0a; dpcd[2]=0x84;
        dpcd[3]=1; dpcd[4]=1; dpcd[6]=1;
        dpcd[8]=2; dpcd[10]=6; dpcd[0x600]=1;
    }
    // Never advertise a synthetic sink without both a valid EDID and a
    // native HDMI backend that has acquired the output resources.
    void setConnection(bool present, bool validEdid, bool nativeBackendReady) {
        if (!present) { reset(); return; }
        connected=present; edidReady=validEdid; backendReady=nativeBackendReady;
    }
    bool admitted() const { return connected && edidReady && backendReady; }
    Result read(uint32_t ddi,uint32_t address,void *buffer,uint32_t length) const {
        if (ddi!=1) return Result::PassThrough;
        if (!valid(address,buffer,length)) return Result::InvalidRequest;
        if (!admitted()) return Result::NotReady;
        auto *bytes=static_cast<uint8_t *>(buffer);
        for(uint32_t i=0;i<length;++i) bytes[i]=status(address+i);
        return Result::Success;
    }
    Result write(uint32_t ddi,uint32_t address,const void *buffer,uint32_t length) {
        if(ddi!=1) return Result::PassThrough;
        if(!valid(address,buffer,length)) return Result::InvalidRequest;
        if(!admitted()) return Result::NotReady;
        const auto *bytes=static_cast<const uint8_t *>(buffer);
        for(uint32_t i=0;i<length;++i) {
            // Only link configuration and power are writable; capabilities
            // and synthetic training status must not be overwritten.
            uint32_t a=address+i;
            if((a>=0x100 && a<=0x10f)||a==0x600) dpcd[a]=bytes[i];
        }
        return Result::Success;
    }
private:
    uint8_t dpcd[Size] = {};
    bool connected=false,edidReady=false,backendReady=false;
    static bool valid(uint32_t address,const void *buffer,uint32_t length) {
        return buffer && length && length<=16 && address<Size && length<=Size-address;
    }
    uint8_t status(uint32_t address) const {
        // Hardware HDMI has no DP link. These synthetic values only describe
        // the software adapter, after its real HDMI backend has been admitted.
        switch(address) {
        case 0x200: return 0x41;
        case 0x202: case 0x203: case 0x200c: case 0x200d: return 0x77;
        case 0x204: case 0x2002: case 0x200e: return 1;
        case 0x205: return 3;
        case 0x206: case 0x207: return 0;
        default: return dpcd[address];
        }
    }
};
}
