"""Create the installable runtime ZIPs for a D2R-Mods GitHub release."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import re
import zipfile


REPOSITORY = Path(__file__).resolve().parents[1]


def read_version(source: Path, pattern: str) -> str:
    match = re.search(pattern, source.read_text(encoding="utf-8"))
    if not match:
        raise SystemExit(f"Could not read plugin version from {source}")
    return match.group(1)


def make_archive(path: Path, files: dict[str, bytes]) -> None:
    for name in files:
        parts = PurePosixPath(name).parts
        if name.startswith("/") or "\\" in name or ":" in name or ".." in parts:
            raise SystemExit(f"Unsafe archive path: {name}")
        if name.lower().startswith("d2rloader/config/"):
            raise SystemExit(f"Runtime archives must not overwrite user configuration: {name}")
        if name.lower().endswith(".toml") and not name.startswith("defaults/"):
            raise SystemExit(f"Reference config must be under defaults/ (never overwrite user config): {name}")
        if name.endswith(".dll") and not name.startswith("d2rloader/plugins/"):
            raise SystemExit(f"DLL is outside the installable layout: {name}")
    checksums = "".join(
        f"{hashlib.sha256(data).hexdigest()}  {name}\n"
        for name, data in sorted(files.items())
    ).encode("utf-8")
    contents = dict(files)
    contents["SHA256SUMS"] = checksums

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
        for line in archive.read("SHA256SUMS").decode("utf-8").splitlines():
            digest, name = line.split("  ", 1)
            if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                raise SystemExit(f"ZIP checksum mismatch: {path}: {name}")


def require_file(path: Path) -> bytes:
    if not path.is_file():
        raise SystemExit(f"Required file does not exist: {path}")
    return path.read_bytes()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--controller-dll", type=Path, required=True)
    parser.add_argument("--ranges-dll", type=Path)
    parser.add_argument("--map-assistance-dll", type=Path)
    parser.add_argument("--controller-only", action="store_true",
                        help="Package only Controller QOL (for focused prereleases).")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not args.controller_only and (not args.ranges_dll or not args.map_assistance_dll):
        parser.error("--ranges-dll and --map-assistance-dll are required unless --controller-only is set")
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

    controller_compatibility = f"""Controller QOL Updates by PyreFly
Plugin version: {controller_version}
D2RLoader plugin ABI: 4 (Windows x64 client)

Native hooks and layouts are qualified only for the builds documented in the
included production record. Every native site is guarded and fails open on a
mismatch. Close D2R before installing or replacing the DLL.
""".encode("utf-8")
    controller_files = {
        "d2rloader/plugins/Controller QOL Updates.dll": require_file(args.controller_dll),
        "defaults/controller-qol-updates.toml": require_file(
            controller / "controller-qol-updates.toml"
        ),
        "defaults/controller-qol-skills.toml": require_file(controller / "skill-defaults.toml"),
        "README.md": require_file(controller / "README.md"),
        "CHANGELOG.md": require_file(controller / "CHANGELOG.md"),
        "COMPATIBILITY.txt": controller_compatibility,
        "docs/PRODUCTION-1.3.1-rev.51.md": require_file(
            controller / "docs" / "PRODUCTION-1.3.1-rev.51.md"
        ),
    }
    controller_files["docs/IDENTIFY-REV11.md"] = require_file(controller / "docs" / "IDENTIFY-REV11.md")
    # Include the release's focused compatibility and native-contract records.
    for name in (
        "PRODUCTION-1.3.1-rev.60.md", "PRODUCTION-1.3.1-rev.73.md", "PRODUCTION-1.3.1-rev.75.md", "CRASH-TRIAGE-2026-10-10.md", "SKILL-TREE-AIM-TOGGLE.md", "GENERAL-SKILLS-OSKILLS.md", "SKILL-DISCOVERY-CONFIG.md",
        "PROJECTILE-LEADING.md", "LEADING-BASELINES-REV71.md", "leading-baselines.json",
        "NEUTRAL-A-2026-10-08.md", "NPC-INTERACTION-2026-10-08.md",
        "CRASH-TRIAGE-2026-10-08.md", "interaction-priority-evidence.json",
        "target-category-evidence.json",
        "SKILL-CATALOG.md", "AIM-CAST-OBSERVER-COMPATIBILITY.md",
        "IMPLEMENTATION-1.3.1-rev.52.md", "PRODUCTION-1.3.1-rev.50.md", "AIM-NATIVE-HISTORY.md", "PRODUCTION-1.3.1-rev.47.md",
        "LADDER-CONFLICTS-REV12.md", "LABEL-REFRESH-REV13.md", "LABEL-MODE-REV14.md",
        "IDENTIFY-STAT70-REV15.md", "AUTO-BELT-COMPATIBILITY-REV16.md",
        "VENDOR-BELT-REV17.md", "SHARED-SDK-REV18.md", "CHRONICLE-NAVIGATION-REV19.md",
        "GROUND-CALLBACK-CRASH-REV20.md", "CONTROLLER-HEADER-REV21.md", "NATIVE-RANGES-REV22.md",
        "LOOT-FILTER-EDITOR-INVESTIGATION.md", "IDENTIFY-NATIVE-REV26.md",
        "BULK-STASH-REV37.md", "AUTO-DEPOSIT-PICKUP-COMPATIBILITY-REV42.md",
        "STASH-WAYPOINT-PRIORITY-REV43.md", "SHRINE-CHEST-PRIORITY-REV44.md",
        "REMOTE-GROUND-LOOT.md", "REMOTE-ID-TRANSFER-TRACE.md",
        "REVIEW-2026-10-06.md", "CRASH-TRIAGE-2026-10-07.md",
        "BELT-NATIVE-CONTRACT.md", "LADDER-CONTROLLER-SCHEDULING.md",
    ):
        controller_files[f"docs/{name}"] = require_file(controller / "docs" / name)
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

    ranges_compatibility = f"""Item Roll Ranges by PyreFly
Plugin version: {ranges_version}
D2RLoader plugin ABI: 4 (Windows x64 client)
Required D2RCore.dll SHA-256:
2A868D013D2E0830BD2D9E04B918B19E46A73CF726C833E70D089B948FDEB5A2

The plugin also validates live code witnesses before activation. Close D2R
before installing or replacing the DLL.
""".encode("utf-8")
    ranges_files = {
        "d2rloader/plugins/Item Roll Ranges.dll": require_file(args.ranges_dll),
        "README.md": require_file(ranges / "docs" / "DISTRIBUTION-README.md"),
        "CHANGELOG.md": require_file(ranges / "CHANGELOG.md"),
        "COMPATIBILITY.txt": ranges_compatibility,
    }
    ranges_zip = args.output / f"Item-Roll-Ranges-{ranges_version}-PyreFly.zip"
    make_archive(ranges_zip, ranges_files)

    map_assistance_files = {
        "d2rloader/plugins/Map Assistance.dll": require_file(args.map_assistance_dll),
        "defaults/map-assistance.toml": require_file(
            map_assistance / "map-assistance.toml"
        ),
        "README.md": require_file(map_assistance / "DISTRIBUTION-README.md"),
        "CHANGELOG.md": require_file(map_assistance / "CHANGELOG.md"),
        "COMPATIBILITY.md": require_file(map_assistance / "COMPATIBILITY.md"),
        "CREDITS.md": require_file(map_assistance / "CREDITS.md"),
    }
    map_assistance_zip = args.output / (
        f"Map-Assistance-{map_assistance_version}-PyreFly.zip"
    )
    make_archive(map_assistance_zip, map_assistance_files)

    release_sums = "".join(
        f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n"
        for path in sorted(
            (controller_zip, ranges_zip, map_assistance_zip),
            key=lambda item: item.name,
        )
    )
    (args.output / "SHA256SUMS").write_text(release_sums, encoding="utf-8", newline="\n")
    print(controller_zip)
    print(ranges_zip)
    print(map_assistance_zip)
    print(args.output / "SHA256SUMS")


if __name__ == "__main__":
    main()
