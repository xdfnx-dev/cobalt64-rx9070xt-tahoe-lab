// Generated from official AMD Linux 238650ef6c7c DCN4.1 register headers.
#pragma once
#include <stdint.h>
namespace Dp120 {namespace StreamFields {
constexpr uint32_t DP_VID_STREAM_CNTL=0x582au;
constexpr uint32_t DP_VID_STREAM_ENABLE_MASK=0x1u;
constexpr unsigned DP_VID_STREAM_ENABLE_SHIFT=0u;
constexpr uint32_t DP_VID_STREAM_STATUS_MASK=0x10000u;
constexpr unsigned DP_VID_STREAM_STATUS_SHIFT=16u;
constexpr uint32_t DP_VID_STREAM_DIS_DEFER_MASK=0x300u;
constexpr unsigned DP_VID_STREAM_DIS_DEFER_SHIFT=8u;
constexpr uint32_t DP_STEER_FIFO=0x582bu;
constexpr uint32_t DP_STEER_FIFO_ENABLE_MASK=0x1u;
constexpr unsigned DP_STEER_FIFO_ENABLE_SHIFT=0u;
constexpr uint32_t DP_STEER_FIFO_RESET_MASK=0x2u;
constexpr unsigned DP_STEER_FIFO_RESET_SHIFT=1u;
constexpr uint32_t DP_VID_TIMING=0x582eu;
constexpr uint32_t DP_VID_M_N_GEN_EN_MASK=0x100u;
constexpr unsigned DP_VID_M_N_GEN_EN_SHIFT=8u;
constexpr uint32_t DP_VID_N_INTERVAL_MASK=0xc000u;
constexpr unsigned DP_VID_N_INTERVAL_SHIFT=14u;
constexpr uint32_t DP_VID_N=0x582fu;
constexpr uint32_t DP_VID_N_MASK=0xffffffu;
constexpr unsigned DP_VID_N_SHIFT=0u;
constexpr uint32_t DP_VID_M=0x5830u;
constexpr uint32_t DP_VID_M_MASK=0xffffffu;
constexpr unsigned DP_VID_M_SHIFT=0u;
constexpr uint32_t DP_PIXEL_FORMAT=0x5827u;
constexpr uint32_t PIXEL_ENCODING_TYPE_MASK=0x1u;
constexpr unsigned PIXEL_ENCODING_TYPE_SHIFT=0u;
constexpr uint32_t UNCOMPRESSED_PIXEL_FORMAT_MASK=0x30u;
constexpr unsigned UNCOMPRESSED_PIXEL_FORMAT_SHIFT=4u;
constexpr uint32_t UNCOMPRESSED_COMPONENT_DEPTH_MASK=0x700u;
constexpr unsigned UNCOMPRESSED_COMPONENT_DEPTH_SHIFT=8u;
constexpr uint32_t COMPRESSED_PIXEL_FORMAT_MASK=0x10000u;
constexpr unsigned COMPRESSED_PIXEL_FORMAT_SHIFT=16u;
constexpr uint32_t DP_MSA_COLORIMETRY=0x5828u;
constexpr uint32_t DP_MSA_MISC0_MASK=0xff000000u;
constexpr unsigned DP_MSA_MISC0_SHIFT=24u;
constexpr uint32_t DIG_FIFO_CTRL0=0x57a3u;
constexpr uint32_t DIG_FIFO_ENABLE_MASK=0x1u;
constexpr unsigned DIG_FIFO_ENABLE_SHIFT=0u;
constexpr uint32_t DIG_FIFO_RESET_MASK=0x2u;
constexpr unsigned DIG_FIFO_RESET_SHIFT=1u;
constexpr uint32_t DIG_FIFO_READ_START_LEVEL_MASK=0x7cu;
constexpr unsigned DIG_FIFO_READ_START_LEVEL_SHIFT=2u;
constexpr uint32_t DIG_FIFO_RESET_DONE_MASK=0x100000u;
constexpr unsigned DIG_FIFO_RESET_DONE_SHIFT=20u;
constexpr uint32_t DIG_FIFO_OUTPUT_PIXEL_PER_CYCLE_MASK=0x300u;
constexpr unsigned DIG_FIFO_OUTPUT_PIXEL_PER_CYCLE_SHIFT=8u;
}}
