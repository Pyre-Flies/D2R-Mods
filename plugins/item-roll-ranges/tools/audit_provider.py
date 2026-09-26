"""Read-only audit of the exact D2RCore provider used by this plugin."""
import hashlib, json, struct, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent / 'python-libs'))
import pefile
path=Path(sys.argv[1] if len(sys.argv)>1 else 'D2RCore.dll')
data=path.read_bytes()
expected='2a868d013d2e0830bd2d9e04b918b19e46a73cf726c833e70d089b948fdeb5a2'
assert hashlib.sha256(data).hexdigest()==expected, 'Unqualified D2RCore binary'
pe=pefile.PE(data=data)
exports={e.name.decode():e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
assert exports['BuildItemTooltipWithStatRanges']==0x819530
assert exports['CopyWideListStat']==0x832940
assert pe.get_data(0x8329bf,4)==bytes.fromhex('49 c1 e4 04')
assert pe.get_data(0x3d8930,4)==bytes.fromhex('41 8b 70 08')
sites=[(0x8195da,0x6fe3b0),(0x819954,0x6fe470),(0x328986,0x702ae0),(0x3e80ac,0x704400),(0x3e80f7,0x704400)]
for call,slot in sites:
    b=pe.get_data(call,6)
    assert b[:2]==b'\xff\x15'
    assert call+6+struct.unpack_from('<i',b,2)[0]==slot
    assert slot%8==0
assert pe.get_data(0x819646,7)==bytes.fromhex('41 83 e6 02 41 d1 ee')
assert pe.get_data(0x81994e,6)==bytes.fromhex('41 b8 00 08 00 00')
assert pe.get_data(0x328981,5)==bytes.fromhex('b9 19 00 00 00')
regions=[('Selection',0x819587,0x8196cc),('ControllerCall',0x819949,0x81996a),('PanelWitness',0x328981,0x328991),('WideCopy',0x832940,0x832a32),('WideRead',0x3d88b1,0x3d8934),('WideRangeCaller',0x3e807c,0x3e80b9),('WideActual',0x3e7e82,0x3e7e9d),('PropertiesOverlay',0x81d4b0,0x81d549),('OverlayScope',0x81a2d4,0x81a378)]
regions += [('AffixSelection',0x81544a,0x815506),('SingleObserverCaller',0x3e82bb,0x3e82ea)]
# Semantic witnesses: TLS index and overlay field, property ABI and affix eligibility.
assert pe.get_data(0x81d4f9,7)==bytes.fromhex('4d 8b b6 40 18 00 00')
assert 0x81d4ec+struct.unpack('<i',pe.get_data(0x81d4e8,4))[0]==0x7df224
assert 0x81d51c+struct.unpack('<i',pe.get_data(0x81d518,4))[0]==0x704490
assert 0x8157de+struct.unpack('<i',pe.get_data(0x8157da,4))[0]==0x6ff370
regions += [('EligibleCaller',0x8157d2,0x8157e3),('ActualPairCaller',0x3e80bb,0x3e8102)]
if '--generate' in sys.argv:
    output=['// Generated from the attested D2RCore image by tools/audit_provider.py.','#pragma once','#include <cstddef>','namespace ProviderProfile {']
    for name,start,end in regions:
        output += [f'inline constexpr std::size_t {name}Rva = 0x{start:x};',f'inline constexpr unsigned char {name}[] = {{']
        b=pe.get_data(start,end-start)
        output += ['    '+','.join(f'0x{x:02x}' for x in b[i:i+16])+',' for i in range(0,len(b),16)]
        output += ['};']
    output += ['}']
    (Path(__file__).parents[1]/'src/provider_profile.h').write_text('\n'.join(output)+'\n')
print(json.dumps({'image':str(path.resolve()),'sha256':expected,'export_rva':'0x819530','calls':[
    {'call_rva':hex(c),'return_rva':hex(c+6),'pointer_slot_rva':hex(s)} for c,s in sites],
    'result':'PASS: static call, slot, selection and panel witnesses; runtime behavior not attested'},indent=2))
