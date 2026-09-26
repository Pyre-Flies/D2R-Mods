"""Read-only comparison of decrypted PE code against reviewed Shared-page evidence.

Does not regenerate admission bytes. After a patch, review the handler ABI and
call graph before updating shared_page_signatures.h or the evidence registry.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--image', type=Path, required=True)
p.add_argument('--evidence', type=Path, default=Path(__file__).resolve().parents[1] / 'docs/shared-page-evidence.json')
args = p.parse_args()
data = args.image.read_bytes()
pe = struct.unpack_from('<I', data, 0x3C)[0]
assert data[pe:pe+4] == b'PE\0\0'
count = struct.unpack_from('<H', data, pe+6)[0]
optional = struct.unpack_from('<H', data, pe+20)[0]
sections = [struct.unpack_from('<IIII', data, pe+24+optional+40*i+8) for i in range(count)]
results = []
for site in json.loads(args.evidence.read_text())['sites']:
    rva, size = int(site['rva'], 16), site['size']
    found = None
    for virtual_size, va, raw_size, raw in sections:
        if va <= rva and rva+size <= va+raw_size:
            found = data[raw+rva-va:raw+rva-va+size]
            break
    matched = found is not None and found.hex() == site['bytes']
    results.append({'name': site['name'], 'rva': site['rva'], 'matches_reviewed_code': matched})
print(json.dumps({'image_sha256': hashlib.sha256(data).hexdigest(), 'sites': results}, indent=2))
raise SystemExit(0 if all(x['matches_reviewed_code'] for x in results) else 1)
