"""Original 68000 fixture; contains no bytes copied from a game."""
from pathlib import Path
import struct
import sys
image = bytearray(1024)
struct.pack_into('>II', image, 0, 0x3F0, 0x200)
struct.pack_into('>I', image, 33 * 4, 0x300)
struct.pack_into('>HHH', image, 0x200, 0x7005, 0x4E41, 0x4E71) # MOVEQ; TRAP #1; NOP
struct.pack_into('>HH', image, 0x300, 0x5280, 0x4E73) # ADDQ.L #1,D0; RTE
Path(sys.argv[1]).write_bytes(image)
