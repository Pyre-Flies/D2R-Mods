"""Isolated native setter probe; never opens or writes a game process.

Reads the known private code capture, verifies its identity, then runs only the
normal SetX/SetY/GetRect paths against a synthetic widget in this process. The
fill-parent assertion paths are deliberately unreachable. No game asset is
copied into the repository. This checks the ABI and exact mutation footprint,
not live drawing or mouse behavior.
"""
import argparse
import ctypes as c
import hashlib
import struct
from pathlib import Path

import pefile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("capture", type=Path)
args = parser.parse_args()
data = args.capture.read_bytes()
assert hashlib.sha256(data).hexdigest() == "02c32a05f182c3f1f1b29d58b2b8da735eadc080b5e13acf8ea721a0648ce60d"
pe = pefile.PE(data=data)
functions = [(0x857570, 0x45), (0x8575C0, 0x45), (0x8562A0, 0x49)]
k = c.WinDLL("kernel32", use_last_error=True)
k.VirtualAlloc.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong, c.c_ulong]
k.VirtualAlloc.restype = c.c_void_p
k.VirtualProtect.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong, c.POINTER(c.c_ulong)]
k.VirtualProtect.restype = c.c_int
k.VirtualFree.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong]
k.VirtualFree.restype = c.c_int
k.GetCurrentProcess.restype = c.c_void_p
k.FlushInstructionCache.argtypes = [c.c_void_p, c.c_void_p, c.c_size_t]
page = k.VirtualAlloc(None, 4096, 0x3000, 0x04)
if not page:
    raise c.WinError(c.get_last_error())
try:
    for i, (rva, length) in enumerate(functions):
        body = pe.get_data(rva, length)
        assert len(body) == length and body[-1] == 0xC3
        c.memmove(page + i * 256, body, length)
    old = c.c_ulong()
    if not k.VirtualProtect(page, 4096, 0x20, c.byref(old)):
        raise c.WinError(c.get_last_error())
    assert k.FlushInstructionCache(k.GetCurrentProcess(), page, 4096)
    set_x = c.WINFUNCTYPE(None, c.c_void_p, c.c_int32)(page)
    set_y = c.WINFUNCTYPE(None, c.c_void_p, c.c_int32)(page + 256)
    get_rect = c.WINFUNCTYPE(c.c_void_p, c.c_void_p, c.c_void_p)(page + 512)
    for x, y in [(860, 1620), (300, 700), (0, 0), (-20, -30), (4096, 4096)]:
        widget = c.create_string_buffer(b"\xA5" * 256, 256)
        widget[0x52] = b"\0"
        struct.pack_into("<4i", widget, 0x70, 800, 1644, 120, 120)
        expected = bytearray(widget.raw)
        struct.pack_into("<2i", expected, 0x70, x, y)
        set_x(widget, x)
        set_y(widget, y)
        assert widget.raw == expected, "setter changed fields other than x/y"
        rect = (c.c_int32 * 4)()
        assert get_rect(widget, rect) == c.addressof(rect)
        assert tuple(rect) == (x, y, 120, 120)
    print("PASS: native setter ABI, five positions, x/y-only mutation, size preservation, native rectangle read-back. No game process accessed.")
finally:
    assert k.VirtualFree(page, 0, 0x8000)
