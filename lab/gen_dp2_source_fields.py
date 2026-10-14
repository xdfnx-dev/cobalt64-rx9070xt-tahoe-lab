import pathlib,re,hashlib,json
p=pathlib.Path(__file__).resolve().parent
ofs=(p.parent/'references/dcn_4_1_0_offset.h').read_text()
masks=(p.parent/'references/dcn_4_1_0_sh_mask.h').read_text()
regs=['DP_LINK_CNTL','DP_CONFIG','DP_DPHY_CNTL','DP_DPHY_TRAINING_PATTERN_SEL','DP_DPHY_PRBS_CNTL','DP_DPHY_SCRAM_CNTL','DP_LINK_FRAMING_CNTL','DP_DPHY_INTERNAL_CTRL']
fields={'DP_LINK_CNTL':['DP_LINK_TRAINING_COMPLETE'],'DP_CONFIG':['DP_UDI_LANES'],'DP_DPHY_CNTL':['DPHY_BYPASS'],'DP_DPHY_TRAINING_PATTERN_SEL':['DPHY_TRAINING_PATTERN_SEL'],'DP_DPHY_PRBS_CNTL':['DPHY_PRBS_EN'],'DP_DPHY_SCRAM_CNTL':['DPHY_SCRAMBLER_ADVANCE','DPHY_SCRAMBLER_BS_COUNT'],'DP_LINK_FRAMING_CNTL':['DP_IDLE_BS_INTERVAL','DP_VBID_DISABLE','DP_VID_ENHANCED_FRAME_MODE'],'DP_DPHY_INTERNAL_CTRL':['DPHY_ALT_SCRAMBLER_RESET_EN','DPHY_ALT_SCRAMBLER_RESET_SEL']}
def value(text,name):
 m=re.search(r'^#define\s+'+re.escape(name)+r'\s+(0x[0-9a-fA-F]+|[0-9]+)[LU]*\s*$',text,re.M)
 if not m:raise RuntimeError(name)
 return int(m[1],0)
out=['// Generated from AMD Linux 238650ef6c7c DCN4.1 offset/sh_mask headers.','// Exact DP2 only; absolute BAR5 dwords. No I/O.','#pragma once','#include <stdint.h>','namespace Dp120 { namespace SourceFields {']
manifest={}
for r in regs:
 n='regDP2_'+r;idx=value(ofs,n+'_BASE_IDX');assert idx==2
 addr=0x34c0+value(ofs,n);out.append('constexpr uint32_t '+r+' = 0x%xu;'%addr);manifest[r]={'abs_dword':addr,'fields':{}}
 for f in fields[r]:
  pre='DP2_'+r+'__'+f
  mask=value(masks,pre+'_MASK');shift=value(masks,pre+'__SHIFT')
  out += ['constexpr uint32_t '+f+'_MASK = 0x%xu;'%mask,'constexpr unsigned '+f+'_SHIFT = %du;'%shift]
  manifest[r]['fields'][f]={'mask':mask,'shift':shift}
out.append('} }')
(p/'dp2_source_fields.hpp').write_text('\n'.join(out)+'\n')
manifest['sources']={n:hashlib.sha256(t.encode()).hexdigest() for n,t in [('dcn_4_1_0_offset.h',ofs),('dcn_4_1_0_sh_mask.h',masks)]}
(p/'dp2-source-fields.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Generated',len(regs),'DP2 addresses from both official headers')
