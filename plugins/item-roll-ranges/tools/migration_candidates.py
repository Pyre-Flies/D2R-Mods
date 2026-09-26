"""Read-only migration candidates: mask only encoded RIP/branch relocations.
Candidates require manual semantic review; this never generates runtime profiles.
"""
import re,sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import capstone,pefile
p=pefile.PE(sys.argv[1]); md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
src=(Path(__file__).parents[1]/'src/provider_profile.h').read_text()
for name,body in re.findall(r'unsigned char (\w+)\[\] = \{(.*?)\};',src,re.S):
 b=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]+)',body)); mask=bytearray(b'\xff'*len(b))
 for i in md.disasm(b,0):
  if any(o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP for o in i.operands):
   mask[i.address+i.disp_offset:i.address+i.disp_offset+i.disp_size]=b'\0'*i.disp_size
  if i.group(capstone.CS_GRP_JUMP) or i.group(capstone.CS_GRP_CALL):
   if i.imm_size:mask[i.address+i.imm_offset:i.address+i.imm_offset+i.imm_size]=b'\0'*i.imm_size
 pattern=b''.join(re.escape(bytes([x])) if m else b'.' for x,m in zip(b,mask))
 found=[]
 for s in p.sections:
  if s.Characteristics&0x20000000:
   found += [hex(s.VirtualAddress+m.start()) for m in re.finditer(pattern,s.get_data(),re.S)]
 print(name,len(b),found)
