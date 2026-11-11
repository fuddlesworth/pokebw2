"""rename_constant.py's path rewrites: a moved file's or directory's path from the repository's root follows the move,
and nothing that only starts with it.

    .venv/bin/python -m unittest tools.data.test_rename_constant
"""
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

# The packers it imports import each other by module name, as scripts run from tools/data do
sys.path.insert(0, str(Path(__file__).resolve().parent))
from tools.data import rename_constant  # noqa: E402


class PathRenameTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        patcher = patch.object(rename_constant, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def rewrite(self, old, new, text):
        pattern, replacement, needle = rename_constant.path_rename(self.root / old, self.root / new)
        self.assertIn(needle, text)
        return pattern.sub(replacement, text)

    def test_a_file_path_follows_the_move(self):
        text = ('NAMES = ROOT / "res/text/system/btl_main_9.txt"\n'
                '["res/text/system/btl_main_9.txt", "res/sound/wave_archive.schema.json"]\n'
                "a message of `res/text/system/btl_main_9.txt` (the\n")
        self.assertEqual(self.rewrite("res/text/system/btl_main_9.txt", "res/text/system/musical_npc_names.txt", text),
                         text.replace("btl_main_9.txt", "musical_npc_names.txt"))

    def test_a_longer_name_that_starts_with_it_stays(self):
        text = '"res/text/system/title_2.txt" "res/text/system/title.txt.bak" "x/res/text/system/title.txt"\n'
        self.assertEqual(self.rewrite("res/text/system/title.txt", "res/text/system/battle.txt", text), text)

    def test_a_directory_path_follows_inside_longer_paths(self):
        text = '"res/moves/tackle" "res/moves/tackle/data.json" "res/moves/tackle_2/data.json"\n'
        self.assertEqual(self.rewrite("res/moves/tackle", "res/moves/body_slam", text),
                         '"res/moves/body_slam" "res/moves/body_slam/data.json" "res/moves/tackle_2/data.json"\n')


    def test_a_message_prefix_follows_alone_and_in_ids_but_not_inside_a_longer_one(self):
        old, new = Path("res/text/system/load_battle_type_text_to_strbuf.txt"), Path("res/text/system/b.txt")
        pattern, replacement, needle = rename_constant.prefix_rename(old, new)
        text = ('"pattern": "^LoadBattleTypeTextToStrbuf_Text_"\nLoadBattleTypeTextToStrbuf_Text_SingleBattle\n'
                "ScriptLoadBattleTypeTextToStrbuf_Text_X\n")
        self.assertEqual(pattern.sub(replacement, text), '"pattern": "^B_Text_"\nB_Text_SingleBattle\n'
                                                         "ScriptLoadBattleTypeTextToStrbuf_Text_X\n")


if __name__ == "__main__":
    unittest.main()
