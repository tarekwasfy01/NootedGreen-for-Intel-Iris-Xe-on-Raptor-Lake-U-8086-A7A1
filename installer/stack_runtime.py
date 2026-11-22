#!/usr/bin/env python3
"""Install only the explicitly enumerated HP graphics components, with rollback."""
from pathlib import Path
import datetime, hashlib, json, os, plistlib, shutil, subprocess, sys

def inventory(path):
    if not path.exists(): return None
    if path.is_file(): return {'.': hashlib.sha256(path.read_bytes()).hexdigest()}
    result = {}
    for p in path.rglob('*'):
        if p.is_symlink(): raise RuntimeError('Symlink refused: '+str(p))
        if p.is_file(): result[p.relative_to(path).as_posix()] = hashlib.sha256(p.read_bytes()).hexdigest()
    return result

def remove(path):
    if path.is_symlink(): raise RuntimeError('Symlink target refused: '+str(path))
    if path.is_dir(): shutil.rmtree(path)
    elif path.exists(): path.unlink()

def copy_checked(src, dst):
    before = inventory(src)
    assert before is not None and not dst.exists()
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.is_dir(): shutil.copytree(src, dst, copy_function=shutil.copyfile)
    else: shutil.copyfile(src, dst)
    assert inventory(dst) == before == inventory(src), 'Copy verification failed: '+str(src)

def validate_manifest(payload, manifest):
    assert inventory(payload) == manifest, 'Payload checksum mismatch'

def allowed_target(name):
    return name in TARGETS

TARGETS = [
 'Library/Extensions/AppleIntelTGLGraphics.kext',
 'Library/Extensions/AppleIntelTGLGraphicsFramebuffer.kext',
 'Library/Extensions/AppleIntelTGLGraphicsMTLDriver.bundle',
 'Library/Extensions/AppleIntelTGLGraphicsGLDriver.bundle',
 'Library/Extensions/AppleIntelTGLGraphicsVADriver.bundle',
 'Library/Extensions/AppleIntelTGLGraphicsVAME.bundle',
 'Library/Extensions/AppleIntelGraphicsShared.bundle',
 'Library/Extensions/HookCase.kext',
 'Library/GPUBundles/AppleIntelTGLGraphicsMTLDriver.bundle',
 'Library/GPUBundles/AppleIntelGraphicsShared.bundle',
 'System/Library/Caches/xe.bundle']

def restore(backup):
    state = json.loads((backup/'state.json').read_text())
    assert all(allowed_target(n) or n == 'Library/KernelCollections' for n in state['system'])
    # Verify every saved byte before deleting any installed component.
    for n, digest in state['system'].items():
        assert inventory(backup/'system'/n) == digest, 'Corrupt backup: '+n
    for n, digest in state['preboot'].items():
        assert '..' not in Path(n).parts and Path(n).name == 'KernelCollections'
        assert inventory(backup/'preboot'/n) == digest
    for n,digest in state['system'].items():
        dst=Path('/')/n;remove(dst)
        if digest is not None:copy_checked(backup/'system'/n,dst)
    for n in state['preboot']:
        dst=Path('/System/Volumes/Preboot')/n;remove(dst);copy_checked(backup/'preboot'/n,dst)
    # Restore original ownership/modes for backed-up files and directories.
    for name, props in state['permissions'].items():
        p=Path(name)
        if p.exists():os.chmod(p,props['mode']);os.chown(p,props['uid'],props['gid'])
    os.sync()

def main():
    assert os.geteuid()==0, 'Run with sudo'
    assert subprocess.check_output(['/usr/bin/sw_vers','-buildVersion'],text=True).strip()=='25G332', 'Only verified for macOS build 25G332'
    if sys.argv[1]=='restore':
        restore(Path(sys.argv[2]));print('Saved graphics files and kernel collections restored. Reboot required.');return
    assert sys.argv[1]=='install'
    folder=Path(sys.argv[2]);payload=folder/'payload';manifest=json.loads((folder/'manifest.json').read_text())
    validate_manifest(payload,manifest)
    assert all(any(n==t or n.startswith(t+'/') for t in TARGETS) for n in manifest)
    for n in TARGETS:
        assert (payload/n).is_dir(), 'Component missing: '+n
        subprocess.run(['/usr/bin/codesign','--verify','--deep','--strict',str(payload/n)],check=True)
    backup=Path('/Library/Application Support/HP250Graphics/Backups')/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    backup.mkdir(parents=True,mode=0o700)
    state={'system':{},'preboot':{},'permissions':{}}
    def save(src,dst):
        digest=inventory(src)
        if digest is not None:
            for p in [src,*src.rglob('*')] if src.is_dir() else [src]:
                st=p.stat();state['permissions'][str(p)]=dict(mode=st.st_mode&0o7777,uid=st.st_uid,gid=st.st_gid)
            copy_checked(src,dst)
        return digest
    for n in TARGETS+['Library/KernelCollections']:
        state['system'][n]=save(Path('/')/n,backup/'system'/n)
    preboot=Path('/System/Volumes/Preboot');assert preboot.is_mount(), 'Preboot must be mounted'
    collections=[p for p in preboot.rglob('KernelCollections') if p.is_dir()]
    assert collections, 'Preboot kernel collections not found'
    for p in collections:
        n=p.relative_to(preboot).as_posix();state['preboot'][n]=save(p,backup/'preboot'/n)
    (backup/'state.json').write_text(json.dumps(state,indent=2))
    shutil.copyfile(__file__,backup/'RESTORE.py')
    (backup/'README.txt').write_text('Private backup. Restore on this same running macOS with sudo python3 RESTORE.py restore BACKUP_DIRECTORY. EFI not included or changed. No reboot performed automatically.\n')
    print('Verified private backup:',backup,flush=True)
    try:
        for n in TARGETS:
            dst=Path('/')/n;stage=dst.with_name(dst.name+'.hp250-new')
            assert not stage.exists(), 'Old staging path exists: '+str(stage)
            copy_checked(payload/n,stage);remove(dst);stage.rename(dst)
            for p in [dst,*dst.rglob('*')]:
                os.chown(p,0,0)
                os.chmod(p,0o755 if p.is_dir() or p.parent.name=='MacOS' else 0o644)
        modules=['HookCase.kext','AppleIntelTGLGraphicsFramebuffer.kext','AppleIntelTGLGraphics.kext']
        command=['/usr/bin/kmutil','load','-z','--load-style','match-only']
        for n in modules:command+=['-p','/Library/Extensions/'+n]
        result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (backup/'kmutil-install.log').write_text(result.stdout)
        assert result.returncode==0 or (result.returncode==28 and 'requires a reboot' in result.stdout.lower()), result.stdout
        inspection=subprocess.check_output(['/usr/bin/kmutil','inspect','-A','/Library/KernelCollections/AuxiliaryKernelExtensions.kc','--show-kext-uuids'],text=True,stderr=subprocess.STDOUT)
        (backup/'kc-inspection.txt').write_text(inspection)
        metadata=json.loads((folder/'modules.json').read_text())
        for item in metadata:
            assert any(item['id'] in line and item['uuid'].lower() in line.lower() for line in inspection.splitlines()), 'Required module/UUID absent from Auxiliary KC: '+item['id']
        for n,digest in manifest.items():assert hashlib.sha256((Path('/')/n).read_bytes()).hexdigest()==digest,n
    except BaseException:
        print('Installation failed; restoring verified previous files and collections.',flush=True)
        restore(backup);raise
    os.sync();(backup/'INSTALL-SUCCESS').write_text('All payload hashes and Auxiliary KC module UUIDs verified.\n')
    print('Graphics stack installed. Reboot through the matching NootedGreen EFI. EFI and unrelated system components unchanged.')

if __name__=='__main__':main()
