"""Strict parsers and admission checks for the single-primary display startup."""
import re
def idle_console(uid,who):
    return uid in (0,88) and not who.strip()
def status_ok(text,already=False):
    values=re.findall(r'^\s*status\s*:\s*(\d+)\s*\(',text,re.M)
    return len(values)==1 and int(values[0]) in ((0,11) if already else (0,)) and 'FAILED (0x' not in text
def adopt_ok(text):
    return status_ok(text,True) and bool(re.search(r'aux traces: pipes ours / all.*:\s*1 / 1,',text)) and 'BAR0 kernel mapping     : WRITE-COMBINED' in text
def stamp_count(text):
    m=re.findall(r'^\s*stamps written .*:\s*(\d+)\s*$',text,re.M)
    return int(m[0]) if len(m)==1 else None
def gpu_healthy(text,require_open=False):
    m=re.search(r'open / DEAD slots / HUNG:\s*(\d+) / (\d+) / (\d+)',text)
    return bool(m and (int(m[1])>0 or not require_open) and m[2]=='0' and m[3]=='0')
