#include "dp2_stream_control.hpp"
#include <cassert>
#include <map>
#include <vector>
#include <utility>
#include <cstdio>
struct Env {
    std::map<uint32_t,uint32_t> regs;
    std::vector<std::pair<uint32_t,uint32_t>> writes;unsigned ops=0,failAt=0,totalDelay=0;
    bool read(uint32_t a,uint32_t*v){if(++ops==failAt)return false;*v=regs[a];return true;}
    bool writeMaskedVerified(uint32_t a,uint32_t m,uint32_t v){if(++ops==failAt)return false;uint32_t old=regs[a];regs[a]=(old&~m)|v;assert((old&~m)==(regs[a]&~m));writes.push_back({a,v});return true;}
    bool waitMasked(uint32_t a,uint32_t m,uint32_t v,unsigned delay,unsigned polls){
        assert(delay==10&&polls<=10200);if(++ops==failAt)return false;regs[a]=(regs[a]&~m)|v;return true;
    }
    void delay(unsigned us){totalDelay+=us;}
};
int main(){using namespace Dp120;using namespace StreamFields;unsigned checks=0;auto c=[&](bool v){++checks;assert(v);};
    SourceProof p;p.beforeLogin=p.exclusiveDisplay=p.firmwareAbiVerified=p.transportVerified=p.sourceTps4=true;p.phy=p.dig=p.aux=2;p.hpd=3;
    c(DP_VID_STREAM_CNTL==0x582a&&DP_STEER_FIFO==0x582b&&DIG_FIFO_CTRL0==0x57a3);
    for(unsigned clock:{533250u,1066560u})for(unsigned rate:{20u,30u}){
        Env e;e.regs[DP_PIXEL_FORMAT]=0x200;e.regs[DP_MSA_COLORIMETRY]=0x40000000;
        StreamControl<Env>s(e,p);c(s.rgb8());c(e.regs[DP_PIXEL_FORMAT]==0x100&&e.regs[DP_MSA_COLORIMETRY]==0x20000000);
        c(s.unblank(clock,rate,1));c(e.regs[DP_VID_N]==0x8000);
        c(e.regs[DP_VID_M]==(uint32_t)(32768ull*clock/(rate*27000u)));c(e.totalDelay==310);
        unsigned maxOps=e.ops;
        for(unsigned i=1;i<=maxOps;i++){Env f;f.failAt=i;StreamControl<Env>t(f,p);bool ok=t.rgb8()&&t.unblank(clock,rate,1);c(!ok);c(f.ops==i);}
    }
    {Env e;e.regs[DP_VID_STREAM_CNTL]=0x10201;StreamControl<Env>s(e,p);c(s.blank());c(e.writes.size()==3);c(e.writes[0].first==DP_VID_STREAM_CNTL&&e.writes[0].second==0x200);c(e.writes.back().first==DP_STEER_FIFO);}
    for(unsigned i=0;i<3;i++){Env e;auto q=p;if(i==0)q.beforeLogin=false;if(i==1)q.firmwareAbiVerified=false;if(i==2)q.dig=1;StreamControl<Env>s(e,q);c(!s.blank()&&!s.rgb8()&&!s.unblank(1066560,30,1)&&e.ops==0);}
    std::printf("DP2 stream transport: %u checks passed (simulated I/O only).\n",checks);
}
