#!/usr/bin/env python3
"""Build a private full .pkg from a verified local graphics stack. No downloads."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, os, plistlib, shutil, subprocess, tarfile, tempfile

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source-root',type=Path,default=Path('/'))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();root=Path(__file__).resolve().parent
    assert subprocess.check_output(['/usr/bin/sw_vers','-buildVersion'],text=True).strip()=='25G332'
    spec=importlib.util.spec_from_file_location('stack',root/'stack_runtime.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
    manifest=json.loads((root/'manifest.json').read_text())
    for n,h in manifest.items():assert hashlib.sha256((args.source_root/n).read_bytes()).hexdigest()==h,'Local component differs: '+n
    assert not args.output.exists(), 'Output exists; choose a new filename'
    with tempfile.TemporaryDirectory(prefix='hp250-pkg-') as temporary:
        work=Path(temporary);scripts=work/'scripts';scripts.mkdir();data=work/'data';payload=data/'payload'
        for n in m.TARGETS:m.copy_checked(args.source_root/n,payload/n)
        for name in ['manifest.json','modules.json','stack_runtime.py']:shutil.copyfile(root/name,data/name)
        shutil.copytree(root/'Licenses',data/'Licenses')
        sla = Path('/System/Library/CoreServices/Setup Assistant.app/Contents/Resources/en.lproj/OSXSoftwareLicense.html')
        if sla.is_file():
            shutil.copyfile(sla, data/'Licenses/macOS-Software-License.html')
        with tarfile.open(scripts/'payload.tar.gz','w:gz') as archive:archive.add(data,arcname='stack')
        shutil.copyfile(root/'preinstall',scripts/'preinstall');shutil.copyfile(root/'postinstall',scripts/'postinstall')
        for n in ['preinstall','postinstall']:(scripts/n).chmod(0o755)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        subprocess.run(['/usr/bin/pkgbuild','--nopayload','--scripts',str(scripts),'--identifier','local.hp250.nootedgreen.systemstack','--version','2026.11.14','--install-location','/',str(args.output.resolve())],check=True)
    print('Private full graphics package:',args.output.resolve())
    print('Not Developer ID signed. Requires Python 3, current compatible NootedGreen EFI and build 25G332. No EFI is installed.')

if __name__=='__main__':main()
