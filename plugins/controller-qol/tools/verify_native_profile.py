"""Verify the candidate native profile against a live process without mutation.
Usage: python verify_native_profile.py PID [core_base] [game_base]
"""
from pathlib import Path
import ctypes as c,sys,re,struct,hashlib
pid=int(sys.argv[1]);core=int(sys.argv[2],0) if len(sys.argv)>2 else 0xc0de5000000
game=int(sys.argv[3],0) if len(sys.argv)>3 else 0x140000000
k=c.WinDLL('kernel32',use_last_error=True);k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x410,0,pid)
if not h:raise c.WinError(c.get_last_error())
def read(a,n):
 b=c.create_string_buffer(n)
 if not k.ReadProcessMemory(h,a,b,n,None):raise c.WinError(c.get_last_error())
 return b.raw
try:
 assert read(core,2)==b'MZ' and read(game,2)==b'MZ'
 src=(Path(__file__).parents[1]/'src/native_input_profile.h').read_text()
 for name,rva,body in re.findall(r'size_t (\w+)Rva=0x([0-9a-f]+);\s*inline constexpr unsigned char \w+Bytes\[\]=\{([^}]+)\}',src):
  expected=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]+)',body))
  assert read(game+int(rva,16),len(expected))==expected,name
  print(name,hex(int(rva,16)),len(expected),'PASS')
 for slot,target in [(0x701bb0,0x3440170),(0x6fe440,0x13ce90),(0x7004a8,0x8b2d0)]:
  actual=struct.unpack('<Q',read(core+slot,8))[0]
  assert actual==game+target,(hex(slot),hex(actual))
  print('Core slot',hex(slot),'-> game',hex(target),'PASS')
finally:k.CloseHandle(h)
