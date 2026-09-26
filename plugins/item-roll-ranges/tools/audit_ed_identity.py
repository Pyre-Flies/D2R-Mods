"""Read-only ED identity metadata audit; qualified 3.3.93787 only. PID."""
import ctypes as c,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import capstone
k=c.WinDLL('kernel32',use_last_error=True);k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x1010,0,int(sys.argv[1]))
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(h,a,b,n,None):raise c.WinError(c.get_last_error())
 return b.raw
def q(a):return struct.unpack('<Q',read(a,8))[0]
try:
 game=0x140000000;table=q(game+0x2a9a580+3*16)
 stats=q(table+0x1258);props=q(table+0x240)
 for stat in (17,18):
  row=read(stats+stat*0x144,0x44)
  print('stat',stat,'row',hex(stats+stat*0x144),'display',row[0x32:0x44].hex())
 p=read(props+29*48,48)
 print('property29 slots',[(p[0x18+i],struct.unpack_from('<H',p,0x20+i*2)[0]) for i in range(7)])
 core=0xc0de5000000
 if '--generate' in sys.argv:
  regions=[('Caller',core,0x3e7fb0,0x3e7fe7),('Entry',game,0x2db800,0x2db843),
           ('Dispatch',game,0x2db848,0x2db875),('ED',game,0x2dbcea,0x2dbd06),
           ('Append',game,0x2dbe77,0x2dbe96),('Return',game,0x2dbeb3,0x2dbecb),
           ('JumpTargets',game,0x2dbee4,0x2dbf22)]
  header=['// Reviewed live special-damage formatter witnesses, 3.3.93787.','#pragma once','#include <cstddef>','namespace SpecialProfile {']
  for name,base,begin,end in regions:
   data=read(base+begin,end-begin)
   header += [f'inline constexpr unsigned char {name}[]={{'+','.join(f'0x{x:02x}' for x in data)+'};']
  header += ['struct Witness { bool core; std::size_t rva; const unsigned char* bytes; std::size_t size; };','inline constexpr Witness Witnesses[]={']
  header += [f'{{{str(base==core).lower()},0x{begin:x},{name},sizeof({name})}},' for name,base,begin,end in regions]
  header += ['};','}']
  (Path(__file__).parents[1]/'src/special_profile.h').write_text('\n'.join(header)+'\n')
  print('Generated reviewed special formatter witnesses')
 locale=read(q(core+0x704098),1)[0];strings=q(core+0x7dc080+locale*16)
 for sid in (0xdb4,0xdb5):
  text=q(strings+0x50+sid*8)
  print('Localized',sid,hex(text),repr(read(text,128).split(b'\0')[0]))
 target=q(core+0x7043a0);print('Special property formatter',hex(target))
 for stat in (17,18):
  index=read(game+0x2dbf20+stat-17,1)[0]
  branch=struct.unpack('<I',read(game+0x2dbee4+index*4,4))[0]
  print('Special stat dispatch',stat,hex(branch))
 print('ED localization key',repr(read(game+0x1cf5ae0,20)))
 md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
 for ins in md.disasm(read(target,0x400),target):
  print(hex(ins.address),ins.mnemonic,ins.op_str)
  if ins.mnemonic=='ret':break
finally:k.CloseHandle(h)
