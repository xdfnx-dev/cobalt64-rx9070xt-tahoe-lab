#include "fw_query.hpp"
#include <map>
#include <cstdio>
#include <cassert>
using namespace Dp120;
struct Env {
    std::map<uint32_t,uint32_t> regs;
    unsigned writes=0,polls=0,ackAt=3;
    uint32_t fail=0,unexpected=0;
    Env(){regs={{0x36b6,0x10000},{0x36c0,0},{0x36a3,0x82},{0x3696,0x940},
        {0x3697,0x940},{0x3695,8192},{0x36b8,0},{0x36aa,0x123456}};}
    bool read(uint32_t a,uint32_t&v){
        if(a==fail || !regs.count(a))return false;
        if(a==0x36b8 && writes && polls>=ackAt)regs[a]=unexpected?unexpected:0x10000;
        v=regs[a];return v!=0xffffffffu;
    }
    void versionRequest(){assert(!writes);regs[0x36b8]=0x10010000;++writes;}
    void pause10us(){++polls;}
};
int main(){
    unsigned checks=0;
    auto check=[&](bool b){++checks;assert(b);};
    {Env e;auto r=queryFirmwareVersion(e);check(r.status==FwOk);check(r.sent);check(r.version==0x123456);check(e.writes==1);check(r.polls==3);}
    for(auto [a,v]:std::map<uint32_t,uint32_t>{{0x36b6,0},{0x36c0,1},{0x36a3,0},{0x36b8,0x10020000},
            {0x3697,0x900},{0x3696,0x941},{0x3695,16384},{0x3696,8192}}){
        Env e;e.regs[a]=v;auto r=queryFirmwareVersion(e);check(r.status==FwBusy);check(!r.sent);check(e.writes==0);check(e.polls==0);
    }
    for(auto a:{0x36b6u,0x36c0u,0x36a3u,0x3696u,0x3697u,0x3695u,0x36b8u}){
        Env e;e.fail=a;auto r=queryFirmwareVersion(e);check(r.status==FwReadFailed);check(!r.sent);check(e.writes==0);
    }
    {Env e;e.ackAt=1001;auto r=queryFirmwareVersion(e);check(r.status==FwTimedOut);check(r.polls==1000);check(e.writes==1);check(e.regs[0x36b8]==0x10010000);}
    {Env e;e.unexpected=0x20000;auto r=queryFirmwareVersion(e);check(r.status==FwRaced);check(e.writes==1);check(r.polls==3);}
    {Env e;e.fail=0x36aa;auto r=queryFirmwareVersion(e);check(r.status==FwReadFailed);check(e.writes==1);}
    std::printf("Fixed GPINT query: %u checks passed, including pending/raced/timeout refusal.\n",checks);
}
