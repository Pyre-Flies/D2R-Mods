"""Regression tests for extraction-safe ten-plugin lean runtime releases."""
from contextlib import redirect_stdout, redirect_stderr
import hashlib
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import package_runtime
from package_runtime import make_archive, package_floating_damage


class RuntimeArchiveTests(unittest.TestCase):
    def test_extract_preserves_user_config_and_leaves_no_extra_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            live = root / "d2rloader/config/controller-qol-updates.toml"
            live.parent.mkdir(parents=True)
            custom = b"# user tuning\r\n[aim]\r\ninitial_speed=19\r\n"
            live.write_bytes(custom)
            files = {
                "d2rloader/plugins/Controller QOL Updates.dll": b"fixture DLL",
            }
            archive = root / "update.zip"
            make_archive(archive, files)
            with zipfile.ZipFile(archive) as zipped:
                zipped.extractall(root)
                for name, data in files.items():
                    self.assertEqual(zipped.read(name), data)
                self.assertEqual(set(zipped.namelist()), {"d2rloader/plugins/Controller QOL Updates.dll"})
            self.assertEqual(live.read_bytes(), custom)
            self.assertFalse((root / "defaults").exists())
            self.assertFalse((root / "README.md").exists())

    def test_live_configs_defaults_docs_and_unsafe_paths_rejected(self):
        for name in ("d2rloader/config/controller-qol-updates.toml", "d2rloader/config/catalog.json",
                     "d2rloader/CONFIG/map-assistance.TOML", "controller-qol-updates.toml",
                     "defaults/controller-qol-updates.toml", "README.md", "SHA256SUMS",
                     "../defaults/config.toml", "C:/defaults/config.toml", "/defaults/config.toml",
                     "defaults\\config.toml", "elsewhere/plugin.dll", "elsewhere/PLUGIN.DLL",
                     "./d2rloader/config/catalog.json", ""):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as temp:
                with self.assertRaises(SystemExit):
                    make_archive(Path(temp) / "bad.zip", {name: b"fixture"})


class PublicDocumentationTests(unittest.TestCase):
    def test_private_tooling_and_build_recipes_block_runtime_docs(self):
        for content in ("Generated with Codex", "ChatGPT tools", "OpenAI workflow",
                        "AI-assisted build", "cmake --build build/plugin", "ctest --test-dir build"):
            with self.subTest(content=content), self.assertRaises(SystemExit):
                package_runtime.check_public_text("README.md", content.encode())

    def test_user_commands_and_reference_settings_are_allowed(self):
        package_runtime.check_public_text("README.md", b"/AlwaysShowRolls on\n/sockets size 65\n/damagenumbers off\n")
        package_runtime.check_public_text("defaults/example.toml", b"[display]\nalways_show_ranges = false\n")


class FloatingDamagePackageFixture:
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repo = self.root / "repo"
        self.plugin = self.repo / "plugins/floating-damage"
        fixtures = {
            "plugins/floating-damage/src/plugin.cpp": b'.version="0.1.0-beta.11",',
            "plugins/floating-damage/floating-damage.toml": b"[floating-damage]\nenabled = true\n",
            "plugins/floating-damage/README.md": b"# Floating Damage\nInstall with the loader.\n",
            "plugins/floating-damage/RUNTIME-RELEASE-NOTES.md": b"# Changelog\nCritical number styling.\n",
            "plugins/floating-damage/COMPATIBILITY.txt": b"# Native contract\nExact guarded build only.\n",
            "LICENSE": b"Plugin license fixture\n",
            "third_party/D2RLoader-PluginSDK/LICENSE": b"SDK license fixture\n",
            "plugins/floating-damage/private.log": b"private capture fixture",
            "plugins/floating-damage/old.dll.bak": b"private backup fixture",
        }
        for name, data in fixtures.items():
            path = self.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.dll = self.root / "built/Floating Damage.dll"
        self.dll.parent.mkdir()
        self.dll.write_bytes(b"MZ\0exact tested floating DLL fixture\xff\0")
        self.verifier = self.root / "built/floating-damage-artifact-tests.exe"
        self.verifier.write_bytes(b"mock verifier fixture")
        self.output = self.root / "dist"
        self.run = self.enterContext(patch("package_runtime.subprocess.run"))

    def package(self):
        return package_floating_damage(self.dll, self.verifier, self.output,
                                       repository=self.repo)


class FloatingDamagePackageTests(FloatingDamagePackageFixture, unittest.TestCase):
    def test_exact_verified_runtime_contents_checksums_and_reproducibility(self):
        archive = self.package()
        self.run.assert_called_once_with(
            [str(self.verifier.resolve()), str(self.dll.resolve())], check=True)
        self.assertEqual(archive.name, "Floating-Damage-0.1.0-beta.11-PyreFly.zip")
        expected = {
            "d2rloader/plugins/Floating Damage.dll",
        }
        original = archive.read_bytes()
        with zipfile.ZipFile(archive) as zipped:
            self.assertEqual(set(zipped.namelist()), expected)
            self.assertEqual(zipped.read("d2rloader/plugins/Floating Damage.dll"),
                             self.dll.read_bytes())
        self.assertEqual(self.package().read_bytes(), original)

    def test_verifier_failure_or_dll_change_blocks_package(self):
        self.run.side_effect = subprocess.CalledProcessError(1, ["verifier"])
        with self.assertRaises(subprocess.CalledProcessError):
            self.package()
        self.assertFalse(self.output.exists())
        self.run.side_effect = lambda *args, **kwargs: self.dll.write_bytes(b"changed DLL")
        with self.assertRaisesRegex(SystemExit, "DLL changed"):
            self.package()
        self.assertFalse(self.output.exists())

    def test_stable_loader_revision_package(self):
        (self.plugin / "src/plugin.cpp").write_text('.version="1.3.1+rev.12",')
        self.assertEqual(self.package().name, "Floating-Damage-1.3.1+rev.12-PyreFly.zip")

    def test_missing_verifier_or_public_document_blocks_package(self):
        self.verifier.unlink()
        with self.assertRaisesRegex(SystemExit, "verifier does not exist"):
            self.package()
        self.run.assert_not_called()
        self.verifier.write_bytes(b"restored fixture")
        (self.plugin / "README.md").unlink()
        with self.assertRaisesRegex(SystemExit, "Required file does not exist"):
            self.package()
        self.assertFalse(self.output.exists())

    def test_source_version_is_safe_before_verifying(self):
        (self.plugin / "src/plugin.cpp").write_text('.version="../../unsafe",')
        with self.assertRaisesRegex(SystemExit, "numeric Floating Damage"):
            self.package()
        self.run.assert_not_called()
        self.assertFalse(self.output.exists())

    def test_private_paths_rejected_in_docs_configuration_and_dll_strings(self):
        cases = (
            (self.plugin / "README.md", b"C:/Users/private/build"),
            (self.plugin / "README.md", b"D:\\work\\private\\capture"),
            (self.plugin / "floating-damage.toml", b"# /home/private/config"),
            (self.dll, b"MZ\0C:\\Users\\private\\floating.pdb\0"),
        )
        for path, content in cases:
            original = path.read_bytes()
            with self.subTest(path=path.name):
                path.write_bytes(content)
                with self.assertRaisesRegex(SystemExit, "Machine-specific path"):
                    self.package()
                self.assertFalse(self.output.exists())
                path.write_bytes(original)


class TenPluginSuiteTests(FloatingDamagePackageFixture, unittest.TestCase):
    def setUp(self):
        super().setUp()
        fixtures = {
            "plugins/gem-bag-ui/src/plugin.cpp": b'.version="1.3.1+rev.2",',
            "plugins/gem-bag-ui/README.md": b"# Gem Bag UI\nMatching client and host.\n",
            "plugins/day-night-cycle/src/plugin.cpp": b'.version="1.3.1+rev.1",',
            "plugins/day-night-cycle/README.md": b"# Day Night Cycle\nClient lighting.\n",
            "plugins/day-night-cycle/day-night-cycle.toml": b"[day-night-cycle]\nenabled = true\n",
            "plugins/ultrawide/src/plugin.cpp": b'.version="1.3.5+rev.2",',
            "plugins/ultrawide/README.md": b"# Ultrawide\n21:9 and 32:9 gameplay.\n",
            "plugins/ultrawide/ultrawide.toml": b"[ultrawide]\nenabled = true\n",
            "plugins/pyre-lootfilter/src/plugin.cpp": b'inline constexpr char PluginVersion[] = "1.3.1+rev.2";',
            "plugins/pyre-lootfilter/README.md": b"# Pyre Loot Filter\nNative loot filter.\n",
            "plugins/pyre-lootfilter/pyre-lootfilter.toml": b"[pyre-lootfilter]\nenabled = true\n",
            "plugins/pyre-lootfilter/assets/reimagined/data/global/excel/sounds.txt": b"sound\tindex\nFilter01\t9999\n",
            "plugins/pyre-stashsearch/src/plugin.cpp": b'.version = "1.3.1+rev.1",',
            "plugins/pyre-stashsearch/README.md": b"# Pyre Stash Search\nCross-page stash search.\n",
            "plugins/pyre-stashsearch/pyre-stashsearch.toml": b"[pyre-stashsearch]\ntab_flyout_expanded = false\n",
            "plugins/running-defense/src/plugin.cpp": b'.version = "1.3.1+rev.1",',
            "plugins/running-defense/README.md": b"# Running Defense\nConfigurable running Defense penalty.\n",
            "plugins/running-defense/running-defense.toml": b"[running-defense]\nenabled = true\n",
            "plugins/always-show-sockets/src/plugin.cpp": b'.version="1.3.1+rev.2",',
            "plugins/always-show-sockets/always-show-sockets.toml": b"# Reference sockets config\n",
            "plugins/always-show-sockets/README.md": b"# Sockets\n",
            "plugins/always-show-sockets/CONFIGURATION-GUIDE.md": b"# Configuration\n",
            "plugins/always-show-sockets/RUNTIME-RELEASE-NOTES.md": b"# Changelog\n",
            "plugins/controller-qol/src/plugin_main.cpp": b'.version="1.3.1+rev.95",',
            "plugins/item-roll-ranges/src/plugin.cpp": b'"item-roll-ranges", "Item Roll Ranges", "1.3.1+rev.56",',
            "plugins/item-roll-ranges/item-roll-ranges.toml": b"[display]\nalways_show_ranges = false\n",
            "plugins/map-assistance/src/plugin.cpp": b'.version="1.3.1+rev.1",',
            "plugins/inventory-deposit/src/plugin_main.cpp": b'.version="1.3.1+rev.13",',
            "plugins/inventory-deposit/inventory-deposit.toml": b"[inventory_deposit]\nenabled = true\n",
            "plugins/inventory-deposit/README.md": b"# Inventory Deposit\nDeposit all inventory items.\n",
            "plugins/inventory-deposit/RUNTIME-RELEASE-NOTES.md": b"# Changelog\nDeposit button.\n",
        }
        for i in range(14):
            fixtures[f"plugins/ultrawide/assets/tc22sh/shader_{i:02d}.bin"] = f"shader-{i}".encode()
        for i in range(1, 17):
            fixtures[f"plugins/pyre-lootfilter/assets/reimagined/data/hd/global/sfx/item/Filter{i:02d}.flac"] = f"flac-{i}".encode()
        for name, data in fixtures.items():
            path = self.repo / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.arguments = ["package_runtime.py", "--controller-dll", str(self.root / "built/Controller QOL Updates.dll")]
        for argument, filename in (
            ("--ranges-dll", "Item Roll Ranges.dll"),
            ("--map-assistance-dll", "Map Assistance.dll"),
            ("--inventory-deposit-dll", "Inventory Deposit.dll"),
            ("--inventory-deposit-artifact-verifier", "inventory-deposit-artifact-tests.exe"),
            ("--floating-damage-dll", "Floating Damage.dll"),
            ("--floating-damage-artifact-verifier", "floating-damage-artifact-tests.exe"),
            ("--always-show-sockets-dll", "Always Show Sockets.dll"),
            ("--always-show-sockets-artifact-verifier", "socket-artifact-tests.exe"),
            ("--gem-bag-dll", "Gem Bag UI.dll"),
            ("--gem-bag-artifact-verifier", "gem-bag-artifact-tests.exe"),
            ("--day-night-dll", "Day Night Cycle.dll"),
            ("--day-night-artifact-verifier", "day-night-artifact-tests.exe"),
            ("--ultrawide-dll", "Ultrawide.dll"),
            ("--ultrawide-artifact-verifier", "ultrawide-artifact-tests.exe"),
            ("--pyre-lootfilter-dll", "pyre-lootfilter.dll"),
            ("--pyre-lootfilter-artifact-verifier", "pyre-lootfilter-artifact-tests.exe"),
            ("--pyre-stashsearch-dll", "pyre-stashsearch.dll"),
            ("--pyre-stashsearch-artifact-verifier", "pyre-stashsearch-artifact-tests.exe"),
            ("--running-defense-dll", "Running Defense.dll"),
            ("--running-defense-artifact-verifier", "running-defense-artifact-tests.exe"),
        ):
            path = self.root / "built" / filename
            if not path.exists():
                path.write_bytes(b"mock verified runtime fixture")
            self.arguments.extend([argument, str(path)])
        socket_pe = bytearray(128)
        socket_pe[:2] = b"MZ"
        socket_pe[60:64] = (64).to_bytes(4, "little")
        socket_pe[64:68] = b"PE\0\0"
        (self.root / "built/Always Show Sockets.dll").write_bytes(socket_pe)
        (self.root / "built/Controller QOL Updates.dll").write_bytes(b"controller runtime fixture")
        self.arguments.extend(["--output", str(self.output)])

    def run_suite(self, arguments=None):
        original_require = package_runtime.require_file

        def fixture_file(path):
            if path.is_file() or not path.is_relative_to(self.repo):
                return original_require(path)
            return b"# Public reference document fixture\n"

        with patch("package_runtime.REPOSITORY", self.repo), \
                patch("package_runtime.require_file", side_effect=fixture_file), \
                patch("sys.argv", self.arguments if arguments is None else arguments), \
                redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            package_runtime.main()

    def test_full_suite_has_exactly_ten_lean_runtime_zips_and_manifest(self):
        self.run_suite()
        expected_dlls = {
            "Controller QOL Updates.dll", "Item Roll Ranges.dll", "Map Assistance.dll",
            "Inventory Deposit.dll", "Floating Damage.dll", "Always Show Sockets.dll",
            "Gem Bag UI.dll", "Day Night Cycle.dll", "Ultrawide.dll", "pyre-lootfilter.dll",
            "pyre-stashsearch.dll", "Running Defense.dll",
        }
        archives = set(self.output.glob("*.zip"))
        self.assertEqual(len(archives), 12)
        manifest = dict(line.split("  ", 1)[::-1]
                        for line in (self.output / "SHA256SUMS").read_text().splitlines())
        self.assertEqual(set(manifest), {path.name for path in archives})
        install = self.root / "install"
        actual_dlls = set()
        for archive in archives:
            self.assertEqual(manifest[archive.name], hashlib.sha256(archive.read_bytes()).hexdigest())
            with zipfile.ZipFile(archive) as zipped:
                dlls = [name for name in zipped.namelist() if name.lower().endswith(".dll")]
                self.assertEqual(len(dlls), 1)
                self.assertTrue(dlls[0].startswith("d2rloader/plugins/"))
                actual_dlls.add(Path(dlls[0]).name)
                self.assertNotIn("README.md", zipped.namelist())
                self.assertNotIn("SHA256SUMS", zipped.namelist())
                self.assertFalse(any(name.startswith("defaults/") or name.endswith(".toml")
                                     for name in zipped.namelist()))
                if Path(dlls[0]).name == "Ultrawide.dll":
                    shaders = [n for n in zipped.namelist() if n.startswith("d2rloader/data/hd/tc22sh/")]
                    self.assertEqual(len(shaders), 14)
                    self.assertEqual(len(zipped.namelist()), 15)
                elif Path(dlls[0]).name == "pyre-lootfilter.dll":
                    flacs = [n for n in zipped.namelist() if n.startswith("Reimagined.mpq/data/hd/global/sfx/item/")]
                    self.assertEqual(len(flacs), 16)
                    self.assertIn("Reimagined.mpq/data/global/excel/sounds.txt", zipped.namelist())
                    self.assertEqual(len(zipped.namelist()), 18)
                else:
                    self.assertEqual(zipped.namelist(), [dlls[0]])
                zipped.extractall(install)
        self.assertEqual(actual_dlls, expected_dlls)
        self.assertFalse((install / "defaults").exists())
        self.assertFalse((install / "README.md").exists())
        self.assertEqual(self.run.call_count, 9)

    def test_controller_only_prerelease_keeps_single_plugin_contract(self):
        arguments = self.arguments[:3] + ["--controller-only", "--output", str(self.output)]
        self.run_suite(arguments)
        archives = list(self.output.glob("*.zip"))
        self.assertEqual([path.name for path in archives], ["Controller-QOL-Updates-1.3.1+rev.95-PyreFly.zip"])
        self.assertEqual((self.output / "SHA256SUMS").read_text().count("\n"), 1)
        self.run.assert_not_called()

    def test_incomplete_suite_is_rejected_before_writing_output(self):
        with self.assertRaises(SystemExit):
            self.run_suite(self.arguments[:3] + ["--output", str(self.output)])
        self.assertFalse(self.output.exists())

    def test_unrelated_output_archive_blocks_suite_publication(self):
        self.output.mkdir()
        (self.output / "old-unrelated.zip").write_bytes(b"old release fixture")
        with self.assertRaisesRegex(SystemExit, "unrelated ZIPs"):
            self.run_suite()


if __name__ == "__main__":
    unittest.main()
