#pragma once
#include <stdint.h>
#include "dp2_source_control.hpp"
#include "dp2_stream_fields.hpp"
// AMD DCN401 DP stream blank/unblank, restricted to this physical DP2/DIG2.
// E owns a display transaction and implements masked, verified writes,
// bounded waitMasked(address,mask,value,delay_us,polls), and delay(us).
namespace Dp120 {
template<class E> class StreamControl {
    E&e;SourceProof proof;
    bool set(uint32_t a,uint32_t mask,uint32_t value){
        uint32_t old=0;
        return sourceAdmitted(proof) && !(value&~mask) && e.read(a,&old) && old!=0xffffffffu && e.writeMaskedVerified(a,mask,value);
    }
    bool wait(uint32_t a,uint32_t mask,uint32_t value,unsigned polls){
        return sourceAdmitted(proof) && e.waitMasked(a,mask,value,10,polls);
    }
public:
    StreamControl(E&env,const SourceProof&p):e(env),proof(p){}
    bool blank(){
        using namespace StreamFields;
        if(!sourceAdmitted(proof))return false;
        uint32_t v=0;if(!e.read(DP_VID_STREAM_CNTL,&v)||v==0xffffffffu)return false;
        if(!(v&DP_VID_STREAM_ENABLE_MASK))return !(v&DP_VID_STREAM_STATUS_MASK);
        return set(DP_VID_STREAM_CNTL,DP_VID_STREAM_DIS_DEFER_MASK,2u<<DP_VID_STREAM_DIS_DEFER_SHIFT) &&
            set(DP_VID_STREAM_CNTL,DP_VID_STREAM_ENABLE_MASK,0) &&
            wait(DP_VID_STREAM_CNTL,DP_VID_STREAM_STATUS_MASK,0,10200) &&
            set(DP_STEER_FIFO,DP_STEER_FIFO_RESET_MASK,DP_STEER_FIFO_RESET_MASK);
    }
    bool rgb8(){
        using namespace StreamFields;
        // 0=uncompressed RGB444, depth code 1=8 bpc; no DSC or sub-sampling.
        return set(DP_PIXEL_FORMAT,PIXEL_ENCODING_TYPE_MASK|UNCOMPRESSED_PIXEL_FORMAT_MASK|
                UNCOMPRESSED_COMPONENT_DEPTH_MASK|COMPRESSED_PIXEL_FORMAT_MASK,
                1u<<UNCOMPRESSED_COMPONENT_DEPTH_SHIFT) &&
            set(DP_MSA_COLORIMETRY,DP_MSA_MISC0_MASK,0x20u<<DP_MSA_MISC0_SHIFT);
    }
    bool unblank(uint32_t pixelKhz,uint8_t rate,unsigned pixelsPerCycle){
        using namespace StreamFields;
        if(!sourceAdmitted(proof) || (rate!=20 && rate!=30) ||
           !pixelKhz || pixelKhz>1066560u || (pixelsPerCycle!=1 && pixelsPerCycle!=2))return false;
        const uint32_t n=0x8000;
        const uint32_t m=(uint32_t)((uint64_t)n*pixelKhz/((uint32_t)rate*27000u));
        if(m==0 || m>DP_VID_M_MASK)return false;
        if(!set(DP_VID_TIMING,DP_VID_M_N_GEN_EN_MASK,0) ||
           !set(DP_VID_N,DP_VID_N_MASK,n) || !set(DP_VID_M,DP_VID_M_MASK,m) ||
           !set(DP_VID_TIMING,DP_VID_N_INTERVAL_MASK,(pixelsPerCycle==2?1u:0u)<<DP_VID_N_INTERVAL_SHIFT) ||
           !set(DP_VID_TIMING,DP_VID_M_N_GEN_EN_MASK,DP_VID_M_N_GEN_EN_MASK) ||
           !set(DIG_FIFO_CTRL0,DIG_FIFO_OUTPUT_PIXEL_PER_CYCLE_MASK,(pixelsPerCycle==2?1u:0u)<<DIG_FIFO_OUTPUT_PIXEL_PER_CYCLE_SHIFT) ||
           !set(DP_VID_STREAM_CNTL,DP_VID_STREAM_ENABLE_MASK,0) ||
           !wait(DP_VID_STREAM_CNTL,DP_VID_STREAM_STATUS_MASK,0,5000) ||
           !set(DP_STEER_FIFO,DP_STEER_FIFO_RESET_MASK,DP_STEER_FIFO_RESET_MASK))return false;
        e.delay(10);
        if(!set(DP_STEER_FIFO,DP_STEER_FIFO_RESET_MASK,0) ||
           !set(DP_STEER_FIFO,DP_STEER_FIFO_ENABLE_MASK,DP_STEER_FIFO_ENABLE_MASK) ||
           !set(DP_VID_STREAM_CNTL,DP_VID_STREAM_ENABLE_MASK|DP_VID_STREAM_DIS_DEFER_MASK,
                DP_VID_STREAM_ENABLE_MASK|(2u<<DP_VID_STREAM_DIS_DEFER_SHIFT)))return false;
        e.delay(200);
        if(!set(DIG_FIFO_CTRL0,DIG_FIFO_READ_START_LEVEL_MASK,7u<<DIG_FIFO_READ_START_LEVEL_SHIFT) ||
           !set(DIG_FIFO_CTRL0,DIG_FIFO_RESET_MASK,DIG_FIFO_RESET_MASK) ||
           !wait(DIG_FIFO_CTRL0,DIG_FIFO_RESET_DONE_MASK,DIG_FIFO_RESET_DONE_MASK,5000) ||
           !set(DIG_FIFO_CTRL0,DIG_FIFO_RESET_MASK,0) ||
           !wait(DIG_FIFO_CTRL0,DIG_FIFO_RESET_DONE_MASK,0,5000) ||
           !set(DIG_FIFO_CTRL0,DIG_FIFO_ENABLE_MASK,DIG_FIFO_ENABLE_MASK))return false;
        e.delay(100);
        return set(DP_VID_STREAM_CNTL,DP_VID_STREAM_ENABLE_MASK,DP_VID_STREAM_ENABLE_MASK);
    }
};
}
