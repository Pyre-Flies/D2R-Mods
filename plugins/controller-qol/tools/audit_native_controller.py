"""Read-only native controller ABI investigation; never writes to the game.

The module bases are deliberately explicit because process addresses are
session-specific. Obtain them from a debugger or module enumeration and record
only the derived RVAs in public evidence.
"""
import argparse
import ctypes as c
import struct

import capstone

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--pid', type=int, required=True)
parser.add_argument('--core-base', type=lambda value: int(value, 0), required=True)
parser.add_argument('--game-base', type=lambda value: int(value, 0), required=True)
args = parser.parse_args()

k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x410,0,args.pid); core=args.core_base; game=args.game_base
assert h

def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(h,a,b,n,None):raise OSError(hex(a),c.get_last_error())
 return b.raw

def ptr(a):return struct.unpack('<Q',read(a,8))[0]
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
try:
 for name,rva in [('manager',0x701bb0),('input',0x6fe440),('index',0x7004a8),('button',0x6fe470)]:
  p=ptr(core+rva);print(name,hex(rva),hex(p),'game RVA',hex(p-game))
  if name=='manager':
   print('manager object',hex(ptr(p)));continue
  for i in md.disasm(read(p,160),p):
   print(hex(i.address-game),i.bytes.hex(),i.mnemonic,i.op_str)
   if i.mnemonic=='ret':break
finally:k.CloseHandle(h)
