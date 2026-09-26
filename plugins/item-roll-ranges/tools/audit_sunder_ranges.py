"""Read-only loaded unique/property-group bounds, qualified 3.3.93787. PID."""
import ctypes as c,struct,sys,json
from pathlib import Path
k=c.WinDLL('kernel32',use_last_error=True);k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x1010,0,int(sys.argv[1]))
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(h,a,b,n,None):raise c.WinError(c.get_last_error())
 return b.raw
def q(a):return struct.unpack('<Q',read(a,8))[0]
def spec(values,depth=0):
 ident,param,low,high=values
 if ident==-1:return
 if depth>4:raise ValueError('Depth limit')
 print(' '*depth,'SPEC',values)
 if ident>>16==1:
  row=read(groups+(ident&65535)*200,200)
  print(' '*depth,'GROUP HEADER',row[:8].hex())
  for n in range(8):
   child=struct.unpack_from('<6i',row,8+n*24)
   if child[5]>0:
    print(' '*depth,'ENTRY',child)
    spec((child[0],child[1],child[3],child[4]),depth+1)
 else:
  row=read(props+ident*48,48)
  for n in range(7):
   fn=row[24+n]
   if not fn:continue
   stat=struct.unpack_from('<H',row,32+n*2)[0]
   cost=read(stats+stat*324,68) if stat<4096 else bytes(68)
   print(' '*depth,'HANDLER',fn,'STAT',stat,'VAL',struct.unpack_from('<H',row,10+n*2)[0],
         'shift',cost[20],'descfn',cost[50],'descval',cost[51])
try:
 game=0x140000000;table=q(game+0x2a9a580+3*16)
 if '--generate' in sys.argv:
  regions=[('Roll1',0x3cf9d7,0x3cf9e6),('Roll8',0x3d0f07,0x3d0f21),
           ('Shift',0x3d5a0c,0x3d5a2a),('Scalar19',0x2d732e,0x2d7347),
           ('GroupMode',0x3d4ea3,0x3d4eea),('GroupCount',0x3d1949,0x3d1975)]
  header=['// Reviewed scalar unique-bound witnesses, D2R3.3.93787.','#pragma once','#include <cstddef>','namespace UniqueRangeProfile {']
  for name,start,end in regions:
   header += [f'inline constexpr unsigned char {name}[]={{'+','.join(f'0x{x:02x}' for x in read(game+start,end-start))+'};']
  header += ['struct Witness { std::size_t rva; const unsigned char* bytes; std::size_t size; };','inline constexpr Witness Witnesses[]={']
  header += [f'{{0x{start:x},{name},sizeof({name})}},' for name,start,end in regions]
  header += ['};','}']
  (Path(__file__).parents[1]/'src/unique_range_profile.h').write_text('\n'.join(header)+'\n')
 unique=q(table+0x13c8);props=q(table+0x240);groups=q(table+0x258);stats=q(table+0x1258)
 row=read(unique+437*348,348)
 print('UNIQUE',row[:32].split(b'\0')[0], 'flags',struct.unpack_from('<I',row,44)[0])
 for n in range(12):spec(struct.unpack_from('<4i',row,152+n*16))
finally:k.CloseHandle(h)
