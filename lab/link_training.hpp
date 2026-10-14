#pragma once
#include <stdint.h>

// Offline prototype for a direct 8b/10b DP2/AUX2 HBR3 link. Hardware backend
// is intentionally absent. Algorithms referenced: AMD Linux 238650ef6c7c
// link_dp_training.c and link_dp_training_8b_10b.c (MIT, see linux-reference).
// This does NOT implement the PHY registers, clocks, DML, or OS mode exposure.
namespace Dp120 {
enum Status { Ok, Denied, SnapshotFailed, OperationFailed, ClockRecoveryFailed,
              EqualizationFailed, MeasurementFailed, RestoreFailed };
struct Admission {
    bool backendVerified=false, beforeLogin=false, directLinkVerified=false;
    bool exactAcerTiming=false, sourceTps4=false, sinkTps4=false;
    uint8_t maxRate=0,lanes=0,aux=0,dig=0,otg=255;
};
struct Result { Status status=Denied; uint32_t crPolls=0,eqPolls=0; bool restored=false; };
inline bool admitted(const Admission&a){
    return a.backendVerified && a.beforeLogin && a.directLinkVerified && a.exactAcerTiming &&
        a.sourceTps4 && a.sinkTps4 && a.maxRate>=30 && a.lanes==4 && a.aux==2 && a.dig==2 && a.otg==0;
}
inline uint8_t lane(const uint8_t*s,unsigned i){return (s[i/2]>>(4*(i%2)))&15;}
inline bool crDone(const uint8_t*s){for(unsigned i=0;i<4;i++)if(!(lane(s,i)&1))return false;return true;}
inline bool eqDone(const uint8_t*s){for(unsigned i=0;i<4;i++)if((lane(s,i)&7)!=7)return false;return (s[2]&1)!=0;}
inline void requested(const uint8_t*s,uint8_t &vs,uint8_t &pe){
    uint8_t v=0,p=0;
    for(unsigned i=0;i<4;i++){
        uint8_t a=(s[4+i/2]>>(4*(i%2)))&15;
        if((a&3)>v)v=a&3;if((a>>2)>p)p=a>>2;
    }
    vs=v;pe=p>3-v?3-v:p;
}
inline uint8_t laneSetting(uint8_t vs,uint8_t pe){
    return vs|(pe<<3)|(vs==3?4:0)|(pe==3-vs?32:0);
}
template<class E> inline Result trial(E&e,const Admission&a){
    Result r;if(!admitted(a))return r;
    if(!e.snapshot()){r.status=SnapshotFailed;return r;}
    auto failure=[&](Status st){r.status=st;r.restored=e.restoreVerified();if(!r.restored)r.status=RestoreFailed;return r;};
    if(!e.blank() || !e.configureSourceHbr3())return failure(OperationFailed);
    const uint8_t link[2]={30,0x84},coding=1;
    if(!e.auxWrite(0x100,link,2) || !e.auxWrite(0x108,&coding,1))return failure(OperationFailed);
    uint8_t vs=0,pe=0,s[6]={},same=0;bool recovered=false;
    if(!e.sourcePattern(1))return failure(OperationFailed);
    for(unsigned poll=0;poll<10;poll++){
        uint8_t p[5]={0x21,0,0,0,0};for(unsigned i=1;i<5;i++)p[i]=laneSetting(vs,pe);
        if(!e.sourceLanes(vs,pe) || !e.auxWrite(0x102,p,5))return failure(OperationFailed);
        e.waitInterval();++r.crPolls;
        if(!e.auxReadStatus(s))return failure(OperationFailed);
        if(crDone(s)){recovered=true;break;}
        if(vs==3)return failure(ClockRecoveryFailed);
        uint8_t nextVs,nextPe;requested(s,nextVs,nextPe);
        same=nextVs==vs?(uint8_t)(same+1):0;vs=nextVs;pe=nextPe;
        if(same>=5)return failure(ClockRecoveryFailed);
    }
    if(!recovered)return failure(ClockRecoveryFailed);
    // TPS4 uses DPCD pattern 7 and scrambling enabled, unlike TPS1's 0x21.
    if(!e.sourcePattern(4))return failure(OperationFailed);
    bool equalized=false;
    for(unsigned poll=0;poll<6;poll++){
        uint8_t p[5]={7,0,0,0,0};for(unsigned i=1;i<5;i++)p[i]=laneSetting(vs,pe);
        if(!e.sourceLanes(vs,pe) || !e.auxWrite(0x102,p,5))return failure(OperationFailed);
        e.waitInterval();++r.eqPolls;
        if(!e.auxReadStatus(s))return failure(OperationFailed);
        if(!crDone(s))return failure(EqualizationFailed);
        if(eqDone(s)){equalized=true;break;}
        requested(s,vs,pe);
    }
    if(!equalized)return failure(EqualizationFailed);
    const uint8_t stop=0;
    if(!e.auxWrite(0x102,&stop,1) || !e.sourcePattern(0) ||
       !e.applyValidatedClockTiming() || !e.unblank())return failure(OperationFailed);
    if(!e.measure120())return failure(MeasurementFailed);
    // Trial always restores 4K60; permanent mode requires a separate design.
    r.restored=e.restoreVerified();r.status=r.restored?Ok:RestoreFailed;return r;
}
} // namespace Dp120
