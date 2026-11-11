"""complete_report.py on a small report.

    python3 -m unittest tools.build.test_complete_report
"""
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from tools.build.complete_report import complete_report, linker_output_gaps


def unit(name, complete, categories, code, matched, functions, matched_functions, data=0, matched_data=0, fuzzy=99.0):
    return {
        "name": name,
        "measures": {"fuzzy_match_percent": fuzzy, "total_code": str(code), "matched_code": str(matched),
                     "total_data": str(data), "matched_data": str(matched_data), "total_functions": functions,
                     "matched_functions": matched_functions, "complete_code": str(code if complete else 0),
                     "complete_data": str(data if complete else 0), "total_units": 1,
                     "complete_units": 1 if complete else 0},
        "functions": [{"name": "f", "size": str(code), "fuzzy_match_percent": fuzzy}],
        "sections": [{"name": ".text", "size": str(code), "fuzzy_match_percent": fuzzy}],
        "metadata": {"complete": complete, "progress_categories": categories},
    }


class StandardLibraryTests(unittest.TestCase):
    """The build runs this script with a Python that has only the standard library."""

    def test_it_imports_without_pyyaml(self):
        blocked = "import sys; sys.modules['yaml'] = None; import tools.build.complete_report"
        result = subprocess.run([sys.executable, "-c", blocked], cwd=Path(__file__).resolve().parents[2],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)


class CompleteReportTests(unittest.TestCase):
    def setUp(self):
        self.report = complete_report({
            "measures": {},
            "categories": [{"id": "main", "name": "Main", "measures": {}}, {"id": "lib", "name": "Lib", "measures": {}}],
            "units": [unit("a", True, ["main"], 100, 60, 4, 3, data=40, matched_data=10),
                      unit("b", False, ["main", "lib"], 300, 150, 6, 2, data=60, matched_data=30, fuzzy=50.0)],
        })

    def test_complete_unit_is_matched(self):
        a = self.report["units"][0]
        self.assertEqual(a["measures"]["matched_code"], "100")
        self.assertEqual(a["measures"]["matched_data"], "40")
        self.assertEqual(a["measures"]["matched_functions"], 4)
        self.assertEqual(a["functions"][0]["fuzzy_match_percent"], 100.0)

    def test_incomplete_unit_keeps_objdiff_score(self):
        b = self.report["units"][1]
        self.assertEqual(b["measures"]["matched_code"], "150")
        self.assertEqual(b["functions"][0]["fuzzy_match_percent"], 50.0)

    def test_totals_and_categories_are_recomputed(self):
        m = self.report["measures"]
        self.assertEqual((m["matched_code"], m["matched_functions"], m["complete_units"]), ("250", 6, 1))
        self.assertAlmostEqual(m["matched_code_percent"], 62.5)
        self.assertAlmostEqual(m["fuzzy_match_percent"], (100 * 100 + 50 * 300) / 400)
        main, lib = self.report["categories"]
        self.assertEqual(main["measures"]["matched_code"], "250")
        self.assertEqual(lib["measures"]["matched_code"], "150")


if __name__ == "__main__":
    unittest.main()


def elf(sections):
    """A minimal ELF32 object with the given (contents or None for .bss, size) allocated sections."""
    blob, headers = b"", [b"\0" * 40]
    for contents, size in sections:
        offset = 52 + len(blob)
        kind = 8 if contents is None else 1
        headers.append(struct.pack("<10I", 0, kind, 2, 0, offset, size, 0, 0, 4, 0))
        blob += contents or b""
    shoff = 52 + len(blob)
    header = b"\x7fELF" + b"\1\1\1" + b"\0" * 9 + struct.pack("<HHIIIIIHHHHHH", 1, 40, 1, 0, 0, shoff, 0, 52, 0, 0,
                                                                   40, len(headers), 0)
    return header + blob + b"".join(headers)


class LinkerOutputGapTests(unittest.TestCase):
    """A gap is left out of the report only when it holds the linker's output: zeros at an autoload's sinit_start, or
    zeros that align a module section's end to 32 bytes."""

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.arm9 = self.root / "config" / "b2_us" / "arm9"

    def write(self, path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(text, bytes):
            path.write_bytes(text)
        else:
            path.write_text(text)

    def gap(self, name, *sections):
        self.write(self.root / "build" / "b2_us" / "delinks" / f"_dsd_gap@{name}.o", elf(sections))

    def delinks(self, module, text):
        self.write(self.arm9 / (module + "/delinks.txt" if module != "main" else "delinks.txt"), text)

    def test_the_linkers_output_is_found_and_other_gaps_stay(self):
        self.delinks("main", "    .data       start:0x02000000 end:0x02000060 kind:data align:32\n\n"
                             "a.c:\n    .data start:0x02000000 end:0x02000010\n"
                             "b.c:\n    .data start:0x02000018 end:0x02000048\n")
        # main_0: 8 zeros between two files; main_1: zero fill to the section's 32-aligned end
        self.gap("main_0", (b"\0" * 8, 8))
        self.gap("main_1", (b"\0" * 0x18, 0x18))
        self.delinks("itcm", "    .text       start:0x01ff8000 end:0x01ff8040 kind:code align:32\n\n"
                             "c.c:\n    .text start:0x01ff8020 end:0x01ff8040\n")
        self.gap("itcm_0", (b"\0" * 0x20, 0x20))
        self.write(self.root / "extract" / "b2_us" / "arm9" / "itcm.yaml", "base_address: 33521664\nsinit_start: 33521664\n")
        self.delinks("overlays/ov081", "    .data       start:0x021eac80 end:0x021eaca0 kind:data align:32\n\n"
                                       "d.c:\n    .data start:0x021eac80 end:0x021eac90\n")
        self.gap("ov081_0", (b"\0" * 15 + b"\1", 0x10))
        found = linker_output_gaps(self.root, "b2_us")
        self.assertEqual(found, {"_dsd_gap@main_1", "_dsd_gap@itcm_0"})

    def test_migrated_dtcm_sections_and_bss_fill(self):
        self.delinks("main", "    .data       start:0x02000000 end:0x02000020 kind:data align:32\n\n"
                             "e.c:\n    .data start:0x02000000 end:0x02000020\n"
                             "    .dtcm.data  start:0x02fe0020 end:0x02fe00a0\n    .dtcm.bss   start:0x02fe00a0 end:0x02fe00a8\n")
        self.delinks("dtcm", "    .data       start:0x02fe0000 end:0x02fe00a0 kind:data align:32\n"
                             "    .bss        start:0x02fe00a0 end:0x02fe00c0 kind:bss align:32\n")
        self.write(self.root / "extract" / "b2_us" / "arm9" / "dtcm.yaml", "sinit_start: 50200576\n")
        self.gap("dtcm_0", (b"\0" * 0x20, 0x20))
        self.gap("dtcm_1", (None, 0x18))
        self.assertEqual(linker_output_gaps(self.root, "b2_us"), {"_dsd_gap@dtcm_0", "_dsd_gap@dtcm_1"})

    def test_a_gap_whose_object_is_another_size_stays(self):
        self.delinks("main", "    .data       start:0x02000000 end:0x02000020 kind:data align:32\n\n"
                             "a.c:\n    .data start:0x02000000 end:0x02000010\n")
        self.gap("main_0", (b"\0" * 8, 8))
        self.assertEqual(linker_output_gaps(self.root, "b2_us"), set())

    def test_the_report_leaves_them_out(self):
        report = complete_report({"measures": {}, "categories": [],
                                  "units": [unit("a", True, ["main"], 100, 100, 1, 1),
                                            unit("_dsd_gap@itcm_0", False, ["main"], 0, 0, 0, 0, data=32)]},
                                 {"_dsd_gap@itcm_0"})
        self.assertEqual([u["name"] for u in report["units"]], ["a"])
        self.assertEqual((report["measures"]["total_units"], report["measures"]["complete_units"]), (1, 1))
