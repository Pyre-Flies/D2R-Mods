"""Create the installable runtime ZIPs for a D2R-Mods GitHub release."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import re
import posixpath
import subprocess
import zipfile


REPOSITORY = Path(__file__).resolve().parents[1]


def read_version(source: Path, pattern: str) -> str:
    match = re.search(pattern, source.read_text(encoding="utf-8"))
    if not match:
        raise SystemExit(f"Could not read plugin version from {source}")
    return match.group(1)


def check_public_text(name: str, data: bytes) -> None:
    """Reject private/workflow text in the curated end-user documents."""
    if not name.lower().endswith((".md", ".txt", ".toml")):
        return
    text = data.decode("utf-8", errors="replace")
    if re.search(r"\b(?:Codex|ChatGPT|OpenAI)\b|AI[- ](?:assisted|generated|tools?)", text, re.I):
        raise SystemExit(f"Internal tooling reference in runtime document: {name}")
    if re.search(r"(?mi)^\s*(?:cmake\s|ctest\s|git clone\s|python -m pip\s)", text):
        raise SystemExit(f"Build instructions in end-user runtime document: {name}")


def make_archive(path: Path, files: dict[str, bytes]) -> None:
    for name in files:
        archive_path = PurePosixPath(name)
        parts = archive_path.parts
        if (not parts or name.startswith("/") or "\\" in name or ":" in name or
                ".." in parts or archive_path.as_posix() != name):
            raise SystemExit(f"Unsafe archive path: {name}")
        if name.lower().startswith("d2rloader/config/"):
            raise SystemExit(f"Runtime archives must not overwrite user configuration: {name}")
        if name.lower().endswith(".toml") and not name.startswith("defaults/"):
            raise SystemExit(f"Reference config must be under defaults/ (never overwrite user config): {name}")
        if name.lower().endswith(".dll") and not name.startswith("d2rloader/plugins/"):
            raise SystemExit(f"DLL is outside the installable layout: {name}")
        if name.lower() == "sha256sums":
            raise SystemExit("Checksums belong alongside runtime ZIPs")
        if name != "README.md" and not name.lower().endswith((".toml", ".dll")):
            raise SystemExit(f"Unexpected runtime content: {name}")
    from package_inventory_deposit import reject_machine_paths
    for name, content in files.items():
        reject_machine_paths(name, content)
        check_public_text(name, content)
    contents = dict(files)
    expected_hashes = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}

    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(contents.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, data, compresslevel=9)

    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise SystemExit(f"ZIP integrity check failed: {path}")
        if set(archive.namelist()) != set(contents):
            raise SystemExit(f"ZIP file list mismatch: {path}")
        for name, digest in expected_hashes.items():
            if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                raise SystemExit(f"ZIP content mismatch: {path}: {name}")


def require_file(path: Path) -> bytes:
    if not path.is_file():
        raise SystemExit(f"Required file does not exist: {path}")
    return path.read_bytes()


def runtime_readme(plugin: Path, repository: Path) -> bytes:
    """One usage document, with required redistribution notices inline."""
    text = require_file(plugin / "README.md").decode("utf-8").rstrip()
    # Repository-relative links become usable from the standalone ZIP. Local
    # section anchors remain local to its README.
    base = plugin.relative_to(repository).as_posix()
    def public_link(match):
        target = match.group(1)
        if target.startswith(("http:", "https:", "mailto:", "#")):
            return match.group(0)
        resolved = posixpath.normpath(posixpath.join(base, target))
        return "](" + "https://github.com/Pyre-Flies/D2R-Mods/blob/main/" + resolved + ")"
    text = re.sub(r"\]\(([^)]+)\)", public_link, text)
    readme = text.encode("utf-8")
    notices = (b"\n\n## Notices\n\n" + require_file(repository / "LICENSE").strip() +
               b"\n\n### D2RLoader Plugin SDK\n\n" +
               require_file(repository / "third_party/D2RLoader-PluginSDK/LICENSE").strip() + b"\n")
    return readme + notices


def package_floating_damage(dll: Path, verifier: Path, output: Path,
                            repository: Path = REPOSITORY) -> Path:
    """Package the exact verified DLL and its public runtime documentation."""
    from package_inventory_deposit import reject_machine_paths

    plugin = repository / "plugins/floating-damage"
    version = read_version(plugin / "src/plugin.cpp", r'\.version\s*=\s*"([^"]+)"')
    if not re.fullmatch(r"\d+\.\d+\.\d+(?:-beta\.\d+|\+rev\.\d+)", version):
        raise SystemExit(f"Expected a numeric Floating Damage beta or loader+revision version: {version}")
    dll_bytes = require_file(dll)
    if not verifier.is_file():
        raise SystemExit(f"Artifact verifier does not exist: {verifier}")
    subprocess.run([str(verifier.resolve()), str(dll.resolve())], check=True)
    if require_file(dll) != dll_bytes:
        raise SystemExit("DLL changed while its artifact verification was running")

    files = {
        "d2rloader/plugins/Floating Damage.dll": dll_bytes,
        "defaults/floating-damage.toml": require_file(plugin / "floating-damage.toml"),
        "README.md": runtime_readme(plugin, repository),
    }
    for name, content in files.items():
        reject_machine_paths(name, content)
    output.mkdir(parents=True, exist_ok=True)
    archive = output / f"Floating-Damage-{version}-PyreFly.zip"
    make_archive(archive, files)
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--controller-dll", type=Path, required=True)
    parser.add_argument("--ranges-dll", type=Path)
    parser.add_argument("--map-assistance-dll", type=Path)
    parser.add_argument("--inventory-deposit-dll", type=Path)
    parser.add_argument("--inventory-deposit-artifact-verifier", type=Path)
    parser.add_argument("--floating-damage-dll", type=Path)
    parser.add_argument("--floating-damage-artifact-verifier", type=Path)
    parser.add_argument("--always-show-sockets-dll", type=Path)
    parser.add_argument("--always-show-sockets-artifact-verifier", type=Path)
    parser.add_argument("--controller-only", action="store_true",
                        help="Package only Controller QOL (for focused prereleases).")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    required_suite_args = (
        "ranges_dll", "map_assistance_dll", "inventory_deposit_dll",
        "inventory_deposit_artifact_verifier", "floating_damage_dll",
        "floating_damage_artifact_verifier", "always_show_sockets_dll",
        "always_show_sockets_artifact_verifier",
    )
    if not args.controller_only:
        missing = ["--" + name.replace("_", "-") for name in required_suite_args
                   if getattr(args, name) is None]
        if missing:
            parser.error(f"Required unless --controller-only is set: {', '.join(missing)}")
    args.output.mkdir(parents=True, exist_ok=True)

    controller = REPOSITORY / "plugins" / "controller-qol"
    ranges = REPOSITORY / "plugins" / "item-roll-ranges"
    map_assistance = REPOSITORY / "plugins" / "map-assistance"
    controller_version = read_version(
        controller / "src" / "plugin_main.cpp",
        r'\.version\s*=\s*"([^"]+)"',
    )
    ranges_version = read_version(
        ranges / "src" / "plugin.cpp",
        r'"item-roll-ranges",\s*"Item Roll Ranges",\s*"([^"]+)"',
    )
    map_assistance_version = read_version(
        map_assistance / "src" / "plugin.cpp",
        r'\.version\s*=\s*"([^"]+)"',
    )

    controller_files = {
        "d2rloader/plugins/Controller QOL Updates.dll": require_file(args.controller_dll),
        "defaults/controller-qol-updates.toml": require_file(controller / "controller-qol-updates.toml"),
        "defaults/controller-qol-skills.toml": require_file(controller / "skill-defaults.toml"),
        "README.md": runtime_readme(controller, REPOSITORY),
    }
    controller_zip = args.output / (
        f"Controller-QOL-Updates-{controller_version}-PyreFly.zip"
    )
    make_archive(controller_zip, controller_files)
    if args.controller_only:
        (args.output / "SHA256SUMS").write_text(
            f"{hashlib.sha256(controller_zip.read_bytes()).hexdigest()}  {controller_zip.name}\n",
            encoding="utf-8", newline="\n")
        print(controller_zip)
        print(args.output / "SHA256SUMS")
        return

    ranges_files = {
        "d2rloader/plugins/Item Roll Ranges.dll": require_file(args.ranges_dll),
        "defaults/item-roll-ranges.toml": require_file(ranges / "item-roll-ranges.toml"),
        "README.md": runtime_readme(ranges, REPOSITORY),
    }
    ranges_zip = args.output / f"Item-Roll-Ranges-{ranges_version}-PyreFly.zip"
    make_archive(ranges_zip, ranges_files)

    map_assistance_files = {
        "d2rloader/plugins/Map Assistance.dll": require_file(args.map_assistance_dll),
        "defaults/map-assistance.toml": require_file(map_assistance / "map-assistance.toml"),
        "README.md": runtime_readme(map_assistance, REPOSITORY),
    }
    map_assistance_zip = args.output / (
        f"Map-Assistance-{map_assistance_version}-PyreFly.zip"
    )
    make_archive(map_assistance_zip, map_assistance_files)

    # Reuse the standalone deposit packager's exact-DLL verification, privacy
    # checks, embedded-artwork documentation and license contract. Its focused
    # checksum file is replaced below by the complete suite manifest.
    from package_inventory_deposit import package_inventory_deposit
    inventory_deposit_zip = package_inventory_deposit(
        args.inventory_deposit_dll, args.inventory_deposit_artifact_verifier,
        args.output, repository=REPOSITORY)
    floating_damage_zip = package_floating_damage(
        args.floating_damage_dll, args.floating_damage_artifact_verifier,
        args.output, repository=REPOSITORY)
    from package_always_show_sockets import package as package_sockets
    sockets_zip = package_sockets(args.always_show_sockets_dll,
        args.always_show_sockets_artifact_verifier, args.output, root=REPOSITORY)
    archives = (controller_zip, ranges_zip, map_assistance_zip,
                inventory_deposit_zip, floating_damage_zip, sockets_zip)
    unexpected = set(args.output.glob("*.zip")) - set(archives)
    if unexpected:
        raise SystemExit("Output contains unrelated ZIPs; use a clean output directory: " +
                         ", ".join(sorted(path.name for path in unexpected)))
    release_sums = "".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
        for path in sorted(
            archives,
            key=lambda item: item.name,
        )
    )
    (args.output / "SHA256SUMS").write_text(release_sums, encoding="utf-8", newline="\n")
    print(controller_zip)
    print(ranges_zip)
    print(map_assistance_zip)
    print(inventory_deposit_zip)
    print(floating_damage_zip)
    print(sockets_zip)
    print(args.output / "SHA256SUMS")


if __name__ == "__main__":
    main()
