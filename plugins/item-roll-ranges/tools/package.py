"""Build clean standalone runtime and source ZIPs from explicit file lists."""
from pathlib import Path
import argparse, hashlib, re, zipfile
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--output',type=Path,default=root/'dist')
args=parser.parse_args()
version=re.search(r'"item-roll-ranges", "Item Roll Ranges", "([^"]+)"',(root/'src/plugin.cpp').read_text()).group(1)
args.output.mkdir(parents=True,exist_ok=True)
name=f'Item-Roll-Ranges-{version}-PyreFly'
dll=root/'build/Item Roll Ranges.dll'
if not dll.is_file(): raise SystemExit('Build the plugin before packaging.')
compat='''Item Roll Ranges by PyreFly
Plugin version: {version}
D2RLoader plugin ABI: 4 (Windows x64 client)
Required D2RCore.dll SHA-256:
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2

The plugin also validates live code witnesses before activation.
No Controller QOL or external hook library is required.
'''.format(version=version)
runtime={
 'd2rloader/plugins/Item Roll Ranges.dll':dll.read_bytes(),
 'README.md':(root/'docs/DISTRIBUTION-README.md').read_bytes(),
 'COMPATIBILITY.txt':compat.encode(),
}
def archive(path,files):
    files=dict(files)
    files['SHA256SUMS']=''.join(hashlib.sha256(data).hexdigest()+'  '+n+'\n' for n,data in sorted(files.items())).encode()
    with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
        for n,data in sorted(files.items()):z.writestr(n,data)
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        for line in z.read('SHA256SUMS').decode().splitlines():
            digest,n=line.split('  ',1)
            assert hashlib.sha256(z.read(n)).hexdigest()==digest
        assert set(z.namelist())==set(files)
    print(path)
archive(args.output/(name+'.zip'),runtime)
source={}
for directory in ['src','tests','docs']:
    for p in (root/directory).rglob('*'):
        if p.is_file():source[p.relative_to(root).as_posix()]=p.read_bytes()
for n in ['CMakeLists.txt','README.md','.gitignore','provider-audit.json','formatter-audit.json']:
    source[n]=(root/n).read_bytes()
for n in ['package.py','audit_provider.py','audit_formatter.py','audit_affixes.py','disasm.py','core_scan.py','live_code.py','migration_candidates.py','migration_live.py','audit_property_sources.py','audit_contributions.py','audit_ed_identity.py','audit_sunder_ranges.py']:
    source['tools/'+n]=(root/'tools'/n).read_bytes()
sdk=root/'sdk' if (root/'sdk/include').exists() else root.parent/'PluginSDK-master'
for p in (sdk/'include').rglob('*'):
    if p.is_file():source['sdk/'+p.relative_to(sdk).as_posix()]=p.read_bytes()
source['sdk/LICENSE']=(sdk/'LICENSE').read_bytes()
archive(args.output/(name+'-Source.zip'),source)
print('DLL SHA256:',hashlib.sha256(dll.read_bytes()).hexdigest())
