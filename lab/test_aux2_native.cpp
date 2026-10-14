#include "aux2_native.hpp"
#include <map>
#include <vector>
#include <cassert>
#include <cstdio>
struct Env {
    std::map<uint32_t,uint32_t> regs{{0x4baa,1},{0x4bab,0},{0x4bac,0},{0x4bae,0}};
    std::vector<uint8_t> tx,rx;unsigned cursor=0,ops=0,failAt=0,go=0,release=0,delays=0;
    bool allowed=true,timeout=false;uint8_t reply=0;
    bool auxAdmitted(bool,uint32_t a,uint8_t n){return allowed && a+n<=0x100000;}
    bool auxReadReg(uint32_t a,uint32_t*out){
        if(++ops==failAt)return false;
        if(a==0x4bb0){if(cursor>=rx.size())return false;*out=(uint32_t)rx[cursor++]<<8;return true;}
        *out=regs[a];return true;
    }
    bool auxWriteReg(uint32_t a,uint32_t v){
        if(++ops==failAt)return false;
        if(a==0x4bac){if(v&0x10000)regs[a]=0x10004;else{regs[a]=0;++release;}}
        else if(a==0x4bad)regs[0x4bae]=0;
        else if(a==0x4bb0){if(v&1)cursor=0;else{if(v&0x80000000)tx.clear();tx.push_back((uint8_t)(v>>8));}}
        else {regs[a]=v;if(a==0x4bab && (v&1)){
            ++go;rx.clear();rx.push_back(reply<<4);bool wr=(tx[0]&0x10)==0;
            if(!wr)for(unsigned i=0;i<(unsigned)tx[3]+1;i++)rx.push_back(0x20+i);
            regs[0x4bae]=timeout?0:((uint32_t)rx.size()<<24)|1;regs[a]&=~1;
        }}return true;
    }
    void auxDelay(unsigned n){delays+=n;}
};
int main(){using namespace Dp120;unsigned checks=0;auto c=[&](bool v){++checks;assert(v);};
    unsigned happyOps=0;
    {Env e;NativeAux2<Env>a(e);uint8_t data[16]={};auto r=a.read(0x2200,data,16);c(r.status==AuxOk);c(r.released);c(r.count==16);c(data[0]==0x20&&data[15]==0x2f);c(e.tx==std::vector<uint8_t>({0x90,0x22,0,15}));happyOps=e.ops;}
    {Env e;NativeAux2<Env>a(e);uint8_t data[2]={30,0x84};auto r=a.write(0x100,data,2);c(r.status==AuxOk&&r.released);c(e.tx==std::vector<uint8_t>({0x80,1,0,1,30,0x84}));}
    for(unsigned i=1;i<=happyOps;i++){
        Env e;e.failAt=i;NativeAux2<Env>a(e);uint8_t b[16]={};auto r=a.read(0x2200,b,16);
        c(r.status!=AuxOk);c(e.go<=1);c(i<=3 || r.released || r.status==AuxReleaseFailed);
    }
    for(unsigned grant:{4u,8u,0x10000u}){Env e;e.regs[0x4bac]=grant;NativeAux2<Env>a(e);uint8_t b=0;auto r=a.read(0,&b,1);c(r.status==AuxBusy);c(e.go==0&&e.release==0);}
    {Env e;e.allowed=false;NativeAux2<Env>a(e);uint8_t b=0;auto r=a.read(0,&b,1);c(r.status==AuxDenied&&e.ops==0);}
    {Env e;e.timeout=true;NativeAux2<Env>a(e);uint8_t b=0;auto r=a.read(0,&b,1);c(r.status==AuxTimeout&&r.released);c(e.delays==20000);}
    {Env e;e.reply=2;NativeAux2<Env>a(e);uint8_t b=0;auto r=a.read(0,&b,1);c(r.status==AuxReply&&r.released);c(e.go==7&&e.release==7);}
    {Env e;NativeAux2<Env>a(e);uint8_t b=0;auto r=a.read(0xfffff,&b,2);c(r.status==AuxDenied&&e.ops==0);}
    std::printf("AUX2 native transport: %u checks passed (simulated I/O only).\n",checks);
}
