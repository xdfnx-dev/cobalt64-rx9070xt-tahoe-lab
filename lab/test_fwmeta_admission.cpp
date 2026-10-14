#include <cstdio>
#include <initializer_list>
#include "Cobalt64/src/navi48-bringup/src/dcn/navi48_dispread.h"
int main(){unsigned bad=0;for(uint64_t a=0;a<65536;a++){bool expected=a<=25||a==0x80||a==0x81;if((bool)n48dr_dmub_arg_ok(a)!=expected)bad++;}
 for(uint64_t a:{0x100000081ull,0x8000000000000081ull,0xffffffffffffffffull})if(n48dr_dmub_arg_ok(a))bad++;
 printf("Firmware-reader API admission: 65539 checks, %u failures; no GPU access\n",bad);return bad?1:0;}
