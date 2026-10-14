#pragma once
#include <stdint.h>
#include "dcn41_regs.h"
namespace Dp120 {
enum FwQueryStatus { FwOk=0,FwBusy=1,FwReadFailed=2,FwTimedOut=3,FwRaced=4 };
struct FwQueryResult {
    FwQueryStatus status=FwBusy;
    uint32_t version=0,before=0,after=0,polls=0;
    bool sent=false;
};
template<class E> FwQueryResult queryFirmwareVersion(E&e){
    FwQueryResult r;
    constexpr uint32_t B=0x34c0u;
    uint32_t cntl=0,cntl2=0,scratch0=0,wptr=0,rptr=0,size=0;
    if(!e.read(B+DCN41_DMCUB_CNTL,cntl) || !e.read(B+DCN41_DMCUB_CNTL2,cntl2) ||
       !e.read(B+DCN41_DMCUB_SCRATCH0,scratch0) || !e.read(B+DCN41_DMCUB_INBOX1_WPTR,wptr) ||
       !e.read(B+DCN41_DMCUB_INBOX1_RPTR,rptr) || !e.read(B+DCN41_DMCUB_INBOX1_SIZE,size) ||
       !e.read(B+DCN41_DMCUB_GPINT_DATAIN1,r.before)){r.status=FwReadFailed;return r;}
    r.after=r.before;
    if(!(cntl&DCN41_DMCUB_CNTL__DMCUB_ENABLE_MASK) || (cntl2&DCN41_DMCUB_CNTL2__DMCUB_SOFT_RESET_MASK) ||
       !(scratch0&2u) || (r.before&0xf0000000u) || wptr!=rptr || size!=8192u || (wptr&63u) || wptr>=size)return r;
    // Fixed version query only; no command/input chosen by the user client.
    e.versionRequest();r.sent=true;
    for(unsigned i=0;i<1000;i++){
        e.pause10us();r.polls=i+1;
        if(!e.read(B+DCN41_DMCUB_GPINT_DATAIN1,r.after)){r.status=FwReadFailed;return r;}
        if(r.after==0x00010000u){
            r.status=e.read(B+DCN41_DMCUB_SCRATCH7,r.version)?FwOk:FwReadFailed;
            return r;
        }
        if(r.after!=0x10010000u){r.status=FwRaced;return r;}
    }
    // No reset/overwrite of the mailbox on failure: firmware may still own it.
    r.status=FwTimedOut;return r;
}
}
