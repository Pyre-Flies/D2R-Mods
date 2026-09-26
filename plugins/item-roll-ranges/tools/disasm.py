"""Read-only PE disassembly; addresses are supplied and printed as RVAs."""
import argparse
import sys
from pathlib import Path

import capstone, pefile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('rva', type=lambda value: int(value, 0))
parser.add_argument('size', type=lambda value: int(value, 0),
                    help='bytes to disassemble; use 0 to find CALL xrefs')
parser.add_argument('image', type=Path,
                    help='private PE image to inspect; never commit this file')
args = parser.parse_args()
image = args.image
pe = pefile.PE(str(image))
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
start, size = args.rva, args.size
if size == 0:
    import struct
    data = pe.get_data(0x1000, 0x1ca0000)
    for i in range(len(data)-5):
        if data[i] == 0xe8 and 0x1000+i+5+struct.unpack_from('<i', data, i+1)[0] == start:
            print(hex(0x1000+i))
    sys.exit()
for ins in md.disasm(pe.get_data(start, size), start):
    print(f'{ins.address:08X} {ins.bytes.hex():30} {ins.mnemonic} {ins.op_str}')
