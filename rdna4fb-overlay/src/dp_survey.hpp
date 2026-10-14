#pragma once
#include <stdint.h>
#include <stddef.h>

// A bounded boot-time survey of the confirmed Acer on AUX2. Native AUX
// requests are READS only. I2C writes select EDID's offset/segment; every
// segment attempt ends by selecting segment zero, even on a failed read.
namespace DpSurvey {
constexpr uint8_t Read = 0x90, I2cWrite = 0x00, I2cWriteMot = 0x40;
constexpr uint8_t I2cRead = 0x10, I2cReadMot = 0x50;
struct Reply { int code; uint8_t bytes; };
struct Result {
    uint8_t caps[16] {}, extendedCaps[16] {}, config[16] {}, status[16] {}, extension[128] {};
    uint32_t valid = 0, calls = 0;
    bool segmentReset = false;
};
inline bool checksum(const uint8_t *p) {
    uint32_t sum = 0; for (unsigned i=0; i<128; ++i) sum += p[i];
    return (sum & 255u) == 0;
}
inline bool target(uint8_t inst, const uint8_t *base) {
    const uint8_t header[8] = {0,255,255,255,255,255,255,0};
    if (inst != 2 || !base || !checksum(base)) return false;
    for (unsigned i=0;i<8;++i) if (base[i]!=header[i]) return false;
    return base[8]==0x04 && base[9]==0x72 && base[10]==0x6a && base[11]==0x0f;
}
template<class X> inline Result capture(uint8_t inst, const uint8_t *base, X x) {
    Result out;
    if (!target(inst,base)) return out;
    auto request = [&](uint8_t act,uint32_t addr,const uint8_t *tx,uint8_t n,uint8_t *rx) {
        // Exact commands/addresses only; no native DPCD writes, no link training.
        if (n>16 || (act==Read && addr!=0 && addr!=0x100 && addr!=0x200 && addr!=0x2200)) return false;
        for (unsigned retry=0;retry<7;++retry) {
            ++out.calls; Reply r=x(act,addr,tx,n,rx);
            if (r.code==0) return !(act&0x10) || r.bytes==n;
            if (r.code!=2 && r.code!=8) break;
        }
        return false;
    };
    if (request(Read,0,nullptr,16,out.caps)) out.valid|=1;
    // DP1.3+: base DPCD may deliberately expose compatibility limits. Use
    // 0x2200 only when its presence bit is set, never infer HBR3 from EDID.
    if ((out.valid&1) && (out.caps[14]&0x80) &&
        request(Read,0x2200,nullptr,16,out.extendedCaps) && out.extendedCaps[0]>=out.caps[0]) out.valid|=16;
    if (request(Read,0x100,nullptr,16,out.config)) out.valid|=2;
    if (request(Read,0x200,nullptr,16,out.status)) out.valid|=4;
    if (base[126]<2) return out;
    uint8_t segment=1, offset=0;
    bool selected=request(I2cWriteMot,0x30,&segment,1,nullptr);
    bool ok=selected && request(I2cWriteMot,0x50,&offset,1,nullptr);
    for (unsigned pos=0;pos<128 && ok;pos+=16)
        ok=request(pos==112?I2cRead:I2cReadMot,0x50,nullptr,16,out.extension+pos);
    // End an unfinished MOT transaction before resetting the segment pointer.
    if (!ok) (void)request(I2cRead,0x50,nullptr,0,nullptr);
    segment=0;
    out.segmentReset=request(I2cWrite,0x30,&segment,1,nullptr);
    if (ok && out.segmentReset && checksum(out.extension)) out.valid|=8;
    return out;
}
} // namespace DpSurvey
