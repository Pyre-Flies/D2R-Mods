"""Read-only bounded disassembly of a user-requested live D2R code range."""
import ctypes as c,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent/'python-libs'))
import capstone
pid,address,size=(int(x,0) for x in sys.argv[1:4])
assert 0<size<=65536
k=c.WinDLL('kernel32',use_last_error=True)
k.OpenProcess.argtypes=[c.c_ulong,c.c_int,c.c_ulong];k.OpenProcess.restype=c.c_void_p
k.ReadProcessMemory.argtypes=[c.c_void_p,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)];k.ReadProcessMemory.restype=c.c_int
k.CloseHandle.argtypes=[c.c_void_p]
h=k.OpenProcess(0x10|0x1000,False,pid)
if not h:raise c.WinError(c.get_last_error())
try:
 b=c.create_string_buffer(size);n=c.c_size_t()
 if not k.ReadProcessMemory(h,address,b,size,c.byref(n)):raise c.WinError(c.get_last_error())
 md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
 for i in md.disasm(b.raw[:n.value],address):print(f'{i.address:016X} {i.bytes.hex():30} {i.mnemonic} {i.op_str}')
finally:k.CloseHandle(h)
