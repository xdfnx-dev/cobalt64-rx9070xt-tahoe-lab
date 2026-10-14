#pragma once
#include <stdint.h>
#define N48DP_CLIENT_TYPE 0x4e445031u
#define N48DP_ABI 1u
#define N48DP_SEL_CENSUS 0u
#define N48DP_SEL_FW_VERSION 1u
#define N48DP_SEL_CODE_READ 2u
struct n48dp_window {
    uint32_t base, top, offset, offset_high;
    uint64_t vram_offset, size;
};
struct n48dp_report {
    uint32_t abi, op, status, fw_version;
    uint32_t gpint_before, gpint_after, scratch0, polls;
    uint32_t pixel_format, msa_colorimetry, stream, link;
    uint32_t frame_before, frame_after, reserved[2];
    struct n48dp_window window[2];
};
#ifdef __cplusplus
static_assert(sizeof(n48dp_report)==128,"Stable report ABI");
#endif
