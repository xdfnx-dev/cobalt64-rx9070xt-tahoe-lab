"""Restore headless defaults when WindowServer changes again after the trial."""
import subprocess,pathlib,time,sys,os
assert os.geteuid()==0
last=sys.argv[1]
while True:
    p=subprocess.run(['/usr/bin/pgrep','-x','WindowServer'],capture_output=True,text=True)
    now=p.stdout.strip()
    if now and now!=last:
        for name in ['n48m-headless-no','n48m-starts']:
            p=pathlib.Path('/private/tmp')/name
            if not p.is_symlink():p.unlink(missing_ok=True)
        print('WindowServer changed '+last+' -> '+now+'; headless default restored.',flush=True)
        last=now
    time.sleep(1)
