"""The naming report's patterns; no fixture reads the repository's sources or configs."""
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import naming_report as nr


class Patterns(unittest.TestCase):
    def test_unk_names(self):
        for name in ("unk_2E4", "unk1C", "unkE6", "BtlvEffect_GetUnk2E4", "WbtUnk18A", "POKEWOOD_BLOCK_FIELD_UNKE0",
                     "unk"):
            self.assertTrue(nr.UNK.search(name), name)
        for name in ("unknown", "Unknown", "trunk", "UNKNOWN_SPECIES", "junkyard"):
            self.assertFalse(nr.UNK.search(name), name)

    def test_placeholder_parameters(self):
        for name in ("a0", "a1", "param2", "arg10", "var3"):
            self.assertTrue(nr.PARAM.match(name), name)
        for name in ("a", "a10", "area", "params", "argc", "variant1"):
            self.assertFalse(nr.PARAM.match(name), name)

    def test_case_labels(self):
        self.assertTrue(nr.CASE.search("    case 3:"))
        self.assertTrue(nr.CASE.search("case 0x1f :"))
        self.assertTrue(nr.CASE.search("case -1:"))
        self.assertFalse(nr.CASE.search("case SEQ_INIT:"))

    def test_compares(self):
        for text in ("x == 2", "x != 0x20", "x == 10)"):
            self.assertTrue(nr.COMPARE.search(text), text)
        for text in ("x == 0", "x != 1", "x == FOO", "x == 2 * y", "x == 3 + y"):
            self.assertFalse(nr.COMPARE.search(text), text)

    def test_comments_and_strings_are_blanked(self):
        text = 'a = 1; // unk_2E4\n"case 3:"; /* unk1C\n*/ b = unk_10;'
        code = nr.code_only(text)
        self.assertEqual(code.count("\n"), text.count("\n"))
        self.assertNotIn("unk_2E4", code)
        self.assertNotIn("case 3", code)
        self.assertNotIn("unk1C", code)
        self.assertIn("unk_10", code)

    def test_modules(self):
        self.assertEqual(nr.module_of("src/ov036/fieldmap.c"), "ov036")
        self.assertEqual(nr.module_of("src/gfl/heap.c"), "main")
        self.assertEqual(nr.module_of("src/system/pokedex_savedata.c"), "main")
        self.assertEqual(nr.module_of("lib/nitro/src/fx/fx.c"), "lib/nitro")


class Veneers(unittest.TestCase):
    """The linker's interworking veneers get a func_ symbol but hold no C, so --veneers counts them apart."""

    def test_the_three_veneer_shapes(self):
        for bytes_, thumb in ((bytes.fromhex("00c09fe51cff2fe1"), False),   # ARM ldr ip,[pc]; bx ip
                              (bytes.fromhex("04f01fe5"), False),           # ARM ldr pc,[pc,#-4]
                              (bytes.fromhex("7847"), True)):               # Thumb bx pc
            with self.subTest(bytes_=bytes_.hex()):
                self.assertTrue(nr.is_veneer(bytes_ + bytes(16), 0x02190000, 0x02190000, thumb))

    def test_real_functions_are_not_veneers(self):
        # Thumb prologues, an immediate return, and a tiny ARM body: what the 27 address-named
        # functions that do have C begin with.
        for start in ("08b5002100220123", "30b585b00a4d021c", "70470000f0b583b0",
                      "38b56ff7e9f8051c", "0201e0e31eff2fe1"):
            with self.subTest(start=start):
                self.assertFalse(nr.is_veneer(bytes.fromhex(start) + bytes(8), 0, 0, False))

    def test_the_instruction_set_decides(self):
        # `04f01fe5` is ARM's ldr pc,[pc,#-4]; in a Thumb stream the same bytes are a BL, not a veneer
        image = bytes.fromhex("04f01fe5") + bytes(8)
        self.assertTrue(nr.is_veneer(image, 0, 0, False))
        self.assertFalse(nr.is_veneer(image, 0, 0, True))

    def test_an_address_outside_the_image_is_not_a_veneer(self):
        image = bytes.fromhex("00c09fe51cff2fe1")
        self.assertFalse(nr.is_veneer(image, 0x02190000, 0x02100000, False))
        self.assertFalse(nr.is_veneer(b"", 0x02190000, 0x02190000, False))

    def test_sizes_come_out_of_symbols(self):
        line = "func_ov167_0219f530 kind:function(thumb,size=0x2) addr:0x0219f530"
        self.assertEqual(nr.SIZE.search(line).group(1), "2")
        self.assertEqual(nr.ISA.search(line).group(1), "thumb")
        self.assertEqual(nr.ISA.search("kind:function(arm,size=0xc)").group(1), "arm")
        self.assertIsNone(nr.SIZE.search("kind:data(any) addr:0x0219f530"))

    def test_the_default_report_keeps_its_six_counts(self):
        self.assertEqual(nr.KINDS, ("funcs", "data", "unk", "params", "cases", "compares"))


class Measurements(unittest.TestCase):
    """module_image and the veneer counts, on a temporary checkout; nothing reads the live config or build."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.config = self.root / "config/b2_us/arm9"
        self.build = self.root / "build/b2_us/build"
        self.build.mkdir(parents=True)
        for target, value in (("ROOT", self.root), ("CONFIG", self.config)):
            patcher = patch.object(nr, target, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def add_module(self, name, delinks, symbols="", image=None):
        """Registers a module's delinks, symbols and, when given, its linked image."""
        owner = self.config / name
        owner.mkdir(parents=True)
        (owner / "delinks.txt").write_text(delinks)
        (owner / "symbols.txt").write_text(symbols)
        if image is not None:
            image_name = (f"arm9_{owner.name}.bin" if owner.parent.name == "overlays"
                          else f"{owner.name}.bin")
            (self.build / image_name).write_bytes(image)
        return owner

    def run_main(self, *args):
        out = io.StringIO()
        with contextlib.redirect_stdout(out), patch.object(sys, "argv", ["naming_report.py", *args]):
            nr.main()
        return out.getvalue()

    def test_the_load_address_is_the_module_section_not_a_file_section(self):
        # The ITCM shape: the module's .text starts at 0x01ff8000, while the first file's section, and so what
        # file_ranges() returns first, starts at 0x01ff8020.
        owner = self.add_module(
            "itcm",
            "    .text       start:0x01ff8000 end:0x01ff939c kind:code align:32\n"
            "    .bss        start:0x01ff93a0 end:0x01ff93a0 kind:bss align:32\n"
            "\n"
            "src/gfl/clact_itcm.c:\n"
            "    complete\n"
            "    .text       start:0x01ff8020 end:0x01ff8bf0\n",
            image=b"\x00" * 0x40)
        image, base = nr.module_image(owner / "symbols.txt")
        self.assertEqual(base, 0x01FF8000)
        self.assertEqual(image, b"\x00" * 0x40)

    def test_an_absent_image_is_none_but_an_empty_symbols_file_keeps_its_base(self):
        owner = self.add_module(
            "overlays/ov006",
            "    .text       start:0x0214f500 end:0x0214f904 kind:code align:32\n")
        self.assertIsNone(nr.module_image(owner / "symbols.txt"))  # no image: unmeasured, not guessed
        image = b"\xab" * 0x20
        (self.build / "arm9_ov006.bin").write_bytes(image)
        self.assertEqual(nr.module_image(owner / "symbols.txt"), (image, 0x0214F500))

    def test_an_absent_image_counts_its_functions_as_unmeasured(self):
        self.add_module(
            "overlays/ov006",
            "    .text       start:0x0214f500 end:0x0214f904 kind:code align:32\n",
            "func_ov006_0214f500 kind:function(thumb,size=0x2) addr:0x0214f500\n")
        counts, veneers = nr.address_names()
        self.assertEqual(counts["ov006"]["funcs"], 1)
        self.assertEqual(veneers["ov006"]["unmeasured"], 1)
        self.assertEqual(veneers["ov006"]["veneers"], 0)

    def test_the_default_table_and_json_leave_the_extra_counts_out(self):
        self.add_module(
            "overlays/ov006",
            "    .text       start:0x0214f500 end:0x0214f904 kind:code align:32\n",
            "func_ov006_0214f500 kind:function(thumb,size=0x2) addr:0x0214f500\n",
            image=bytes.fromhex("7847") + bytes(0x1e))
        # A second part with no image exercises the unmeasured key once --veneers asks for it.
        self.add_module(
            "overlays/ov007",
            "    .text       start:0x0214f900 end:0x0214f904 kind:code align:32\n",
            "func_ov007_0214f900 kind:function(thumb,size=0x2) addr:0x0214f900\n")
        self.assertEqual(nr.report()[0]["ov006"]["ov006"]["veneers"], 1)
        table = self.run_main()
        self.assertNotIn("veneers", table)
        self.assertNotIn("unmeasured", table)
        plain = self.run_main("--json")
        self.assertNotIn('"veneers"', plain)
        self.assertNotIn('"unmeasured"', plain)
        enabled = self.run_main("--json", "--veneers")
        self.assertIn('"veneers": 1', enabled)
        self.assertIn('"unmeasured": 1', enabled)

    def test_module_veneer_columns_are_opt_in(self):
        self.add_module(
            "overlays/ov006",
            "    .text       start:0x0214f500 end:0x0214f904 kind:code align:32\n",
            "func_ov006_0214f500 kind:function(thumb,size=0x2) addr:0x0214f500\n",
            image=bytes.fromhex("7847") + bytes(0x1e))
        self.assertNotIn("veneers", self.run_main("--module", "ov006"))
        listed = self.run_main("--module", "ov006", "--veneers")
        self.assertIn("veneers", listed)
        self.assertIn("unmeasured", listed)

    def test_the_size_boundary_is_twelve_bytes(self):
        # Both functions start with a Thumb bx pc; only the 0xc one is short enough to be examined.
        image = bytearray(0x20)
        image[0:2] = bytes.fromhex("7847")
        image[0x10:0x12] = bytes.fromhex("7847")
        self.add_module(
            "overlays/ov006",
            "    .text       start:0x0214f500 end:0x0214f904 kind:code align:32\n",
            "func_ov006_0214f500 kind:function(thumb,size=0xc) addr:0x0214f500\n"
            "func_ov006_0214f510 kind:function(thumb,size=0xd) addr:0x0214f510\n",
            image=bytes(image))
        counts, veneers = nr.address_names()
        self.assertEqual(counts["ov006"]["funcs"], 2)
        self.assertEqual(veneers["ov006"]["veneers"], 1)
        self.assertEqual(veneers["ov006"]["unmeasured"], 0)


if __name__ == "__main__":
    unittest.main()
