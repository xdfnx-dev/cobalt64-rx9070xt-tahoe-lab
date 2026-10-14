#pragma once
#include <stdint.h>
#include "dp2_source_fields.hpp"

// Source side of direct DP2 8b/10b training, ported from AMD's
// dcn10_link_encoder.c (used by dcn401_dio_link_encoder.c).
// Host-tested only. No production transport or firmware admission yet.
// Preserve AMD MIT copyright/license in linux-reference/dcn10_link_encoder.c.
namespace Dp120 {
struct SourceProof {
    bool beforeLogin=false, exclusiveDisplay=false, firmwareAbiVerified=false;
    bool transportVerified=false, sourceTps4=false;
    uint8_t phy=255,dig=255,aux=255,hpd=0;
};
inline bool sourceAdmitted(const SourceProof&p) {
    return p.beforeLogin && p.exclusiveDisplay && p.firmwareAbiVerified &&
        p.transportVerified && p.sourceTps4 && p.phy==2 && p.dig==2 && p.aux==2 && p.hpd==3;
}

// E must serialize against all other display writers and implement:
// bool read(abs, *value), bool writeMaskedVerified(abs, mask, value).
// writeMaskedVerified must itself restrict the address/mask, preserve other
// bits, read back, and reject an unreadable 0xffffffff register. These are
// not unrestricted MMIO writes. An E returning false stops the sequence.
template<class E> class SourceControl {
    E &e; SourceProof proof;
    bool set(uint32_t a,uint32_t mask,uint32_t v) {
        if(!sourceAdmitted(proof) || (v & ~mask))return false;
        uint32_t old=0;
        if(!e.read(a,&old) || old==0xffffffffu)return false;
        return e.writeMaskedVerified(a,mask,v);
    }
public:
    SourceControl(E&env,const SourceProof&p):e(env),proof(p){}
    bool configureFourLanes() {
        using namespace SourceFields;
        return set(DP_CONFIG,DP_UDI_LANES_MASK,3u<<DP_UDI_LANES_SHIFT) &&
            set(DP_DPHY_SCRAM_CNTL,DPHY_SCRAMBLER_ADVANCE_MASK,1u<<DPHY_SCRAMBLER_ADVANCE_SHIFT);
    }
    bool pattern(uint8_t pattern) {
        using namespace SourceFields;
        if(!sourceAdmitted(proof) || (pattern!=0 && pattern!=1 && pattern!=4))return false;
        if(pattern) {
            // TPS1=0 and TPS4=3 are hardware encodings; DPCD TPS4=7.
            if(!set(DP_DPHY_TRAINING_PATTERN_SEL,DPHY_TRAINING_PATTERN_SEL_MASK,
                (pattern==1?0u:3u)<<DPHY_TRAINING_PATTERN_SEL_SHIFT) ||
               !set(DP_LINK_CNTL,DP_LINK_TRAINING_COMPLETE_MASK,0))return false;
        } else {
            // DP_PANEL_MODE_DEFAULT: no alternate scrambler-reset mode.
            if(!set(DP_DPHY_INTERNAL_CTRL,DPHY_ALT_SCRAMBLER_RESET_EN_MASK|
                    DPHY_ALT_SCRAMBLER_RESET_SEL_MASK,0) ||
               !set(DP_LINK_FRAMING_CNTL,DP_IDLE_BS_INTERVAL_MASK|DP_VBID_DISABLE_MASK|
                    DP_VID_ENHANCED_FRAME_MODE_MASK,
                    (0x2000u<<DP_IDLE_BS_INTERVAL_SHIFT)|(1u<<DP_VID_ENHANCED_FRAME_MODE_SHIFT)) ||
               !set(DP_DPHY_SCRAM_CNTL,DPHY_SCRAMBLER_BS_COUNT_MASK,
                    0x1ffu<<DPHY_SCRAMBLER_BS_COUNT_SHIFT) ||
               !set(DP_LINK_CNTL,DP_LINK_TRAINING_COMPLETE_MASK,
                    1u<<DP_LINK_TRAINING_COMPLETE_SHIFT))return false;
        }
        return set(DP_DPHY_CNTL,DPHY_BYPASS_MASK,0) &&
            set(DP_DPHY_PRBS_CNTL,DPHY_PRBS_EN_MASK,0);
    }
};
} // namespace Dp120
