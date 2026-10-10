#!/usr/bin/env python3
"""Compile every C file in src/ and lib/ for each version, with the compiler and flags configure.py gives it, and
report the files that fail. It needs no base ROM, so CI runs it on every push and pull request; matching is still
checked by `ninja` with the ROMs.

    compile_all.py                  # every file, both versions
    compile_all.py --version b2_us  # one version
"""
import argparse
import os
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from configure import (CC_FLAGS, GENERATED_INCLUDE_DIR, INCLUDE_DIRS, MWCC_VERSION, VERSIONS,  # noqa: E402
                       download_tools, library_of)
from gen_constants import generate  # noqa: E402

TOOLS = ROOT / "tools"


def compile_file(source: Path, version: str, out_dir: Path) -> tuple[Path, str] | None:
    """Compiles one file, returning it and the compiler's output if it fails."""
    library = library_of(source)
    compiler, flags = library if library else (MWCC_VERSION, CC_FLAGS)
    obj = out_dir / version / source.with_suffix(".o")
    obj.parent.mkdir(parents=True, exist_ok=True)
    command = [
        str(TOOLS / "wibo"),
        str(TOOLS / "mwccarm" / compiler / "mwccarm.exe"),
        *(arg for flag in flags for arg in shlex.split(flag)),
        *(arg for define in VERSIONS[version]["defines"] for arg in ("-d", define)),
        *(arg for d in INCLUDE_DIRS for arg in ("-i", d)),
        "-o", str(obj),
        str(source),
    ]
    proc = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        return source, proc.stdout + proc.stderr
    return None


def generate_constants():
    """Writes the constant headers of data/constants/, as ninja does, since the sources include them."""
    out_dir = ROOT / GENERATED_INCLUDE_DIR / "constants"
    out_dir.mkdir(parents=True, exist_ok=True)
    for source in sorted((ROOT / "data" / "constants").glob("*.txt")):
        header = out_dir / source.with_suffix(".h").name
        text = generate(source)
        if not header.exists() or header.read_text() != text:
            header.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", choices=VERSIONS, action="append", help="version to compile, default every one")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count(), help="files to compile at once")
    args = parser.parse_args()

    download_tools(TOOLS)
    generate_constants()
    sources = sorted(p.relative_to(ROOT) for pattern in ("src/**/*.c", "lib/*/src/**/*.c") for p in ROOT.glob(pattern))
    versions = args.version or list(VERSIONS)
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(args.jobs) as pool:
        jobs = [pool.submit(compile_file, source, version, Path(tmp)) for version in versions for source in sources]
        failures = [result for job in jobs if (result := job.result())]
    for source, output in failures:
        print(f"{source}:\n{output}")
    print(f"{len(sources) * len(versions) - len(failures)} of {len(sources) * len(versions)} compiled "
          f"({len(sources)} files, {', '.join(versions)})")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
