"""Read-only capture of a running D2R process's executable .text section.
Creates a PE-shaped disassembly artifact; it is NOT a runnable executable.
No hooks, writes, thread suspension, or process injection. Keep output private.
Usage: python capture_code.py --pid <D2R PID> --output runtime-code.exe
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import struct

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--pid', type=int, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
k = c.WinDLL('kernel32', use_last_error=True)
p = c.WinDLL('psapi', use_last_error=True)
k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]; k.OpenProcess.restype = w.HANDLE
k.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
k.ReadProcessMemory.restype = w.BOOL
k.CloseHandle.argtypes = [w.HANDLE]; k.CloseHandle.restype = w.BOOL
p.EnumProcessModules.argtypes = [w.HANDLE, c.POINTER(w.HMODULE), w.DWORD, c.POINTER(w.DWORD)]
p.EnumProcessModules.restype = w.BOOL
p.GetModuleBaseNameW.argtypes = [w.HANDLE, w.HMODULE, w.LPWSTR, w.DWORD]
p.GetModuleBaseNameW.restype = w.DWORD
handle = k.OpenProcess(0x0400 | 0x0010, False, args.pid)
if not handle:
    raise c.WinError(c.get_last_error())
try:
    modules = (w.HMODULE * 1024)(); needed = w.DWORD()
    if not p.EnumProcessModules(handle, modules, c.sizeof(modules), c.byref(needed)):
        raise c.WinError(c.get_last_error())
    module = modules[0]; name = c.create_unicode_buffer(1024)
    if not p.GetModuleBaseNameW(handle, module, name, len(name)) or name.value.lower() != 'd2r.exe':
        raise ValueError('PID does not identify D2R.exe')
    base = int(module)
    def read(address, size):
        buffer = c.create_string_buffer(size); received = c.c_size_t()
        if not k.ReadProcessMemory(handle, address, buffer, size, c.byref(received)) or received.value != size:
            raise c.WinError(c.get_last_error())
        return buffer.raw
    header = bytearray(read(base, 4096))
    pe = struct.unpack_from('<I', header, 0x3c)[0]
    if header[pe:pe+4] != b'PE\0\0': raise ValueError('Invalid PE header')
    count = struct.unpack_from('<H',header,pe+6)[0]
    optional_size = struct.unpack_from('<H',header,pe+20)[0]
    if struct.unpack_from('<H',header,pe+24)[0] != 0x20b: raise ValueError('Expected x64 PE')
    header_size = struct.unpack_from('<I',header,pe+24+60)[0]
    if header_size > len(header): header = bytearray(read(base,header_size))
    struct.pack_into('<Q',header,pe+24+24,base)
    captured = None
    for i in range(count):
        at = pe+24+optional_size+40*i
        name = header[at:at+8].rstrip(b'\0')
        virtual_size, rva = struct.unpack_from('<II',header,at+8)
        if name == b'.text':
            captured = (rva, read(base+rva,virtual_size))
            struct.pack_into('<II',header,at+16,virtual_size,rva)
        else:
            struct.pack_into('<II',header,at+16,0,0)
    if captured is None: raise ValueError('No .text section')
    rva, code = captured
    # 'xb' refuses to overwrite earlier evidence.
    with args.output.open('xb') as file:
        file.write(header[:header_size]); file.seek(rva); file.write(code)
    print(f'Captured .text at module base {base:#x}; {len(code)} bytes. Other sections omitted.')
finally:
    k.CloseHandle(handle)
