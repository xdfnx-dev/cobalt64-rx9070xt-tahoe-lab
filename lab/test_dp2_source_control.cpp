#include <cstdio>
#include <map>
#include <vector>
#include <utility>
#include "dp2_source_control.hpp"
static unsigned checks=0,failures=0;
static void check(bool b,const char*m){++checks;if(!b){++failures;std::printf("FAIL %s\n",m);}}
struct Fake {
 std::map<uint32_t,uint32_t> regs;
 std::vector<std::pair<uint32_t,uint32_t>> writes;
 unsigned ops=0,failAt=0;bool allOnes=false;
 bool read(uint32_t a,uint32_t*v){++ops;if(ops==failAt)return false;*v=allOnes?0xffffffffu:regs[a];return true;}
 bool writeMaskedVerified(uint32_t a,uint32_t mask,uint32_t v){
  ++ops;if(ops==failAt)return false;auto old=regs[a];regs[a]=(old&~mask)|v;
  check((regs[a]&~mask)==(old&~mask),"non-target fields preserved");writes.push_back({a,v});return true;
 }
};
static Dp120::SourceProof proof(){Dp120::SourceProof p;p.beforeLogin=p.exclusiveDisplay=p.firmwareAbiVerified=p.transportVerified=p.sourceTps4=true;p.phy=p.dig=p.aux=2;p.hpd=3;return p;}
int main(){
 using namespace Dp120;using namespace SourceFields;
 check(DP_LINK_CNTL==0x5826 && DP_CONFIG==0x5829 && DP_DPHY_CNTL==0x5835 &&
       DP_DPHY_TRAINING_PATTERN_SEL==0x5836 && DP_DPHY_PRBS_CNTL==0x583b &&
       DP_DPHY_SCRAM_CNTL==0x583c && DP_LINK_FRAMING_CNTL==0x5831 && DP_DPHY_INTERNAL_CTRL==0x582d,
       "DP2 addresses independently fixed to physical DIG2 block");
 check(DP_LINK_TRAINING_COMPLETE_MASK==0x10 && DPHY_BYPASS_MASK==0x10000 &&
       DPHY_TRAINING_PATTERN_SEL_MASK==3,"reference field masks");
 auto p=proof();
 for(unsigned pat: {0u,1u,4u}){
  Fake x;for(auto a:{DP_CONFIG,DP_LINK_CNTL,DP_DPHY_CNTL,DP_DPHY_TRAINING_PATTERN_SEL,DP_DPHY_PRBS_CNTL,DP_DPHY_SCRAM_CNTL,DP_LINK_FRAMING_CNTL,DP_DPHY_INTERNAL_CTRL})x.regs[a]=0xa5a5a5a5;
  SourceControl<Fake> s(x,p);check(s.pattern(pat),"source pattern completes");
  if(pat){check(x.writes.size()==4 && x.writes[0].first==0x5836 && x.writes[0].second==(pat==1?0u:3u) && x.writes[1].first==0x5826 && x.writes[1].second==0,"training pattern before clearing complete");}
  else check(x.writes.size()==6 && x.writes[3].first==0x5826 && x.writes[3].second==0x10,"video framing prepared before training-complete");
  unsigned total=x.ops;
  for(unsigned f=1;f<=total;f++){Fake y;y.failAt=f;SourceControl<Fake> t(y,p);check(!t.pattern(pat) && y.ops==f,"first I/O failure stops without later writes");}
 }
 for(unsigned i=0;i<9;i++){
  auto q=p;switch(i){case 0:q.beforeLogin=false;break;case 1:q.exclusiveDisplay=false;break;case 2:q.firmwareAbiVerified=false;break;case 3:q.transportVerified=false;break;case 4:q.sourceTps4=false;break;case 5:q.phy=1;break;case 6:q.dig=1;break;case 7:q.aux=1;break;case 8:q.hpd=2;break;}
  Fake x;SourceControl<Fake> s(x,q);check(!s.pattern(1) && !s.configureFourLanes() && x.ops==0,"unproven ABI, active desktop or wrong physical connector no I/O");
 }
 for(unsigned pat:{2u,3u,7u,255u}){Fake x;SourceControl<Fake>s(x,p);check(!s.pattern(pat)&&!x.ops,"DPCD encodings cannot be mistaken for supported hardware pattern");}
 Fake bad;bad.allOnes=true;SourceControl<Fake> bs(bad,p);check(!bs.pattern(1)&&bad.writes.empty(),"unreadable MMIO never written");
 Fake four;SourceControl<Fake> fs(four,p);check(fs.configureFourLanes() && four.writes[0]==std::make_pair(0x5829u,3u),"four lanes source side");
 std::printf("DP2 source control: %u checks, %u failures; simulated MMIO, no live GPU writes\n",checks,failures);return failures?1:0;
}
