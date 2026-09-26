"""Read-only qualified-profile audit. PID [Core base] [game base]; no process writes."""
import ctypes as c,struct,re,sys
from pathlib import Path
k=c.WinDLL('kernel32');k.OpenProcess.restype=c.c_void_p;k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.c_void_p]
h=k.OpenProcess(0x410,0,int(sys.argv[1]));base=int(sys.argv[2],0) if len(sys.argv)>2 else 0xc0de5000000;game=int(sys.argv[3],0) if len(sys.argv)>3 else 0x140000000
def read(a,n):
 b=c.create_string_buffer(n);assert k.ReadProcessMemory(h,a,b,n,None),(hex(a),n);return b.raw
assert read(base,2)==b'MZ' and read(game,2)==b'MZ', 'Module bases do not point to PE images'
expected={'key':0x1209fa0,'pad':0x13ca70,'panel':0xce500,'properties':0x2dc4b0,'single':0x2d6520,'tables':0x300a90,'eligible':0x3d4220}
for name,rva in [('key',0x6fe3b0),('pad',0x6fe470),('panel',0x702ae0),('properties',0x704490),('single',0x7043e8),('tables',0x701e70),('eligible',0x6ff370)]:
 v=struct.unpack('<Q',read(base+rva,8))[0];assert v==game+expected[name],(name,hex(v));print(name,hex(rva),hex(v),'game offset',hex(v-game),'core offset',hex(v-base))
src=(Path(__file__).parents[1]/'src/affix_profile.h').read_text()
for name,rva,body in re.findall(r'size_t (\w+)Rva=0x([0-9a-f]+);\s*inline constexpr unsigned char \w+\[\]=\{(.*?)\};',src,re.S):
 b=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]+)',body));actual=read(game+int(rva,16),len(b));assert actual==b,name;print('witness',name,'PASS')

provider=(Path(__file__).parents[1]/'src/provider_profile.h').read_text()
for name,rva,body in re.findall(r'size_t (\w+)Rva = 0x([0-9a-f]+);\s*inline constexpr unsigned char \w+\[\] = \{(.*?)\};',provider,re.S):
 b=bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]+)',body));actual=read(base+int(rva,16),len(b));assert actual==b,name;print('core witness',name,'PASS')

k.CloseHandle.argtypes=[c.c_void_p]
k.CloseHandle(h)
