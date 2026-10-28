"""Inventory Deposit release regression checks; no game or game assets needed."""

import hashlib
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from package_inventory_deposit import (
    CORE_SHA256, RUNTIME_NAMES, package_inventory_deposit, reject_machine_paths,
)


class InventoryDepositPackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repo = self.root / "repo"
        self.plugin = self.repo / "plugins/inventory-deposit"
        fixtures = {
            "plugins/inventory-deposit/src/plugin_main.cpp": b'.version="0.1.0-beta.13",',
            "plugins/inventory-deposit/inventory-deposit.toml": b"[inventory_deposit]\nenabled = true\n",
            "plugins/inventory-deposit/README.md": b"Install the DLL once.\nhttps://github.com/Pyre-Flies/D2R-Mods\n",
            "plugins/inventory-deposit/RUNTIME-RELEASE-NOTES.md": b"# Changelog\nInitial public prerelease.\n",
            "LICENSE": b"Plugin license fixture\n",
            "third_party/D2RLoader-PluginSDK/LICENSE": b"SDK license fixture\n",
            "plugins/inventory-deposit/assets/art/normal.png": b"private art fixture",
            "plugins/inventory-deposit/local.log": b"private log fixture",
            "plugins/inventory-deposit/old.dll.bak": b"private backup fixture",
            "plugins/inventory-deposit/reference.mpq": b"game asset fixture",
        }
        for name, content in fixtures.items():
            path = self.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
        self.dll = self.root / "built/Inventory Deposit.dll"
        self.dll.parent.mkdir()
        self.dll.write_bytes(b"MZ\x00exact tested DLL bytes\xff\x00")
        self.verifier = self.root / "built/inventory-deposit-artifact-tests.exe"
        self.verifier.write_bytes(b"mocked verifier fixture")
        self.output = self.root / "dist"
        self.run = self.enterContext(patch("package_inventory_deposit.subprocess.run"))

    def package(self, **kwargs):
        return package_inventory_deposit(self.dll, self.verifier, self.output,
                                         repository=self.repo, **kwargs)

    def test_exact_dll_layout_checksums_and_preserved_live_config(self):
        archive = self.package(tag="beta-inventory-deposit-0.1.0-beta.13")
        self.run.assert_called_once_with(
            [str(self.verifier.resolve()), str(self.dll.resolve())], check=True)
        self.assertEqual(archive.name, "Inventory-Deposit-0.1.0-beta.13-PyreFly.zip")
        live = self.root / "install/d2rloader/config/inventory-deposit.toml"
        live.parent.mkdir(parents=True)
        customized = b"# user's values\r\n[inventory_deposit]\r\ngold_gap = 99\r\n"
        live.write_bytes(customized)
        with zipfile.ZipFile(archive) as zipped:
            self.assertEqual(set(zipped.namelist()), RUNTIME_NAMES)
            self.assertEqual(zipped.read("d2rloader/plugins/Inventory Deposit.dll"), self.dll.read_bytes())
            self.assertNotIn("SHA256SUMS", zipped.namelist())
            self.assertNotIn("README.md", zipped.namelist())
            zipped.extractall(self.root / "install")
        self.assertEqual(live.read_bytes(), customized)
        self.assertFalse((self.root / "install/defaults").exists())
        self.assertEqual((self.output / "SHA256SUMS").read_text(),
                         f"{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n")

    def test_reproducible_archive_and_no_config_created_on_fresh_install(self):
        archive = self.package()
        initial = archive.read_bytes()
        self.assertEqual(self.package().read_bytes(), initial)
        with zipfile.ZipFile(archive) as zipped:
            zipped.extractall(self.root / "fresh")
        self.assertFalse((self.root / "fresh/d2rloader/config").exists())

    def test_verifier_failure_blocks_archive(self):
        self.run.side_effect = subprocess.CalledProcessError(1, ["verifier"])
        with self.assertRaises(subprocess.CalledProcessError):
            self.package()
        self.assertFalse(self.output.exists())

    def test_dll_change_during_verification_blocks_archive(self):
        self.run.side_effect = lambda *args, **kwargs: self.dll.write_bytes(b"changed")
        with self.assertRaisesRegex(SystemExit, "DLL changed"):
            self.package()
        self.assertFalse(self.output.exists())

    def test_missing_verifier_or_required_document_blocks_archive(self):
        self.verifier.unlink()
        with self.assertRaisesRegex(SystemExit, "verifier does not exist"):
            self.package()
        self.run.assert_not_called()
        self.verifier.write_bytes(b"restored fixture")
        (self.plugin / "README.md").unlink()
        with self.assertRaisesRegex(SystemExit, "Required file does not exist"):
            self.package()
        self.assertFalse(self.output.exists())

    def test_stable_loader_revision_and_channel(self):
        (self.plugin / "src/plugin_main.cpp").write_text('.version="1.3.1+rev.13",')
        archive = self.package(tag="inventory-deposit-1.3.1+rev.13")
        self.assertEqual(archive.name, "Inventory-Deposit-1.3.1+rev.13-PyreFly.zip")

    def test_stable_version_under_beta_tag_is_rejected(self):
        (self.plugin / "src/plugin_main.cpp").write_text('.version="1.3.1+rev.13",')
        with self.assertRaisesRegex(SystemExit, "tag does not match"):
            self.package(tag="beta-inventory-deposit-1.3.1+rev.13")
        self.run.assert_not_called()

    def test_tag_and_version_mismatch_rejected_before_verification(self):
        with self.assertRaisesRegex(SystemExit, "tag does not match"):
            self.package(tag="beta-inventory-deposit-0.1.0-beta.12")
        (self.plugin / "src/plugin_main.cpp").write_text('.version="../../unsafe",')
        with self.assertRaisesRegex(SystemExit, "numeric Inventory Deposit"):
            self.package()
        self.run.assert_not_called()

    def test_private_paths_blocked_in_docs_config_and_dll_data_sections(self):
        pe = bytearray(256)
        pe[:2] = b"MZ"
        pe[60:64] = struct.pack("<I", 64)
        pe[64:68] = b"PE\0\0"
        pe[70:72] = struct.pack("<H", 1)
        pe[84:86] = struct.pack("<H", 24)
        section = 64 + 24 + 24
        pe[section:section + 6] = b".rdata"
        pe[section + 16:section + 24] = struct.pack("<II", 64, 160)
        pe[section + 36:section + 40] = struct.pack("<I", 0x40000040)
        pe[160:191] = b"C:\\Users\\private\\deposit.pdb\x00"
        cases = (
            (self.plugin / "README.md", b"Built from I:\\D2R-Mods\\plugins"),
            (self.plugin / "inventory-deposit.toml", b"# /Users/private/config"),
            (self.dll, bytes(pe)),
        )
        for path, bad in cases:
            original = path.read_bytes()
            with self.subTest(path=path.name):
                path.write_bytes(bad)
                with self.assertRaisesRegex(SystemExit, "Machine-specific path"):
                    self.package()
                self.assertFalse(self.output.exists())
                path.write_bytes(original)


if __name__ == "__main__":
    unittest.main()
