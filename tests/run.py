"""ASan/UBSan host models. Never opens a GPU service or writes hardware."""
import pathlib, subprocess, tempfile, shutil, json
ROOT=pathlib.Path(__file__).resolve().parents[1]
assert (ROOT/'vendor/Cobalt64/src').is_dir(), 'Run python3 tools/prepare_sources.py first'
LAB=ROOT/'lab'
cases=['test_aux2_native','test_dp2_source_control','test_dp2_stream_control','test_link_training',
       'test_dto_clock','test_fwmeta_scan','test_fwmeta_admission','test_survey','DPControl/test_fw_query']
results=[]
with tempfile.TemporaryDirectory(prefix='cobalt-host-tests-') as tmp:
    inc=pathlib.Path(tmp)/'include/RDNA4FB';inc.parent.mkdir(parents=True);inc.symlink_to(ROOT/'rdna4fb-overlay')
    for case in cases:
        exe=pathlib.Path(tmp)/pathlib.Path(case).name
        cmd=['clang++','-std=c++17','-Wall','-Wextra','-fsanitize=address,undefined','-fno-omit-frame-pointer','-O1',
             '-I'+str(ROOT/'vendor'),'-I'+str(ROOT/'vendor/Cobalt64/src/dcn41'),
             '-I'+str(pathlib.Path(tmp)/'include'),str(LAB/(case+'.cpp')),'-o',str(exe)]
        subprocess.run(cmd,check=True)
        args=[str(exe)]
        if case=='test_survey':
            # Synthetic valid EDID header + Acer product ID; no real monitor serial.
            base=bytearray(128);base[:12]=bytes([0,255,255,255,255,255,255,0,4,0x72,0x6a,0x0f])
            base[126]=2
            base[127]=(-sum(base[:127]))&255
            fixture=pathlib.Path(tmp)/'synthetic-edid.bin';fixture.write_bytes(base)
            args.append(str(fixture))
        r=subprocess.run(args,capture_output=True,text=True,check=True)
        print(case+': '+r.stdout.strip())
        results.append({'case':case,'exit':r.returncode,'output':r.stdout.strip(),'hardware_access':False})
(ROOT/'tests/results.json').write_text(json.dumps(results,indent=2)+'\n')
