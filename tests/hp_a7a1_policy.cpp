#include "../NootedGreen/kern_hp_a7a1.hpp"
#include <cassert>
int main() {
    using namespace HPA7A1;
    assert(matches(0x8086,0xA7A1,4,0x103C,0x8B34,true));
    assert(matches(0x8086,0x9A49,4,0x103C,0x8B34,true));
    assert(!matches(0x8086,0xA7A1,4,0x103C,0x8B34,false));
    assert(!matches(0x8086,0xA7A0,4,0x103C,0x8B34,true));
    assert(!matches(0x8086,0xA7A1,3,0x103C,0x8B34,true));
    assert(!matches(0x8086,0xA7A1,4,0x17AA,0x8B34,true));
    assert(!matches(0x8086,0xA7A1,4,0x103C,0x8B35,true));
    assert(!matches(0x1002,0xA7A1,4,0x103C,0x8B34,true));
    assert(EUs==80 && NativeSubslices==10 && L3Banks==6);
    assert(MaxFrequencyMHz==1250);
    assert(frequencyFloor(1500)==1250 && frequencyFloor(1250)==1250);
    assert(frequencyFloor(900)==900 && frequencyFloor(0)==0 && frequencyFloor(-1)==0);
    // Intel GMS 0xF0..0xFE denotes four-MiB increments, not 32-MiB units.
    for (unsigned i=0;i<15;++i) assert(dvmtBytes(0xF0+i)==(i+1)*4*1024*1024);
    assert(dvmtBytes(0xFE)==62914560);
    assert(dvmtBytes(2)==67108864);
    assert(dvmtBytes(0)==0 && dvmtBytes(0x11)==0 && dvmtBytes(0xFF)==0);
    assert(classify(nullptr,16)==Sequence::Unexpected);
    assert(classify(Original,15)==Sequence::Unexpected);
    assert(classify(Original,sizeof(Original))==Sequence::Original);
    assert(classify(Corrected,sizeof(Corrected))==Sequence::Corrected);
    // Reject mutation anywhere in either instruction/store fingerprint.
    for (unsigned i=0;i<sizeof(Original);++i) {
        uint8_t a[sizeof(Original)], b[sizeof(Corrected)];
        for (unsigned j=0;j<sizeof(Original);++j) { a[j]=Original[j];b[j]=Corrected[j]; }
        a[i]^=0x80;b[i]^=0x80;
        assert(classify(a,sizeof(a))==Sequence::Unexpected);
        assert(classify(b,sizeof(b))==Sequence::Unexpected);
    }
    uint32_t immediate=0;
    for (unsigned i=0;i<4;++i) immediate|=uint32_t(Corrected[1+i])<<(8*i);
    assert(immediate==DVMT60);
    for (unsigned i=8;i<sizeof(Original);++i) assert(Original[i]==Corrected[i]);
}
