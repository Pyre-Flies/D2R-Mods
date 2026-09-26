"""Check the actual installed layout against the resolver's name contract.

Usage: python audit_materials_layout.py --layout <bankexpansionlayouthd.json>
The old BankPanel type-as-name lookup must fail this check.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--layout', type=Path, required=True)
parser.add_argument('--source', type=Path, default=Path(__file__).resolve().parents[1] / 'src/materials_actions.cpp')
args = parser.parse_args()
data = args.layout.read_bytes()
layout = json.loads(data.decode('utf-8-sig'))
source = args.source.read_text()
match = re.search(r'FindTopLevelPanelByNameFn>\(ctx,0x846170\)\("([^"]+)"\)', source)
assert match, 'Cannot identify resolver lookup; review audit after refactor'
assert match[1] == layout['name'], f"Resolver name {match[1]!r} differs from layout name {layout['name']!r}"
children = {child['name']: child for child in layout['children']}
checked = {}
for category, codes in {'gems': ['gcg'], 'materials': ['rvl', 'rvs'], 'runes': ['r22']}.items():
    container = children['advancedstash_' + category]
    slots = {child['name']: child for child in container['children']}
    for code in codes:
        assert slots[code]['type'] == 'AdvancedStashSlotWidget', (category, code)
    checked[category] = codes
print(json.dumps({'layout': str(args.layout.resolve()), 'sha256': hashlib.sha256(data).hexdigest(),
                  'lookup_name': match[1], 'root_type': layout['type'], 'direct_slots_checked': checked}, indent=2))
