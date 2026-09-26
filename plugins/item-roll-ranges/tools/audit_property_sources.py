"""Read-only native property audit; records disassembly and item affix metadata."""
import ctypes as c,struct,sys,json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import capstone
k=c.WinDLL('kernel32');k.OpenProcess.restype=c.c_void_p;k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p];h=k.OpenProcess(0x410,0,int(sys.argv[1]))
def read(a,n):
 b=c.create_string_buffer(n);assert k.ReadProcessMemory(h,a,b,n,None),hex(a);return b.raw
def q(a):return struct.unpack('<Q',read(a,8))[0]
game=0x140000000
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
if '--generate' in sys.argv:
 regions=[('ClassLayer',0x3cc849,0x3cc863),('TabLayer',0x3d1211,0x3d1241),
          ('ParameterLayer',0x3cca09,0x3cca2f),('ParameterLayer24',0x3d1135,0x3d1159),
          ('StatTable',0x2d97ab,0x2d97b7),('StatGroup',0x2d65a8,0x2d65db),
          ('UniqueTable',0x19439b,0x1943a7),('GroupTable',0x2d96ff,0x2d970b),
          ('GroupEntry',0x3d1a31,0x3d1a7f),('RangeHelperABI',0x2d634d,0x2d6379),('RangePairCall',0x2d64c3,0x2d64ef)]
 header=['// Qualified live D2R 3.3.93787 source-layout witnesses.','#pragma once','#include <cstddef>','namespace SourceProfile {']
 for name,start,end in regions:
  b=read(game+start,end-start)
  header += [f'inline constexpr unsigned char {name}[]={{'+','.join(f'0x{x:02x}' for x in b)+'};']
 header += ['struct Witness { std::size_t rva; const unsigned char* bytes; std::size_t size; };','inline constexpr Witness Witnesses[]={']
 header += [f'{{0x{start:x},{name},sizeof({name})}},' for name,start,end in regions]
 header += ['};','}']
 (Path(__file__).parents[1]/'src/source_profile.h').write_text('\n'.join(header)+'\n')
 print('Generated reviewed source-layout witnesses:',len(regions))
elif len(sys.argv)>2:
 a=int(sys.argv[2],0);n=int(sys.argv[3],0)
 for i in md.disasm(read(a,n),a):print(hex(i.address),i.mnemonic,i.op_str)
else:
 for fn in [1,8,10,11,15,16,19,21,22,24]:
  a=q(game+0x2386ab0+fn*8); print('FUNCTION',fn,hex(a))
  for i in md.disasm(read(a,0x240),a):
   print(hex(i.address),i.mnemonic,i.op_str)
   if i.mnemonic=='ret':break
