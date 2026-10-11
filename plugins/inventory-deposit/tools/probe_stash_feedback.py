"""Exercise captured tab setters against synthetic widgets, never a game process.

The alpha setter's one RIP-relative scalar is relocated to a synthetic 1.0f.
This checks the x64 ABI and mutation footprint, not the live scalar or renderer.
"""
import argparse
import ctypes as c
import hashlib
import re
import struct
from pathlib import Path

import pefile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("capture", type=Path)
args = parser.parse_args()
data = args.capture.read_bytes()
assert hashlib.sha256(data).hexdigest() == "02c32a05f182c3f1f1b29d58b2b8da735eadc080b5e13acf8ea721a0648ce60d"
pe = pefile.PE(data=data)
profile = (Path(__file__).resolve().parents[1] / "src/stash_feedback_profile.h").read_text()
for name in ("Category", "Enable", "Alpha", "Notification", "BankType"):
    rva = int(re.search(rf"{name}Rva=(0x[0-9a-f]+)", profile)[1], 16)
    body = re.search(rf"char {name}\[\]=\{{(.*?)\}};", profile, re.S)[1]
    expected = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-f]{2})", body))
    assert pe.get_data(rva, len(expected)) == expected, f"{name} guard differs from pinned capture"
show_body = pe.get_data(0x879470, 0x65)
alpha_body = bytearray(pe.get_data(0x879D20, 0x48))
assert show_body[-1] == alpha_body[-1] == 0xC3
assert alpha_body[0x26:0x29] == bytes.fromhex("0f2f15")
assert 0x879D20 + 0x2D + struct.unpack_from("<i", alpha_body, 0x29)[0] == 0x1CBA904
# Alpha code starts at allocation+256; its scalar lives at allocation+512.
struct.pack_into("<i", alpha_body, 0x29, 512 - (256 + 0x2D))
k = c.WinDLL("kernel32", use_last_error=True)
k.VirtualAlloc.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong, c.c_ulong]
k.VirtualAlloc.restype = c.c_void_p
k.VirtualProtect.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong, c.POINTER(c.c_ulong)]
k.VirtualProtect.restype = c.c_int
k.VirtualFree.argtypes = [c.c_void_p, c.c_size_t, c.c_ulong]
k.VirtualFree.restype = c.c_int
k.GetCurrentProcess.restype = c.c_void_p
k.FlushInstructionCache.argtypes = [c.c_void_p, c.c_void_p, c.c_size_t]
k.FlushInstructionCache.restype = c.c_int
page = k.VirtualAlloc(None, 4096, 0x3000, 0x04)
if not page:
    raise c.WinError(c.get_last_error())
try:
    c.memmove(page, show_body, len(show_body))
    c.memmove(page + 256, bytes(alpha_body), len(alpha_body))
    c.memmove(page + 512, struct.pack("<f", 1.0), 4)
    old = c.c_ulong()
    if not k.VirtualProtect(page, 4096, 0x20, c.byref(old)):
        raise c.WinError(c.get_last_error())
    assert k.FlushInstructionCache(k.GetCurrentProcess(), page, 4096)
    show = c.WINFUNCTYPE(None, c.c_void_p, c.c_int32, c.c_bool)(page)
    alpha = c.WINFUNCTYPE(None, c.c_void_p, c.c_int32, c.c_float)(page + 256)
    calls = []
    callback_type = c.WINFUNCTYPE(None, c.c_void_p, c.c_bool)
    visible = callback_type(lambda widget, flag: calls.append(("visible", widget, flag)))
    enabled = callback_type(lambda widget, flag: calls.append(("enabled", widget, flag)))
    vtable = c.create_string_buffer(0x58)
    struct.pack_into("<Q", vtable, 0x48, c.cast(visible, c.c_void_p).value)
    struct.pack_into("<Q", vtable, 0x50, c.cast(enabled, c.c_void_p).value)
    for index in (2, 3, 4):
        tabs = c.create_string_buffer(0x17A0)
        child = c.create_string_buffer(0x100)
        struct.pack_into("<Q", child, 0, c.addressof(vtable))
        struct.pack_into("<Q", tabs, 0x1748 + index * 8, c.addressof(child))
        for allowed, active in ((True, True), (True, False), (False, True)):
            tabs[0x1788 + index] = bytes([allowed])
            before_tabs, before_child = bytearray(tabs.raw), bytearray(child.raw)
            calls.clear()
            show(tabs, index, active)
            before_tabs[0x1790 + index] = active
            struct.pack_into("<f", before_child, 0x94, 1.0)
            assert tabs.raw == before_tabs and child.raw == before_child
            assert calls == [("visible", c.addressof(child), allowed and active),
                             ("enabled", c.addressof(child), allowed and active)]
        for requested, expected in ((-1.0, 0.0), (0.25, 0.25), (1.0, 1.0), (2.0, 1.0)):
            before_tabs, before_child = tabs.raw, bytearray(child.raw)
            struct.pack_into("<f", before_child, 0x94, expected)
            alpha(tabs, index, requested)
            assert tabs.raw == before_tabs and child.raw == before_child
        struct.pack_into("<Q", tabs, 0x1748 + index * 8, 0)
        calls.clear()
        show(tabs, index, False)
        before_tabs = tabs.raw
        alpha(tabs, index, 0.5)
        assert not calls and tabs.raw == before_tabs
    print("PASS: native tab setter ABI, tabs 2/3/4, visibility gates, alpha clamping, null children and bounded mutation footprints. Synthetic widgets only; no game process accessed.")
finally:
    assert k.VirtualFree(page, 0, 0x8000)
