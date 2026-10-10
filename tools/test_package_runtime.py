"""Regression tests for extraction-safe release configuration."""
import hashlib
from pathlib import Path
import tempfile
import unittest
import zipfile

from package_runtime import make_archive


class RuntimeArchiveTests(unittest.TestCase):
    def test_extract_preserves_user_config(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            live = root / "d2rloader/config/controller-qol-updates.toml"
            live.parent.mkdir(parents=True)
            custom = b"# user tuning\r\n[aim]\r\ninitial_speed=19\r\n"
            live.write_bytes(custom)
            files = {
                "d2rloader/plugins/Controller QOL Updates.dll": b"fixture DLL",
                "defaults/controller-qol-updates.toml": b"[aim]\ninitial_speed=8\n",
                "defaults/map-assistance.toml": b"# reference\n",
            }
            archive = root / "update.zip"
            make_archive(archive, files)
            with zipfile.ZipFile(archive) as zipped:
                zipped.extractall(root)
                for line in zipped.read("SHA256SUMS").decode().splitlines():
                    digest, name = line.split("  ", 1)
                    self.assertEqual(digest, hashlib.sha256(zipped.read(name)).hexdigest())
            self.assertEqual(live.read_bytes(), custom)
            self.assertFalse((root / "d2rloader/config/map-assistance.toml").exists())
            self.assertEqual((root / "defaults/controller-qol-updates.toml").read_bytes(), files["defaults/controller-qol-updates.toml"])

    def test_live_configs_and_unsafe_paths_rejected(self):
        for name in ("d2rloader/config/controller-qol-updates.toml", "d2rloader/config/catalog.json",
                     "d2rloader/CONFIG/map-assistance.TOML", "controller-qol-updates.toml",
                     "../defaults/config.toml", "C:/defaults/config.toml", "/defaults/config.toml",
                     "defaults\\config.toml", "elsewhere/plugin.dll"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp:
                with self.assertRaises(SystemExit):
                    make_archive(Path(temp) / "bad.zip", {name: b"fixture"})


if __name__ == "__main__":
    unittest.main()
