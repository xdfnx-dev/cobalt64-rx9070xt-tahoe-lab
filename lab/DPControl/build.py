import pathlib, plistlib, subprocess, hashlib, json
ROOT=pathlib.Path(__file__).resolve().parents[2]
HERE=pathlib.Path(__file__).resolve().parent
SDK=pathlib.Path(subprocess.check_output(['xcrun','--sdk','macosx','--show-sdk-path'],text=True).strip())
import os
MKSDK=pathlib.Path(os.environ['MACKERNELSDK'])
GPU=ROOT/'vendor/Cobalt64/src/navi48-bringup'
DCN=ROOT/'vendor/Cobalt64/src/dcn41'
B=HERE/'build';B.mkdir(exist_ok=True)
K=B/'Navi48DPControl.kext';EX=K/'Contents/MacOS/Navi48DPControl';EX.parent.mkdir(parents=True,exist_ok=True)
common=['-arch','x86_64','-target','x86_64-apple-macos11.0','-isysroot',str(SDK),
 '-I'+str(MKSDK/'Headers'),'-I'+str(GPU/'src'),'-I'+str(GPU/'src/amd'),'-I'+str(DCN),
 '-I'+str(ROOT/'vendor/Cobalt64/src/xlat12'),'-DKERNEL','-DKERNEL_PRIVATE','-DDRIVER_PRIVATE','-DAPPLE','-DNeXT',
 '-D_FORTIFY_SOURCE=0','-nostdinc','-fno-builtin','-fno-common','-fno-stack-protector','-mkernel','-fapple-kext','-Wall','-Wextra','-Os','-g']
subprocess.run(['clang++']+common+['-std=c++17','-fno-exceptions','-fno-rtti','-fcheck-new','-fno-c++-static-destructors',
 '-c',str(HERE/'DPControl.cpp'),'-o',str(B/'DPControl.o')],check=True)
subprocess.run(['clang']+common+['-c',str(HERE/'kmod_info.c'),'-o',str(B/'kmod_info.o')],check=True)
subprocess.run(['clang++','-arch','x86_64','-target','x86_64-apple-macos11.0','-isysroot',str(SDK),'-nostdlib','-Xlinker','-kext',
 str(B/'DPControl.o'),'-L'+str(MKSDK/'Library/x86_64'),'-lkmodc++',str(B/'kmod_info.o'),'-lkmod','-Wl,-no_deduplicate','-o',str(EX)],check=True)
p={'CFBundleDevelopmentRegion':'en','CFBundleExecutable':'Navi48DPControl','CFBundleIdentifier':'com.navi48.dpcontrol',
 'CFBundleInfoDictionaryVersion':'6.0','CFBundleName':'Navi48DPControl','CFBundlePackageType':'KEXT','CFBundleVersion':'0.0.2',
 'CFBundleShortVersionString':'0.0.2','OSBundleCompatibleVersion':'0.0.1','IOKitPersonalities':{'Navi48DPControl':{
 'CFBundleIdentifier':'com.navi48.dpcontrol','IOClass':'Navi48DPControl','IOProviderClass':'Navi48Bringup',
 'IOMatchCategory':'Navi48DPControl','IOProbeScore':100}},'OSBundleLibraries':{
 'com.navi48.bringup':'0.0.665','com.apple.kpi.iokit':'20.0.0','com.apple.kpi.libkern':'20.0.0',
 'com.apple.kpi.mach':'20.0.0','com.apple.kpi.unsupported':'20.0.0'}}
(K/'Contents/Info.plist').write_bytes(plistlib.dumps(p))
subprocess.run(['codesign','--force','--sign','-',str(K)],check=True)
subprocess.run(['codesign','--verify','--strict',str(K)],check=True)
subprocess.run(['clang','-arch','x86_64','-Wall','-Wextra',str(HERE/'query.c'),'-framework','IOKit','-framework','CoreFoundation','-o',str(B/'dp-query')],check=True)
receipt={'kext':str(K),'executable_sha256':hashlib.sha256(EX.read_bytes()).hexdigest(),
 'loaded_dependency_uuid':'5FA6358D-643F-3B75-A119-17B90ECC576C','physical_120_enabled':False}
(B/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('Built signed DPControl; fixed query only, no display mode writes.')
