"""Verify and package the standalone Inventory Deposit runtime."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess

from package_runtime import make_archive, read_version, require_file, runtime_readme


REPOSITORY = Path(__file__).resolve().parents[1]
CORE_SHA256 = "2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2"
RUNTIME_NAMES = frozenset({
    "d2rloader/plugins/Inventory Deposit.dll",
    "defaults/inventory-deposit.toml",
    "README.md",
})
MACHINE_PATH = re.compile(
    r"(?<![A-Za-z0-9])[A-Za-z]:[/\\]|\\\\[A-Za-z0-9][^\s\\]*\\|/(?:Users|home)/",
    re.IGNORECASE,
)


def reject_machine_paths(name: str, content: bytes) -> None:
    regions = [content]
    if name.endswith(".dll") and len(content) >= 64 and content.startswith(b"MZ"):
        # The artifact verifier has already admitted this PE. Inspect its data
        # sections (including CodeView/PDB strings), not executable instructions
        # or sprite pixels that can coincidentally resemble ASCII paths. The
        # embedded config's source is checked separately below.
        pe = struct.unpack_from("<I", content, 60)[0]
        if pe + 24 > len(content) or content[pe:pe + 4] != b"PE\0\0":
            raise SystemExit("Verified DLL has an invalid PE header")
        count = struct.unpack_from("<H", content, pe + 6)[0]
        section_table = pe + 24 + struct.unpack_from("<H", content, pe + 20)[0]
        if section_table + count * 40 > len(content):
            raise SystemExit("Verified DLL has an invalid section table")
        regions = []
        for offset in range(section_table, section_table + count * 40, 40):
            section = content[offset:offset + 8].split(b"\0", 1)[0]
            size, start = struct.unpack_from("<II", content, offset + 16)
            flags = struct.unpack_from("<I", content, offset + 36)[0]
            if not flags & 0x40 or section in (b".rsrc", b".pdata", b".xdata", b".reloc"):
                continue
            if start + size > len(content):
                raise SystemExit("Verified DLL has an invalid data section")
            regions.append(content[start:start + size])
    for region in regions:
        # Check narrow strings and both possible UTF-16 alignments.
        texts = [region.decode("utf-8", errors="replace")]
        texts.extend(region[alignment:].decode("utf-16-le", errors="replace")
                     for alignment in (0, 1))
        if any(MACHINE_PATH.search(text) for text in texts):
            raise SystemExit(f"Machine-specific path found in runtime file: {name}")


def package_inventory_deposit(dll: Path, verifier: Path, output: Path,
                              repository: Path = REPOSITORY,
                              tag: str | None = None) -> Path:
    plugin = repository / "plugins/inventory-deposit"
    version = read_version(plugin / "src/plugin_main.cpp", r'\.version\s*=\s*"([^"]+)"')
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:-beta\.\d+|\+rev\.\d+)", version):
        raise SystemExit(f"Expected a numeric Inventory Deposit beta or loader+revision version: {version}")
    prefix = "beta-inventory-deposit-" if "-beta." in version else "inventory-deposit-"
    if tag is not None and tag != prefix + version:
        raise SystemExit("Release tag does not match Inventory Deposit's source version")

    dll_bytes = require_file(dll)
    if not verifier.is_file():
        raise SystemExit(f"Artifact verifier does not exist: {verifier}")
    subprocess.run([str(verifier.resolve()), str(dll.resolve())], check=True)
    if require_file(dll) != dll_bytes:
        raise SystemExit("DLL changed while its artifact verification was running")

    files = {
        "d2rloader/plugins/Inventory Deposit.dll": dll_bytes,
        "defaults/inventory-deposit.toml": require_file(plugin / "inventory-deposit.toml"),
        "README.md": runtime_readme(plugin, repository),
    }
    if files.keys() != RUNTIME_NAMES:
        raise SystemExit("Unexpected Inventory Deposit runtime contents")
    for name, content in files.items():
        reject_machine_paths(name, content)

    output.mkdir(parents=True, exist_ok=True)
    archive = output / f"Inventory-Deposit-{version}-PyreFly.zip"
    make_archive(archive, files)
    (output / "SHA256SUMS").write_text(
        f"{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n",
        encoding="utf-8", newline="\n")
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dll", type=Path, required=True)
    parser.add_argument("--artifact-verifier", type=Path, required=True,
                        help="Built inventory-deposit-artifact-tests executable; runs on this exact DLL.")
    parser.add_argument("--tag", help="If supplied, must match beta-inventory-deposit-<version>.")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    archive = package_inventory_deposit(args.dll, args.artifact_verifier, args.output, tag=args.tag)
    print(archive)
    print(args.output / "SHA256SUMS")


if __name__ == "__main__":
    main()
