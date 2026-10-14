#include <cstdio>
#include <cstring>
#include <vector>
#include "link_training.hpp"
static unsigned checks=0,bad=0;
static void ck(bool b,const char*s){++checks;if(!b){++bad;std::printf("FAIL %s\n",s);}}
struct Mock {
    unsigned calls=0,fail=0,restores=0,reads=0;bool restoreOk=true,measurement=true;
    bool stuckCr=false,lostCr=false,stuckEq=false;
    std::vector<uint8_t>patterns;
    bool step(){return ++calls!=fail;}
    bool snapshot(){return step();} bool blank(){return step();}
    bool configureSourceHbr3(){return step();}
    bool sourcePattern(uint8_t p){patterns.push_back(p);return step();}
    bool sourceLanes(uint8_t vs,uint8_t pe){ck(vs<=3 && pe<=3-vs,"legal combined source drive");return step();}
    bool auxWrite(uint32_t a,const uint8_t*p,unsigned n){
        ck(a==0x100 || a==0x108 || a==0x102,"only training DPCD addresses");
        ck(n<=5,"bounded AUX writes");
        if(a==0x100)ck(n==2 && p[0]==30 && p[1]==0x84,"HBR3 four lanes enhanced framing");
        if(a==0x108)ck(n==1 && p[0]==1,"8b10b coding");
        if(a==0x102 && n==5)ck(p[0]==0x21 || p[0]==7,"TPS1 scrambled off or TPS4 scrambled on");
        return step();
    }
    void waitInterval(){}
    bool auxReadStatus(uint8_t*s){
        ++reads;std::memset(s,0,6);
        if(!stuckCr){s[0]=s[1]=reads==1?0x11:0x77;s[2]=reads==1?0:1;}
        if(lostCr && reads>1)s[0]=0x76;
        if(stuckEq && reads>1){s[0]=s[1]=0x11;s[2]=0;}
        return step();
    }
    bool applyValidatedClockTiming(){return step();} bool unblank(){return step();}
    bool measure120(){return step() && measurement;}
    bool restoreVerified(){++restores;return restoreOk;}
};
int main(){
    Dp120::Admission a;a.backendVerified=a.beforeLogin=a.directLinkVerified=a.exactAcerTiming=a.sourceTps4=a.sinkTps4=true;
    a.maxRate=30;a.lanes=4;a.aux=a.dig=2;a.otg=0;
    Mock m;auto r=Dp120::trial(m,a);ck(r.status==Dp120::Ok && r.restored && m.restores==1,"positive simulated trial restores once");
    unsigned calls=m.calls;ck(m.patterns==std::vector<uint8_t>({1,4,0}),"source TPS order");
    for(unsigned fail=1;fail<=calls;fail++){
        Mock x;x.fail=fail;r=Dp120::trial(x,a);ck(r.status!=Dp120::Ok,"every failed callback prevents success");
        ck(x.restores==(fail==1?0u:1u),"no snapshot => no writes; otherwise exactly one restore");
    }
    auto b=a;b.backendVerified=false;Mock x;r=Dp120::trial(x,b);ck(!x.calls && r.status==Dp120::Denied,"missing hardware backend touches nothing");
    b=a;b.beforeLogin=false;Mock y;r=Dp120::trial(y,b);ck(!y.calls,"logged-in desktop cannot be mutated");
    b=a;b.dig=1;Mock z;r=Dp120::trial(z,b);ck(!z.calls,"author DP1 topology refused");
    b=a;b.maxRate=20;Mock low;r=Dp120::trial(low,b);ck(!low.calls,"HBR2 cannot start 4K120 trial");
    Mock cr;cr.stuckCr=true;r=Dp120::trial(cr,a);ck(r.status==Dp120::ClockRecoveryFailed && r.crPolls==5 && cr.restores==1,"stuck clock recovery bounded and restored");
    Mock eq;eq.stuckEq=true;r=Dp120::trial(eq,a);ck(r.status==Dp120::EqualizationFailed && r.eqPolls==6 && eq.restores==1,"stuck EQ bounded and restored");
    Mock lost;lost.lostCr=true;r=Dp120::trial(lost,a);ck(r.status==Dp120::EqualizationFailed && lost.restores==1,"lost CR in EQ restored");
    Mock rate;rate.measurement=false;r=Dp120::trial(rate,a);ck(r.status==Dp120::MeasurementFailed && rate.restores==1,"wrong measured frequency cannot pass");
    Mock restore;restore.restoreOk=false;r=Dp120::trial(restore,a);ck(r.status==Dp120::RestoreFailed && !r.restored,"failed restore not presented as successful");
    std::printf("DP training protocol prototype: %u checks, %u failures; SIMULATED backend only\n",checks,bad);return bad?1:0;
}
