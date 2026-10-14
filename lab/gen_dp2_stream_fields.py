import pathlib,re,json,hashlib
P=pathlib.Path(__file__).resolve().parent
ofs=(P.parent/'references/dcn_4_1_0_offset.h').read_text();masks=(P.parent/'references/dcn_4_1_0_sh_mask.h').read_text()
fields={
 'DP2_DP_VID_STREAM_CNTL':['DP_VID_STREAM_ENABLE','DP_VID_STREAM_STATUS','DP_VID_STREAM_DIS_DEFER'],
 'DP2_DP_STEER_FIFO':['DP_STEER_FIFO_ENABLE','DP_STEER_FIFO_RESET'],
 'DP2_DP_VID_TIMING':['DP_VID_M_N_GEN_EN','DP_VID_N_INTERVAL'],
 'DP2_DP_VID_N':['DP_VID_N'],'DP2_DP_VID_M':['DP_VID_M'],
 'DP2_DP_PIXEL_FORMAT':['PIXEL_ENCODING_TYPE','UNCOMPRESSED_PIXEL_FORMAT','UNCOMPRESSED_COMPONENT_DEPTH','COMPRESSED_PIXEL_FORMAT'],
 'DP2_DP_MSA_COLORIMETRY':['DP_MSA_MISC0'],
 'DIG2_DIG_FIFO_CTRL0':['DIG_FIFO_ENABLE','DIG_FIFO_RESET','DIG_FIFO_READ_START_LEVEL','DIG_FIFO_RESET_DONE','DIG_FIFO_OUTPUT_PIXEL_PER_CYCLE']}
def value(s,n):
 m=re.search(r'^#define\s+'+re.escape(n)+r'\s+(0x[0-9a-fA-F]+|[0-9]+)[LU]*\s*$',s,re.M)
 if not m:raise RuntimeError(n)
 return int(m[1],0)
out=['// Generated from official AMD Linux 238650ef6c7c DCN4.1 register headers.','#pragma once','#include <stdint.h>','namespace Dp120 {namespace StreamFields {'];data={}
for r,fs in fields.items():
 assert value(ofs,'reg'+r+'_BASE_IDX')==2
 label=r.split('_',1)[1];addr=0x34c0+value(ofs,'reg'+r);out.append('constexpr uint32_t '+label+'=0x%xu;'%addr);data[label]={'abs_dword':addr,'fields':{}}
 for f in fs:
  mask=value(masks,r+'__'+f+'_MASK');shift=value(masks,r+'__'+f+'__SHIFT')
  out+=['constexpr uint32_t '+f+'_MASK=0x%xu;'%mask,'constexpr unsigned '+f+'_SHIFT=%du;'%shift]
  data[label]['fields'][f]={'mask':mask,'shift':shift}
out.append('}}')
(P/'dp2_stream_fields.hpp').write_text('\n'.join(out)+'\n')
data['source_hashes']={'offset':hashlib.sha256(ofs.encode()).hexdigest(),'mask':hashlib.sha256(masks.encode()).hexdigest()}
(P/'dp2-stream-fields.json').write_text(json.dumps(data,indent=2)+'\n')
print('Generated',len(fields),'DP2/DIG2 stream registers from official headers')
