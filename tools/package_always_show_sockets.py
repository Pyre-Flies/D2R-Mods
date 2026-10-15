"""Package the standalone socket plugin after verifying the exact DLL."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[1]
NAMES={'d2rloader/plugins/Always Show Sockets.dll','defaults/always-show-sockets.toml','README.md'}
MACHINE=re.compile(r'(?<![A-Za-z0-9])[A-Za-z]:[/\\]|\\\\[A-Za-z0-9][^\s\\]*\\|/(?:Users|home)/',re.I)
def privacy(name,data):
    regions=[data]
    if name.endswith('.dll'):
        assert data[:2]==b'MZ','Invalid DLL'
        pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0','Invalid PE'
        count=struct.unpack_from('<H',data,pe+6)[0]
        table=pe+24+struct.unpack_from('<H',data,pe+20)[0]
        regions=[]
        for off in range(table,table+count*40,40):
            section=data[off:off+8].split(b'\0')[0]
            size,start=struct.unpack_from('<II',data,off+16)
            flags=struct.unpack_from('<I',data,off+36)[0]
            if flags&0x40 and section not in (b'.rsrc',b'.pdata',b'.xdata',b'.reloc'):
                assert start+size<=len(data)
                regions.append(data[start:start+size])
    for region in regions:
        texts=[region.decode('utf8',errors='replace')]+[region[n:].decode('utf-16-le',errors='replace') for n in (0,1)]
        if any(MACHINE.search(s) for s in texts):raise ValueError('Machine path in '+name)
    from package_runtime import check_public_text
    check_public_text(name,data)
def validate_version(version,tag=None):
    beta=bool(re.fullmatch(r'\d+\.\d+\.\d+-beta\.\d+',version))
    stable=bool(re.fullmatch(r'\d+\.\d+\.\d+\+rev\.\d+',version))
    if not (beta or stable):raise ValueError('Expected loader version + revision or beta version')
    prefix='beta-always-show-sockets-' if beta else 'always-show-sockets-'
    if tag and tag!=prefix+version:raise ValueError('Tag/version/channel mismatch')

def package(dll,verifier,output,tag=None,root=ROOT):
    plugin=root/'plugins/always-show-sockets'
    version=re.search(r'\.version="([^"]+)"',(plugin/'src/plugin.cpp').read_text()).group(1)
    validate_version(version,tag)
    original=dll.read_bytes()
    subprocess.run([str(verifier.resolve()),str(dll.resolve())],check=True)
    if dll.read_bytes()!=original:raise ValueError('DLL changed during verification')
    from package_runtime import make_archive,runtime_readme
    files={'d2rloader/plugins/Always Show Sockets.dll':original,
           'defaults/always-show-sockets.toml':(plugin/'always-show-sockets.toml').read_bytes(),
           'README.md':runtime_readme(plugin,root)}
    assert files.keys()==NAMES
    for name,data in files.items():privacy(name,data)
    output.mkdir(parents=True,exist_ok=True)
    archive=output/('Always-Show-Sockets-'+version+'-PyreFly.zip')
    make_archive(archive,files)
    verify(archive)
    (output/'SHA256SUMS').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n',encoding='utf8')
    return archive
def verify(archive,expected_sha256=None):
    if expected_sha256 is not None and hashlib.sha256(archive.read_bytes()).hexdigest()!=expected_sha256:
        raise ValueError('Archive checksum mismatch')
    with zipfile.ZipFile(archive) as z:
        if set(z.namelist())!=NAMES or len(z.namelist())!=len(NAMES):raise ValueError('Unexpected contents')
        if z.testzip():raise ValueError('ZIP corruption')
        for name in NAMES:privacy(name,z.read(name))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--dll',type=Path,required=True);p.add_argument('--artifact-verifier',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--tag')
    a=p.parse_args();print(package(a.dll,a.artifact_verifier,a.output,a.tag))
