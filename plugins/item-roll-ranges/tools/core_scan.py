import sys, struct
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent / 'python-libs'))
import pefile, capstone
p=pefile.PE('D2RCore.dll'); base=p.OPTIONAL_HEADER.ImageBase
for e in p.DIRECTORY_ENTRY_EXPORT.symbols:
    name=(e.name or b'').decode()
    if any(x in name.lower() for x in ['range','tooltip','input','statroll']): print('export',hex(e.address),name)
for s in p.sections:
    data=s.get_data(); start=s.VirtualAddress
    for key in [b'item_stat_ranges',b'possible stat rolls',b'GetAsyncKeyState',b'GetKeyState']:
        pos=data.find(key)
        if pos>=0: print('string',key,hex(start+pos))
if len(sys.argv)>1:
    target=int(sys.argv[1],0)
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
    for s in p.sections:
        if not s.Characteristics & 0x20000000:continue
        for i in md.disasm(s.get_data(),s.VirtualAddress):
            for o in i.operands:
                if o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP and i.address+i.size+o.mem.disp==target:
                    print('xref',hex(i.address),i.mnemonic,i.op_str)
