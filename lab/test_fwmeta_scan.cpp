#include <cstdio>
#include <vector>
#include <cstring>
#include "Cobalt64/src/navi48-bringup/src/dcn/navi48_fwmeta_scan.h"
static unsigned checks=0,bad=0;static void ck(bool x,const char*m){++checks;if(!x){++bad;printf("FAIL %s\n",m);}}
struct E{std::vector<uint32_t>w;unsigned calls=0,fail=0;uint64_t clock=0,tick=0;E():w(0x200000/4,0){}
 bool read(uint64_t a,uint32_t*p,uint32_t n){++calls;ck(n>0&&n<=64&&!(a&3)&&a+(uint64_t)n*4<=w.size()*4,"bounded aligned serialized-reader contract");if(calls==fail)return false;memcpy(p,w.data()+a/4,n*4);return true;}
 uint64_t now_us(){clock+=tick;return clock;}
 void meta(unsigned at){uint32_t m[8]={0x444d5542,0x10000,0x1000,0x10300,0,0x400,0,0};memcpy(w.data()+at/4,m,32);}
};
int main(){
 for(unsigned pos:{0x800u,0x8fcu,0x900u,0xfdcu}){E e;e.meta(pos);auto r=n48fw::scan(e,0x1000,e.w.size()*4);ck(r.status==n48fw::Found&&r.address==pos&&r.words[3]==0x10300,"candidate across chunk boundary decoded");unsigned calls=e.calls;
 for(unsigned f=1;f<=calls;f++){E x;x.meta(pos);x.fail=f;auto q=n48fw::scan(x,0x1000,x.w.size()*4);ck(q.status==n48fw::ReadFailed&&x.calls==f,"reader failure stops without candidate");}}
 for(uint64_t a:{0ull,128ull,257ull,0x200004ull}){E e;auto r=n48fw::scan(e,a,e.w.size()*4);ck(r.status==n48fw::InvalidRange&&!e.calls,"invalid anchor no memory access");}
 E empty;auto r=n48fw::scan(empty,0x180000,empty.w.size()*4);ck(r.status==n48fw::NotFound&&r.chunks==4096,"scan at most one MiB");
 E outside;outside.meta(0x100);r=n48fw::scan(outside,0x180000,outside.w.size()*4);ck(r.status==n48fw::NotFound,"candidate outside scan window inaccessible");
 E tail;tail.meta(0xfe4);r=n48fw::scan(tail,0x1000,tail.w.size()*4);ck(r.status==n48fw::NotFound,"metadata extending beyond anchor not read");
 E invalid;invalid.meta(0x800);invalid.w[(0x800+12)/4]=0;r=n48fw::scan(invalid,0x1000,invalid.w.size()*4);ck(r.status==n48fw::NotFound,"zero version not a candidate");
 E delay;delay.tick=200000;r=n48fw::scan(delay,0x180000,delay.w.size()*4);ck(r.status==n48fw::Deadline&&r.chunks<=10,"elapsed-time deadline");
 printf("Serialized FW metadata reader: %u checks, %u failures; host model only\n",checks,bad);return bad?1:0;
}
