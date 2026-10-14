#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ABI.h"
int main(int argc,char**argv){
 if(geteuid()!=0){fprintf(stderr,"Root required\n");return 1;}
 if(argc!=2 || (strcmp(argv[1],"census") && strcmp(argv[1],"fw"))){fprintf(stderr,"Usage: %s census|fw\n",argv[0]);return 2;}
 io_service_t s=IOServiceGetMatchingService(kIOMainPortDefault,IOServiceMatching("Navi48DPControl"));
 if(!s){fprintf(stderr,"DPControl not loaded\n");return 3;}
 io_connect_t c=0;kern_return_t kr=IOServiceOpen(s,mach_task_self(),N48DP_CLIENT_TYPE,&c);IOObjectRelease(s);
 if(kr){fprintf(stderr,"Open: %#x\n",kr);return 4;}
 struct n48dp_report r;memset(&r,0,sizeof r);size_t n=sizeof r;
 kr=IOConnectCallStructMethod(c,!strcmp(argv[1],"fw")?N48DP_SEL_FW_VERSION:N48DP_SEL_CENSUS,NULL,0,&r,&n);IOServiceClose(c);
 printf("DPControl ABI=%u op=%u kr=%#x size=%zu status=%u FW=%#x GPINT=%#x->%#x polls=%u SCRATCH0=%#x\n",r.abi,r.op,kr,n,r.status,r.fw_version,r.gpint_before,r.gpint_after,r.polls,r.scratch0);
 printf("DP2 PIXEL_FORMAT=%#x MSA_COLORIMETRY=%#x STREAM=%#x LINK=%#x OTG0 frames=%u->%u\n",r.pixel_format,r.msa_colorimetry,r.stream,r.link,r.frame_before,r.frame_after);
 for(unsigned i=0;i<2;i++)printf("CW%u base=%#x top=%#x offset=%#x:%08x VRAM=%#llx bytes=%llu\n",i,r.window[i].base,r.window[i].top,r.window[i].offset_high,r.window[i].offset,(unsigned long long)r.window[i].vram_offset,(unsigned long long)r.window[i].size);
 return kr || n!=sizeof r || r.abi!=N48DP_ABI || r.status?5:0;
}
