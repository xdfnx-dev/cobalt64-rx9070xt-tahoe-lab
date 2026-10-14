#include <cstdio>
#include <cstdint>
#include "Cobalt64/src/navi48-bringup/src/dcn/navi48_dto_clock.h"
static unsigned n=0,bad=0;
static void ck(bool v,const char*s){++n;if(!v){++bad;std::printf("FAIL %s\n",s);}}
int main(){
    uint32_t phase=0,mod=0,rate=0;
    ck(n48dto0_encode(1066560000ull,720000000u,0x8421u,&phase,&mod,&rate),"actual Acer 4K120 encoded");
    ck(phase==346560000u && mod==720000000u && rate==0x8423u,"independent expected 120Hz register tuple");
    ck(n48dto0_decode(0x11090,phase,mod,rate)==1066560000ull,"full clock reconstructed including integer");
    ck((rate&~0x1eu)==(0x8421u&~0x1eu),"other pipes and divider fields preserved");
    ck(n48dto0_decode(0x11090,533250000u,720000000u,0x8421)==533250000ull,"working 4K60 unchanged");
    ck(n48dto0_decode(0x11090,0,720000000,2)==720000000ull,"integer-only boundary accepted");
    for(unsigned i=0;i<=15;i++){
        uint64_t hz=(uint64_t)i*720000000ull+123456u;
        ck(n48dto0_encode(hz,720000000,0xa5a58421,&phase,&mod,&rate),"legal integer field range");
        ck(n48dto0_decode(0x10,phase,mod,rate)==hz,"round trip without 32-bit overflow");
        ck((rate&~0x1eu)==(0xa5a58421u&~0x1eu),"non-target fields unchanged across range");
    }
    ck(!n48dto0_encode(16ull*720000000ull,720000000,0,&phase,&mod,&rate),"unrepresentable integer refused");
    ck(!n48dto0_encode(0,720000000,0,&phase,&mod,&rate),"zero target refused");
    ck(!n48dto0_encode(120,0,0,&phase,&mod,&rate),"missing reference refused");
    ck(!n48dto0_encode(120,720000000,0,nullptr,&mod,&rate),"missing output refused");
    ck(n48dto0_decode(0,123,720000000,0)==0,"disabled DTO refused");
    ck(n48dto0_decode(0x10,720000000,720000000,0)==0,"unnormalized phase refused");
    ck(n48dto0_decode(0x10,0,0,2)==0,"zero modulo refused");
    ck(n48dto0_decode(0xffffffff,123,720000000,0)==0,"unreadable control refused");
    ck(n48dto0_decode(0x10,0xffffffff,720000000,0)==0,"unreadable phase refused");
    ck(n48dto0_decode(0x10,123,0xffffffff,0)==0,"unreadable modulo refused");
    ck(n48dto0_decode(0x10,123,720000000,0xffffffff)==0,"unreadable divider refused");
    std::printf("DTO clock: %u checks, %u failures; no hardware access\n",n,bad);return bad?1:0;
}
