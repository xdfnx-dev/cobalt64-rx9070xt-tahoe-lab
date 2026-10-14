#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include "RDNA4FB/src/dp_survey.hpp"
static unsigned checks=0,failures=0;
static void ck(bool b,const char *s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
struct Mock {
    unsigned calls=0,segment=0,pos=0,resets=0,native=0;
    int failAt=-1; bool defer=false,shortRead=false,badChecksum=false,extended=true,oldExtRevision=false;
    std::vector<uint32_t> addresses;
    DpSurvey::Reply operator()(uint8_t action,uint32_t address,const uint8_t *tx,uint8_t n,uint8_t *rx){
        unsigned call=calls++;addresses.push_back(address);
        ck(n<=16,"bounded AUX payload");
        ck(action!=0x80,"no native DPCD write");
        if(action==DpSurvey::Read){
            ++native;ck(address==0 || address==0x100 || address==0x200 || address==0x2200,"fixed native READ address");
        } else ck(address==0x30 || address==0x50,"I2C touches segment/EDID only");
        if(action==DpSurvey::I2cWrite && address==0x30 && tx && *tx==0){++resets;segment=0;}
        if(defer)return {2,0};
        if((int)call==failAt)return {-1,0};
        if(action==DpSurvey::I2cWriteMot && address==0x30)segment=*tx;
        if(action==DpSurvey::I2cWriteMot && address==0x50)pos=*tx;
        if(rx){
            std::memset(rx,0,n);
            if(address==0 && n==16){rx[0]=0x12;rx[1]=20;rx[2]=4;rx[14]=extended?0x82:2;}
            if(address==0x2200 && n==16){rx[0]=oldExtRevision?0x11:0x14;rx[1]=30;rx[2]=4;}
            if(address==0x50){ck(segment==1,"third block uses segment one");
                if(pos==0)rx[0]=0x70;
                if(pos==112)rx[15]=badChecksum?0:0x90;
                pos+=n;
            }
        }
        return {0,(uint8_t)(shortRead && (action&0x10) && n?n-1:n)};
    }
};
static DpSurvey::Result run(Mock &m,uint8_t inst,const uint8_t *base){
    return DpSurvey::capture(inst,base,[&](uint8_t a,uint32_t p,const uint8_t *t,uint8_t n,uint8_t *r){return m(a,p,t,n,r);});
}
int main(int argc,char **argv){
    uint8_t base[128]{};FILE*f=argc==2?std::fopen(argv[1],"rb"):nullptr;if(!f)return 2;
    size_t n=std::fread(base,1,128,f);std::fclose(f);if(n!=128)return 2;
    ck(DpSurvey::target(2,base),"actual Acer EDID admitted");
    Mock m;auto r=run(m,2,base);ck(r.valid==31 && r.segmentReset,"complete capture accepted");
    ck(r.calls==m.calls && m.calls==15,"bounded positive request count");
    ck(m.native==4 && m.resets==1 && m.segment==0,"native reads plus final segment reset");
    for(unsigned inst=0;inst<4;inst++)if(inst!=2){Mock x;auto z=run(x,inst,base);ck(!z.valid && !x.calls,"wrong AUX touches nothing");}
    uint8_t wrong[128];std::memcpy(wrong,base,128);wrong[10]^=1;
    unsigned total=0;for(unsigned i=0;i<127;i++)total+=wrong[i];wrong[127]=(uint8_t)(-total);
    ck(DpSurvey::checksum(wrong),"wrong-model negative retains valid EDID checksum");
    Mock w;ck(!run(w,2,wrong).valid && !w.calls,"wrong monitor touches nothing");
    std::memcpy(wrong,base,128);wrong[54]^=1;Mock c;ck(!run(c,2,wrong).valid && !c.calls,"bad base checksum touches nothing");
    Mock nil;ck(!run(nil,2,nullptr).valid && !nil.calls,"missing EDID touches nothing");
    for(int fail=4;fail<=13;fail++){
        Mock x;x.failAt=fail;auto z=run(x,2,base);
        ck(!(z.valid&8),"failed third-block transaction refused");
        ck(x.resets==1 && x.segment==0,"every extension failure attempts segment zero");
    }
    Mock shorty;shorty.shortRead=true;r=run(shorty,2,base);
    ck(!r.valid && r.segmentReset,"short native/EDID reads not published");
    Mock sum;sum.badChecksum=true;r=run(sum,2,base);ck(r.valid==23 && r.segmentReset,"bad extension checksum not published");
    Mock d;d.defer=true;r=run(d,2,base);ck(!r.valid && !r.segmentReset,"DEFER exhaustion not reported successful");
    ck(d.calls==42,"all DEFER retries bounded including STOP/reset");
    Mock reset;reset.failAt=14;r=run(reset,2,base);ck(!(r.valid&8) && !r.segmentReset,"reset failure prevents extension publication");
    Mock noext;noext.extended=false;r=run(noext,2,base);ck(r.valid==15 && noext.native==3,"no extended read without presence bit");
    Mock oldext;oldext.oldExtRevision=true;r=run(oldext,2,base);ck(r.valid==15,"older extended revision not published");
    Mock failedExt;failedExt.failAt=1;r=run(failedExt,2,base);ck(r.valid==15,"failed extended read not claimed successful");
    std::printf("DP survey: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
