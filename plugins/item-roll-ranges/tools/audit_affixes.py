"""Read-only live validation of the affix profile; never writes game memory."""
import ctypes as c
import csv, json, struct, sys
from pathlib import Path
pid=int(sys.argv[1]); base=int(sys.argv[2],0)
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[c.c_ulong,c.c_int,c.c_ulong]; k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x1010,False,pid)
if not h: raise c.WinError(c.get_last_error())
def read(a,n):
    assert 0<n<=4*1024*1024
    b=c.create_string_buffer(n)
    if not k.ReadProcessMemory(h,a,b,n,None): raise c.WinError(c.get_last_error())
    return b.raw
def q(a):return struct.unpack('<Q',read(a,8))[0]
root=Path(__file__).resolve().parents[1]
try:
    table=q(base+0x2a9a580+2*16)
    rows=q(table+0x15e8); count=q(table+0x15f0)
    partitions=[q(table+x) for x in (0x1600,0x1608,0x1610)]
    assert rows==partitions[0] and all((p-rows)%140==0 for p in partitions)
    excel=Path('mods/Reimagined/Reimagined.mpq/data/global/excel')
    def txt(name):
        with (excel/(name+'.txt')).open(encoding='utf-8-sig') as f:return list(csv.DictReader(f,delimiter='\t'))
    suffix,prefix=txt('magicsuffix'),txt('magicprefix')
    # Blank/end marker rows count towards the game's native row IDs.
    affixes=suffix+prefix
    assert len(suffix)==(partitions[1]-rows)//140
    assert len(affixes)==(partitions[2]-rows)//140
    fields={'version':(0x22,'H'),'spawnable':(0x54,'B'),'level':(0x58,'I'),
            'group':(0x5c,'I'),'maxlevel':(0x60,'I'),'rare':(0x64,'B'),
            'levelreq':(0x65,'B'),'frequency':(0x82,'B')}
    checked=0
    for i,row in enumerate(affixes):
        if not row['name']:continue
        b=read(rows+i*140,140)
        assert b[:32].split(b'\0')[0].decode()==row['name'][:31],(i,row['name'])
        for key,(off,fmt) in fields.items():
            assert struct.unpack_from('<'+fmt,b,off)[0]==int(row[key] or 0),(i,key)
        for slot in range(3):
            for key,off in [('min',0x2c),('max',0x30)]:
                assert struct.unpack_from('<i',b,off+16*slot)[0]==int(row[f'mod{slot+1}{key}'] or 0),(i,slot,key)
        checked+=1
    props=txt('properties'); stats=txt('itemstatcost')
    statids={r['Stat']:int(r['*ID']) for r in stats if r['*ID']}
    p=q(table+0x240); pc=q(table+0x248)
    for row in props:
        if not row['*Id']:continue
        i=int(row['*Id']); assert i<pc
        b=read(p+i*48,48)
        for slot in range(7):
            assert b[0x18+slot]==int(row[f'func{slot+1}'] or 0),(i,slot,'func')
            expected=statids.get(row[f'stat{slot+1}'],65535)
            assert struct.unpack_from('<H',b,0x20+slot*2)[0]==expected,(i,slot,'stat')
    regions=[('Tables',0x300a90,0x300ad3),('ItemData',0x34a52b,0x34a535),
             ('AffixA',0x36cdc2,0x36cdd2),('AffixB',0x36ce52,0x36ce62),
             ('Eligible',0x3d4220,0x3d4335),('Single',0x2d6520,0x2d6530),
             ('PropertyRow',0x3d329d,0x3d32ad)]
    header=['// Generated from read-only live validation by audit_affixes.py.','#pragma once','#include <cstddef>','namespace AffixProfile {']
    for name,start,end in regions:
        b=read(base+start,end-start)
        header += [f'inline constexpr std::size_t {name}Rva=0x{start:x};',f'inline constexpr unsigned char {name}[]={{']
        header += ['    '+','.join(f'0x{x:02x}' for x in b[i:i+16])+',' for i in range(0,len(b),16)]
        header += ['};']
    header += ['struct Witness { std::size_t rva; const unsigned char* bytes; std::size_t size; };','inline constexpr Witness Witnesses[]={']
    header += [f'    {{{n}Rva,{n},sizeof({n})}},' for n,_,_ in regions]
    header += ['};','}']
    if '--generate' in sys.argv:(root/'src/affix_profile.h').write_text('\n'.join(header)+'\n')
    print(json.dumps({'affix_rows':count,'verified_prefix_suffix_rows':checked,'properties':pc,
                      'partition_row_offsets':[(x-rows)//140 for x in partitions],
                      'fields':fields,'status':'PASS: loaded tables compared to active mod text and native code captured'},indent=2))
finally:k.CloseHandle(h)
