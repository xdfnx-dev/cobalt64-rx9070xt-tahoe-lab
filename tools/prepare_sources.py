"""Fetch pinned upstream Cobalt64, apply public local changes. No installation."""
import argparse, pathlib, subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
PIN='d6c9aedf8e6526a39e349b5886cbc8c7ddf77f65'
p=argparse.ArgumentParser();p.add_argument('--cobalt64-source',help='Optional existing upstream Git checkout')
a=p.parse_args();dest=ROOT/'vendor/Cobalt64'
assert not dest.exists(), 'Use a fresh vendor directory; refusing to overwrite sources'
dest.parent.mkdir(exist_ok=True)
subprocess.run(['git','clone','--no-checkout',a.cobalt64_source or 'https://github.com/Almosst-DEV/Cobalt64.git',str(dest)],check=True)
subprocess.run(['git','-C',str(dest),'checkout','--detach',PIN],check=True)
assert subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()==PIN
subprocess.run(['git','-C',str(dest),'apply','--check',str(ROOT/'patches/cobalt64-local.patch')],check=True)
subprocess.run(['git','-C',str(dest),'apply',str(ROOT/'patches/cobalt64-local.patch')],check=True)
print('Pinned public source patched. Private Apple ABI and external build dependencies are still required for the full stack.')
