"""Read-only item contribution audit for qualified game 3.3.93787. PID required.
Uses documented client unit buckets and native property dispatch; no writes.
"""
import ctypes as c
import json, struct, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import capstone
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[c.c_ulong,c.c_int,c.c_ulong];k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x1010,False,int(sys.argv[1]))
if not h: raise c.WinError(c.get_last_error())
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(h,a,b,n,None):raise c.WinError(c.get_last_error())
 return b.raw
def q(a):return struct.unpack('<Q',read(a,8))[0]
game=0x140000000
try:
 core=0xc0de5000000
 image=read(core+0x1000,0x900000)
 for pos in range(len(image)-6):
  if image[pos:pos+2]!=b'\xff\x15':continue
  addr=core+0x1000+pos
  target=addr+6+struct.unpack_from('<i',image,pos+2)[0]
  if target==core+0x704400:print('Range helper call/return',hex(addr-core),hex(addr+6-core))
 md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
 fn=q(game+0x3e2a240) if '--adder' in sys.argv else q(game+0x2386ab0+7*8)
 print('Core stat adder' if '--adder' in sys.argv else 'Physical enhanced damage function7',hex(fn))
 for i in md.disasm(read(fn,0x600),fn):
  print(hex(i.address),i.mnemonic,i.op_str)
  if i.mnemonic=='ret':break
 seen=set()
 for bucket in range(128):
  unit=q(game+0x2a23910+4*1024+bucket*8)
  while unit and unit not in seen:
   current=unit;seen.add(unit);b=read(unit,0x1c0);unit=struct.unpack_from('<Q',b,0x158)[0]
   if struct.unpack_from('<I',b,0)[0]!=4:continue
   data=struct.unpack_from('<Q',b,0x10)[0];d=read(data-28,0xa0)
   quality=struct.unpack_from('<I',d,28)[0]
   if quality not in (4,6,7,8):continue
   bank=b[0x1bd];table=q(game+0x2a9a580+bank*16);rows=q(table+0x15e8)
   ids=struct.unpack_from('<6I',d,4);affixes=[]
   for aid in ids:
    if not aid or aid>q(table+0x15f0):continue
    row=read(rows+(aid-1)*140,140)
    affixes.append({'id':aid,'name':row[:32].split(b'\0')[0].decode(errors='replace'),'mods':[struct.unpack_from('<4i',row,0x24+n*16) for n in range(3)]})
   statlist=struct.unpack_from('<Q',b,0x88)[0]
   sb=read(statlist,0xc0)
   vectors={}
   for off in (0x30,0xa8):
    ptr,count=struct.unpack_from('<QQ',sb,off)
    if ptr and 0<count<256:
     try:vectors[hex(off)]=[struct.unpack_from('<IIiI',read(ptr+n*16,16)) for n in range(count)]
     except OSError:pass
   print('ITEM',json.dumps({'unit':hex(current),'data':hex(data),'code':struct.unpack_from('<I',b,4)[0],'quality':quality,'affixes':affixes,'statlist':hex(statlist),'vectors':vectors,'statlist_qwords':{hex(n):hex(struct.unpack_from('<Q',sb,n)[0]) for n in range(0,0xc0,8)}}))
   child=struct.unpack_from('<Q',sb,0x90)[0];visited=set()
   while child and child not in visited and len(visited)<16:
    visited.add(child);cb=read(child,0x90);ptr,count=struct.unpack_from('<QQ',cb,0x30)
    values=[struct.unpack_from('<IIiI',read(ptr+n*16,16)) for n in range(count)] if ptr and count<256 else []
    print('CHILD',hex(child),'stats',values,'qwords',{hex(n):hex(struct.unpack_from('<Q',cb,n)[0]) for n in range(0,0x90,8)})
    child=struct.unpack_from('<Q',cb,0x48)[0]
finally:k.CloseHandle(h)
