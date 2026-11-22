// Local HP 250 profile. Identity and measured DVMT policy, no hardware writes.
#ifndef NG_HP_A7A1_HPP
#define NG_HP_A7A1_HPP
#include <stdint.h>
#include <stddef.h>
namespace HPA7A1 {
inline bool matches(uint16_t vendor, uint16_t device, uint8_t revision,
                    uint16_t subvendor, uint16_t subsystem, bool requested) {
    // 9A49 is allowed only as the configured TGL donor on this exact HP board.
    return requested && vendor == 0x8086 && (device == 0xA7A1 || device == 0x9A49) &&
        revision == 4 && subvendor == 0x103C && subsystem == 0x8B34;
}
constexpr uint32_t EUs = 80, NativeSubslices = 10, L3Banks = 6;
constexpr int MaxFrequencyMHz = 1250; // Intel i5-1334U published maximum.
inline int frequencyFloor(int requested) {
    return requested < 0 ? 0 : requested > MaxFrequencyMHz ? MaxFrequencyMHz : requested;
}
constexpr uint32_t MiB = 1024 * 1024, DVMT60 = 60 * MiB;
inline uint32_t dvmtBytes(uint8_t gms) {
    if (gms < 0x10 || gms == 0x20 || gms == 0x30 || gms == 0x40)
        return uint32_t(gms) * 32 * MiB;
    if (gms >= 0xF0 && gms <= 0xFE)
        return uint32_t((gms & 15) + 1) * 4 * MiB;
    return 0;
}
// FBMemMgr_Init, exact 16.0.32 debug TGL donor. Preserve the member store.
constexpr uint8_t Original[] = {0xC1,0xE0,0x11,0x25,0,0,0,0xFE,0x41,0x89,0x84,0x24,0xE4,0x0D,0,0};
constexpr uint8_t Corrected[] = {0xB8,0,0,0xC0,0x03,0x90,0x90,0x90,0x41,0x89,0x84,0x24,0xE4,0x0D,0,0};
enum class Sequence { Unexpected, Original, Corrected };
inline Sequence classify(const uint8_t *code, size_t bytes) {
    if (!code || bytes < sizeof(Original)) return Sequence::Unexpected;
    bool original = true, corrected = true;
    for (size_t i = 0; i < sizeof(Original); ++i) {
        original &= code[i] == Original[i]; corrected &= code[i] == Corrected[i];
    }
    return original ? Sequence::Original : corrected ? Sequence::Corrected : Sequence::Unexpected;
}
}
#endif
