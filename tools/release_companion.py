"""Fetch and verify the exact reviewed Buff HUD companion package."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import io
import json
import shutil
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def manifest(root=ROOT):
    return json.loads((root / "release-companions.json").read_text(encoding="utf-8"))["buffhud"]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def verify(data, expected):
    if digest(data) != expected["sha256"]:
        raise ValueError("Buff HUD archive identity mismatch")
    with zipfile.ZipFile(io.BytesIO(data)) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or archive.testzip() is not None:
            raise ValueError("Invalid or duplicate archive entries")
        for name in names:
            path = PurePosixPath(name)
            if path.is_absolute() or ".." in path.parts or "\\" in name or ":" in name:
                raise ValueError("Unsafe companion archive path")
            if name.lower().startswith("d2rloader/config/") or name.lower().endswith(".toml"):
                raise ValueError("Companion must preserve active configuration")
        expected_entries = {"d2rloader/plugins/d2rl-buffhud.dll", "d2rloader/plugins/d2rl-buffhud.mpq"}
        if set(names) != expected_entries:
            raise ValueError("Companion must contain only d2rl-buffhud.dll and d2rl-buffhud.mpq")
        dll = archive.read("d2rloader/plugins/d2rl-buffhud.dll")
        if digest(dll) != expected["dllSha256"]:
            raise ValueError("Unreviewed Buff HUD DLL")
        mpq = archive.read("d2rloader/plugins/d2rl-buffhud.mpq")
        if digest(mpq) != expected["mpqSha256"]:
            raise ValueError("Unreviewed Buff HUD MPQ")
        return dll


def stage(source, output, root=ROOT):
    expected = manifest(root)
    data = Path(source).read_bytes()
    verify(data, expected)
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    target = output / expected["asset"]
    if Path(source).resolve() != target.resolve():
        shutil.copyfile(source, target)
    if target.read_bytes() != data:
        raise ValueError("Companion changed during packaging")
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    expected = manifest()
    url = f'https://github.com/{expected["repository"]}/releases/download/{expected["tag"]}/{expected["asset"]}'
    request = urllib.request.Request(url, headers={"User-Agent": "D2R-Mods-release-verifier"})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read(8 * 1024 * 1024 + 1)
    if len(data) > 8 * 1024 * 1024:
        raise ValueError("Unexpected companion size")
    dll = verify(data, expected)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / expected["asset"]).write_bytes(data)
    (args.output / "d2rl-buffhud.dll").write_bytes(dll)
    print("Verified Buff HUD companion package and exact native artifacts.")


if __name__ == "__main__":
    main()
