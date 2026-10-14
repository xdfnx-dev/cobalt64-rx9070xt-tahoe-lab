#pragma once
#include <stdint.h>
// DP AUX2 software engine. Register fields match AMD DCN4.1 and the existing
// RDNA4FB AUX implementation. Native DPCD only; no I2C, EDID writes or power.
namespace Dp120 {
enum AuxStatus { AuxOk,AuxDenied,AuxBusy,AuxIo,AuxTimeout,AuxReply,AuxReleaseFailed };
struct AuxResult { AuxStatus status=AuxDenied;uint32_t hwStatus=0;uint8_t reply=255,count=0;bool released=false; };
template<class E> class NativeAux2 {
    E&e;
    static constexpr uint32_t B=0x4baau;
    static constexpr uint32_t REQ=1u<<16,DONE=1u<<17;
    bool read(uint32_t o,uint32_t&v){return e.auxReadReg(B+o,&v)&&v!=0xffffffffu;}
    bool write(uint32_t o,uint32_t v){return e.auxWriteReg(B+o,v);}
    bool release(){uint32_t v=0;return read(2,v)&&write(2,(v&~REQ)|DONE);}
    AuxResult transfer(bool wr,uint32_t address,const uint8_t*src,uint8_t*dst,uint8_t len){
        AuxResult r;
        if(!e.auxAdmitted(wr,address,len) || len==0 || len>16 || address>0xfffffu ||
           (wr&&!src)||(!wr&&!dst))return r;
        uint32_t arb=0,ctl=0,sw=0,status=0;
        if(!read(2,arb)||!read(0,ctl)||!read(1,sw)){r.status=AuxIo;return r;}
        // No stealing an already held software/firmware grant and no GO overlap.
        if((arb&0xcu) || (arb&REQ) || !(ctl&1u) || (sw&1u)){r.status=AuxBusy;return r;}
        auto finish=[&](AuxStatus why){r.status=why;r.released=release();if(!r.released)r.status=AuxReleaseFailed;return r;};
        if(!write(2,(arb&~DONE)|REQ))return finish(AuxIo);
        if(!read(2,arb)||((arb>>2)&3u)!=1u)return finish(AuxBusy);
        if(!write(3,2u))return finish(AuxIo);
        bool idle=false;
        for(unsigned i=0;i<100;i++){
            if(!read(4,status))return finish(AuxIo);
            if(!(status&1u)){idle=true;break;}e.auxDelay(10);
        }
        if(!idle)return finish(AuxTimeout);
        unsigned requestBytes=4u+(wr?len:0u);
        if(!read(1,sw)||!write(1,(sw&~0x1f0000u)|(requestBytes<<16)))return finish(AuxIo);
        auto push=[&](uint8_t b,bool first){return write(6,((uint32_t)b<<8)|(first?0x80000000u:0u));};
        if(!push((wr?0x80u:0x90u)|((address>>16)&15u),true)||
           !push((address>>8)&255u,false)||!push(address&255u,false)||!push(len-1,false))return finish(AuxIo);
        if(wr)for(unsigned i=0;i<len;i++)if(!push(src[i],false))return finish(AuxIo);
        if(!read(1,sw)||!write(1,sw|1u))return finish(AuxIo);
        bool done=false;
        for(unsigned i=0;i<2000;i++){
            if(!read(4,status))return finish(AuxIo);
            if(status&1u){done=true;break;}e.auxDelay(10);
        }
        r.hwStatus=status;
        if(!done || (status&(0x70u|0x80u|0x200u)))return finish(AuxTimeout);
        unsigned count=(status>>24)&31u;
        if(count<1 || count>17 || !write(6,0x80000001u))return finish(AuxReply);
        uint32_t byte=0;
        if(!read(6,byte))return finish(AuxIo);
        r.reply=(byte>>12)&15u;r.count=(uint8_t)(count-1);
        for(unsigned i=1;i<count;i++){
            if(!read(6,byte))return finish(AuxIo);
            if(!wr && i<=len)dst[i-1]=(uint8_t)(byte>>8);
        }
        if(r.reply==0 && (wr || r.count==len))return finish(AuxOk);
        return finish(AuxReply);
    }
public:
    NativeAux2(E&env):e(env){}
    AuxResult transaction(bool wr,uint32_t address,const uint8_t*src,uint8_t*dst,uint8_t len){
        AuxResult r;
        for(unsigned attempt=0;attempt<7;attempt++){
            r=transfer(wr,address,src,dst,len);
            if(r.status!=AuxReply || r.reply!=2u || !r.released)return r;
            e.auxDelay(400);
        }
        return r;
    }
    AuxResult read(uint32_t address,uint8_t*data,uint8_t len){return transaction(false,address,nullptr,data,len);}
    AuxResult write(uint32_t address,const uint8_t*data,uint8_t len){return transaction(true,address,data,nullptr,len);}
};
}
