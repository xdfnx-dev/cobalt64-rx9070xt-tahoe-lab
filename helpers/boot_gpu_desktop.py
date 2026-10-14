#!/Library/Developer/CommandLineTools/usr/bin/python3
"""One primary framebuffer only. Arm at the login window, never over user work."""
import pathlib,subprocess,os,time,sys,hashlib,json,re
from desktop_core import idle_console,status_ok,adopt_ok,stamp_count,gpu_healthy
BASE=pathlib.Path('/usr/local/navi48');LOG=pathlib.Path('/Library/Logs/Navi48');SUP=pathlib.Path('/Library/Application Support/Navi48')
CLI=str(BASE/'navi48test');NUB=str(BASE/'n48nub');PRE=True;ARMED=False;RENAMED=False;RESTARTED=False
def output(msg):print(time.strftime('%Y-%m-%d %H:%M:%S')+' '+msg,flush=True)
def raw(cmd,timeout=25):
    p=subprocess.run(cmd,capture_output=True,text=True,timeout=timeout)
    output('$ '+' '.join(cmd)+'\n'+p.stdout+p.stderr+'\nexit='+str(p.returncode))
    return p
def users_idle():
    return idle_console(os.stat('/dev/console').st_uid,subprocess.check_output(['/usr/bin/who'],text=True))
def guard():
    if PRE and not users_idle():raise RuntimeError('User logged in: stopping before further display operations')
def step(args,check):
    guard();p=raw([CLI,'accel']+args)
    if p.returncode or not check(p.stdout+p.stderr):raise RuntimeError('Driver refused '+repr(args))
    return p.stdout+p.stderr
def marker(name):
    p=pathlib.Path('/private/tmp')/name
    assert not p.is_symlink()
    if p.exists():assert p.stat().st_uid==0
    p.touch();os.chmod(p,0o644)
def recover(reason):
    output('RECOVERY: '+reason)
    if not (ARMED or RENAMED or RESTARTED):
        output('No display arming/rename performed: no recovery mutations.');return
    for name in ['n48m-off','n48m-noflip']:marker(name)
    try:
        if ARMED:raw([CLI,'accel','pipearm','0'])
        if RENAMED:raw([CLI,'accel','fbname','0'])
        pathlib.Path('/private/tmp/n48m-headless-no').unlink(missing_ok=True)
        # One fallback restart is allowed ONLY at the login window. It closes
        # the native client (restoring its console) and loads a GPU-free device.
        if RESTARTED and users_idle():raw(['/usr/bin/killall','-9','WindowServer'])
    except Exception as exc:output('Recovery error: '+str(exc))
def main():
    global PRE,ARMED,RENAMED,RESTARTED
    assert os.geteuid()==0
    args=set(subprocess.check_output(['/usr/sbin/sysctl','-n','kern.bootargs'],text=True).split())
    required={'rdna4-cobalt=1','navi48-metal-ws=1','navi48-metal-disp=1','navi48-native=1','navi48-metal=1','navi48-multisession=1','amfi_get_out_of_my_way=1'}
    if not required.issubset(args):output('Desktop profile absent: nothing done.');return
    assert subprocess.check_output(['/usr/bin/sw_vers','-buildVersion'],text=True).strip()=='25G241'
    if (SUP/'desktop-off').exists():output('Persistent desktop kill switch: nothing done.');return
    boot=subprocess.check_output(['/usr/sbin/sysctl','-n','kern.boottime'],text=True).strip()
    last=SUP/'desktop-last-boot'
    if last.exists() and last.read_text()==boot:output('Already attempted this boot: nothing done.');return
    manifest=BASE/'desktop-manifest.json'
    assert not manifest.is_symlink() and manifest.stat().st_uid==0 and not manifest.stat().st_mode&0o022
    for name,want in json.loads(manifest.read_text()).items():
        p=pathlib.Path(name);assert not p.is_symlink() and p.stat().st_uid==0 and not p.stat().st_mode&0o022
        assert hashlib.sha256(p.read_bytes()).hexdigest()==want,name
    cache_manifest=BASE/'windowserver-cache-manifest.json'
    assert not cache_manifest.is_symlink() and cache_manifest.stat().st_uid==0 and not cache_manifest.stat().st_mode&0o022
    cache=json.loads(cache_manifest.read_text());assert len(cache)==104
    for name,want in cache.items():
        p=pathlib.Path(name)
        assert p.parent==pathlib.Path('/private/var/tmp/n48m-spv') and not p.is_symlink()
        assert p.stat().st_uid==4294967294 and not p.stat().st_mode&0o022
        assert hashlib.sha256(p.read_bytes()).hexdigest()==want,'Required WindowServer shader unavailable: '+name
    output('Known WindowServer shader preflight: 52 shaders/104 files verified.')
    retry_file=BASE/'desktop-retry-markers.json'
    if retry_file.exists():
        assert str(retry_file) in json.loads(manifest.read_text()),'Retry ticket must be pinned by manifest'
        retry=json.loads(retry_file.read_text())
        assert int(re.search(r'sec = (\d+)',boot).group(1))>retry['boot_sec'],'Retry is permitted only after reboot'
        for name,meta in retry['markers'].items():
            assert name in ['n48m-off','n48m-noflip','n48m-headless-no','n48m-starts']
            guard();p=pathlib.Path('/private/tmp')/name
            if not p.exists():continue
            assert not p.is_symlink() and p.stat().st_uid==meta.get('owner',0) and p.stat().st_mtime_ns==meta['mtime_ns']
            assert hashlib.sha256(p.read_bytes()).hexdigest()==meta['sha256']
            p.unlink();output('Cleared unchanged recovery marker from prior boot: '+name)
    for _ in range(45):
        guard()
        b=subprocess.check_output(['/usr/sbin/ioreg','-r','-c','Navi48Bringup','-d1','-l'],text=True)
        f=subprocess.check_output(['/usr/sbin/ioreg','-r','-c','RDNA4FB','-d1','-l'],text=True)
        if 'POSITIVE PASS (fault leg REFUSED)' in b and '"Navi48,StageResult" = 0' in b and 'registered, matched, active' in f:break
        time.sleep(2)
    else:raise RuntimeError('Bringup/framebuffer did not become ready')
    assert len(re.findall(r'^\+-o RDNA4FB ',f,re.M))==1,'Exactly one primary framebuffer required'
    for _ in range(30):
        guard()
        if subprocess.run(['/usr/bin/pgrep','-x','WindowServer'],capture_output=True).returncode==0:break
        time.sleep(2)
    else:raise RuntimeError('No login-window WindowServer')
    time.sleep(5);guard();last.write_text(boot)
    for name in ['n48m-off','n48m-noflip','n48m-presentall','n48m-headless-no']:
        if (pathlib.Path('/private/tmp')/name).exists():raise RuntimeError('GPU control marker present: '+name)
    loaded=subprocess.check_output(['/usr/bin/kmutil','showloaded'],stderr=subprocess.STDOUT,text=True)
    if 'com.navi48.accelprobe (0.0.7)' not in loaded:
        p=raw(['/usr/bin/kmutil','load','--load-style','start-only','--bundle-path','/Library/Extensions/Navi48Accel-25G241-20261013.kext'])
        if p.returncode:raise RuntimeError('Aux kext not available')
        loaded=subprocess.check_output(['/usr/bin/kmutil','showloaded'],stderr=subprocess.STDOUT,text=True)
    for uuid in ['5FA6358D-643F-3B75-A119-17B90ECC576C','D7D4106A-FF18-3D3D-8C17-4957AE640050','E80B51A2-70EF-3D51-A42F-860C1422BA32']:
        # During approval, either exact previously tested framebuffer is safe.
        assert uuid in loaded or (uuid == '5FA6358D-643F-3B75-A119-17B90ECC576C' and '57775865-5B7E-3C3B-B94C-26D755C60E01' in loaded) or (uuid == 'E80B51A2-70EF-3D51-A42F-860C1422BA32' and '86DF2639-BDE2-3A39-ABB5-088057F4C007' in loaded),'Different GPU kernel binary: '+uuid
    guard();p=raw([NUB,'status'])
    if p.returncode:
        guard();p=raw([NUB,'publish'])
        if p.returncode:raise RuntimeError('Metal nub publish refused')
    a=subprocess.check_output(['/usr/sbin/ioreg','-r','-c','Navi48Accelerator','-d1'],text=True)
    assert 'registered, matched, active' in a,'Runtime accelerator ABI probe failed'
    for name in ['metal_probe','mtlprobe-x86_64','Discord','Discord Helper','Discord Helper (','ghostty']:
        guard();p=raw([CLI,'accel','appallow','add',name])
        if p.returncode:raise RuntimeError('Diagnostic application admission failed: '+name)
    step(['pipeadopt'],adopt_ok)
    step(['pipeagdc','1'],lambda x: bool(re.search(r'agdc status\s*:\s*0 \(PUBLISHED\)',x)) and 'REFUSED' not in x)
    RENAMED=True
    step(['fbname','1'],lambda x: 'class name NOW          : AMDRDNA4' in x and bool(re.search(r'armed\s*:\s*1 \(status 0\)',x)))
    fb=subprocess.check_output(['/usr/sbin/ioreg','-r','-c','RDNA4FB','-d1'],text=True)
    assert '<class AMDRDNA4FB,' in fb,'Full framebuffer class rename not observed'
    guard();marker('n48m-headless-no')
    ARMED=True
    step(['pipearm','1'],lambda x:status_ok(x) and bool(re.search(r'accel\+0xccf now\s*:\s*1\s',x)))
    baseline=stamp_count(step(['pipestat','4'],status_ok));assert baseline is not None
    step(['pipereload'],lambda x:status_ok(x) and bool(re.search(r'restart window\s*:\s*OPEN',x)))
    guard();old_pid=subprocess.check_output(['/usr/bin/pgrep','-x','WindowServer'],text=True).strip()
    p=raw(['/usr/bin/killall','-9','WindowServer']);assert p.returncode==0
    RESTARTED=True;PRE=False
    for _ in range(30):
        p=subprocess.run(['/usr/bin/pgrep','-x','WindowServer'],capture_output=True,text=True)
        new_pid=p.stdout.strip()
        if new_pid and new_pid!=old_pid:break
        time.sleep(0.5)
    else:raise RuntimeError('WindowServer did not restart')
    subprocess.Popen(['/Library/Developer/CommandLineTools/usr/bin/python3',str(BASE/'desktop_watch.py'),new_pid],start_new_session=True)
    # Stay at the login window for the first trial. No restart if a user enters.
    evidence=[]
    for _ in range(18):
        time.sleep(5)
        s=raw([CLI,'accel','pipestat','4']);g=raw([CLI,'accel','sessstat','4'])
        n=stamp_count(s.stdout);evidence.append({'stamps':n,'healthy':gpu_healthy(g.stdout)})
        if not gpu_healthy(g.stdout):raise RuntimeError('GPU session health failed')
        if n is not None and n>baseline and gpu_healthy(g.stdout,require_open=True):
            ws=raw(['/usr/bin/log','show','--last','45s','--style','compact','--predicate','process == "WindowServer" AND eventMessage CONTAINS "Navi48Metal"'],40)
            (LOG/'windowserver-metal.txt').write_text(ws.stdout+ws.stderr)
            if 'vkQueueSubmit = 0' in ws.stdout and ('native open' in ws.stdout or 'scanout:' in ws.stdout):
                (LOG/'desktop-result.json').write_text(json.dumps({'boot':boot,'result':'GPU submit and pipe stamps observed','evidence':evidence},indent=2))
                output('GPU submit and display pipe activity observed. Further desktop/app validation required.');return
    (LOG/'desktop-result.json').write_text(json.dumps({'boot':boot,'result':'unverified; no complete GPU/pipe evidence','evidence':evidence},indent=2))
    raise RuntimeError('No complete GPU/pipe evidence within 90 seconds')
if __name__=='__main__':
    try:main()
    except Exception as exc:
        recover(str(exc));sys.exit(1)
