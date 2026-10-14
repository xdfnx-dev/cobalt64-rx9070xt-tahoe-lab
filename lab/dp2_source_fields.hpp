// Generated from AMD Linux 238650ef6c7c DCN4.1 offset/sh_mask headers.
// Exact DP2 only; absolute BAR5 dwords. No I/O.
#pragma once
#include <stdint.h>
namespace Dp120 { namespace SourceFields {
constexpr uint32_t DP_LINK_CNTL = 0x5826u;
constexpr uint32_t DP_LINK_TRAINING_COMPLETE_MASK = 0x10u;
constexpr unsigned DP_LINK_TRAINING_COMPLETE_SHIFT = 4u;
constexpr uint32_t DP_CONFIG = 0x5829u;
constexpr uint32_t DP_UDI_LANES_MASK = 0x3u;
constexpr unsigned DP_UDI_LANES_SHIFT = 0u;
constexpr uint32_t DP_DPHY_CNTL = 0x5835u;
constexpr uint32_t DPHY_BYPASS_MASK = 0x10000u;
constexpr unsigned DPHY_BYPASS_SHIFT = 16u;
constexpr uint32_t DP_DPHY_TRAINING_PATTERN_SEL = 0x5836u;
constexpr uint32_t DPHY_TRAINING_PATTERN_SEL_MASK = 0x3u;
constexpr unsigned DPHY_TRAINING_PATTERN_SEL_SHIFT = 0u;
constexpr uint32_t DP_DPHY_PRBS_CNTL = 0x583bu;
constexpr uint32_t DPHY_PRBS_EN_MASK = 0x1u;
constexpr unsigned DPHY_PRBS_EN_SHIFT = 0u;
constexpr uint32_t DP_DPHY_SCRAM_CNTL = 0x583cu;
constexpr uint32_t DPHY_SCRAMBLER_ADVANCE_MASK = 0x10u;
constexpr unsigned DPHY_SCRAMBLER_ADVANCE_SHIFT = 4u;
constexpr uint32_t DPHY_SCRAMBLER_BS_COUNT_MASK = 0x3ff00u;
constexpr unsigned DPHY_SCRAMBLER_BS_COUNT_SHIFT = 8u;
constexpr uint32_t DP_LINK_FRAMING_CNTL = 0x5831u;
constexpr uint32_t DP_IDLE_BS_INTERVAL_MASK = 0x3ffffu;
constexpr unsigned DP_IDLE_BS_INTERVAL_SHIFT = 0u;
constexpr uint32_t DP_VBID_DISABLE_MASK = 0x1000000u;
constexpr unsigned DP_VBID_DISABLE_SHIFT = 24u;
constexpr uint32_t DP_VID_ENHANCED_FRAME_MODE_MASK = 0x10000000u;
constexpr unsigned DP_VID_ENHANCED_FRAME_MODE_SHIFT = 28u;
constexpr uint32_t DP_DPHY_INTERNAL_CTRL = 0x582du;
constexpr uint32_t DPHY_ALT_SCRAMBLER_RESET_EN_MASK = 0x1u;
constexpr unsigned DPHY_ALT_SCRAMBLER_RESET_EN_SHIFT = 0u;
constexpr uint32_t DPHY_ALT_SCRAMBLER_RESET_SEL_MASK = 0x10u;
constexpr unsigned DPHY_ALT_SCRAMBLER_RESET_SEL_SHIFT = 4u;
} }
