import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
spec = importlib.util.spec_from_file_location('cheat_tool', ROOT / 'tools/cheat_tool.py')
cheat_tool = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = cheat_tool
spec.loader.exec_module(cheat_tool)


class CheatToolTests(unittest.TestCase):
    def test_known_genesis_game_genie_vector(self):
        patch = cheat_tool.decode_game_genie('SCRA-BJX0')
        self.assertEqual((patch.address, patch.value), (0x009C76, 0x5478))

    def test_doom_catalog(self):
        cheats = cheat_tool.parse(ROOT / 'config/doom_troopers_usa.cht')
        self.assertEqual(len(cheats), 30)
        self.assertTrue(all(not cheat.enabled for cheat in cheats))
        self.assertEqual((cheats[0].patches[0].address, cheats[0].patches[0].value),
                         (0xFF06BE, 0x000E))
        self.assertEqual((cheats[12].patches[0].address, cheats[12].patches[0].value),
                         (0xFF06EB, 0xFFFF))
        self.assertEqual(len(cheats[11].patches), 2)

    def test_rejects_bad_code(self):
        with self.assertRaises(ValueError):
            cheat_tool.decode_part('NOT-A-CODE')

    def test_conflicting_variants_are_rejected(self):
        cheats = cheat_tool.parse(ROOT / 'config/doom_troopers_usa.cht')
        with self.assertRaises(ValueError):
            cheat_tool.select(cheats, '17,18')

    def test_selection_accepts_rom_and_ram(self):
        cheats = cheat_tool.parse(ROOT / 'config/doom_troopers_usa.cht')
        selected = cheat_tool.select(cheats, '25,0')
        self.assertEqual([cheat.index for cheat in selected], [25, 0])


if __name__ == '__main__':
    unittest.main()
