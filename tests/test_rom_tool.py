"""Synthetic fixtures only. Never use commercial game bytes in tests."""
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest
from tools.rom_tool import validate, inspect


class RomToolTests(unittest.TestCase):
    def test_identity_gate(self):
        data = bytes(512)
        identity = {'size': 512, 'sha256': hashlib.sha256(data).hexdigest()}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'arbitrary.extension'
            path.write_bytes(data)
            self.assertEqual(validate(path, identity), data)
            path.write_bytes(bytes(511))
            with self.assertRaisesRegex(ValueError, 'size'):
                validate(path, identity)
            path.write_bytes(bytes(513))
            with self.assertRaisesRegex(ValueError, 'size'):
                validate(path, identity)
            path.write_bytes(b'x' + data[1:])
            with self.assertRaisesRegex(ValueError, 'SHA-256'):
                validate(path, identity)
            with self.assertRaises(OSError):
                validate(Path(directory) / 'missing', identity)

    def test_big_endian_metadata(self):
        data = bytearray(512)
        struct.pack_into('>II', data, 0, 0xFF1234, 0x200)
        result = inspect(data)
        self.assertEqual(result['initial_sp'], '0x00FF1234')
        self.assertEqual(result['reset_pc'], '0x00000200')
        self.assertFalse(result['sram_marker_present'])
