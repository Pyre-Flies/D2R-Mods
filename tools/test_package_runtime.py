"""Regression tests for extraction-safe six-plugin runtime releases."""
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
                     "defaults\\config.toml", "elsewhere/plugin.dll", "elsewhere/PLUGIN.DLL",
                     "./d2rloader/config/catalog.json", "defaults//config.toml", "SHA256SUMS", ""):
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

    def test_archive_checks_all_curated_text_not_only_readme(self):
        with tempfile.TemporaryDirectory() as temp:
            for name in ("README.md", "CHANGELOG.md", "COMPATIBILITY.txt", "defaults/example.toml"):
                with self.subTest(name=name), self.assertRaisesRegex(SystemExit, "Internal tooling"):
                    make_archive(Path(temp) / "bad.zip", {name: b"Internal Codex instructions"})


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
            "plugins/floating-damage/DISTRIBUTION-README.md": b"# Floating Damage\nInstall with the loader.\n",
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
            "d2rloader/plugins/Floating Damage.dll", "defaults/floating-damage.toml",
            "README.md", "CHANGELOG.md", "COMPATIBILITY.txt", "LICENSE",
            "licenses/D2RLoader-PluginSDK-LICENSE",
        }
        original = archive.read_bytes()
        with zipfile.ZipFile(archive) as zipped:
            self.assertEqual(set(zipped.namelist()), expected | {"SHA256SUMS"})
            self.assertEqual(zipped.read("d2rloader/plugins/Floating Damage.dll"),
                             self.dll.read_bytes())
            sums = dict(line.split("  ", 1)[::-1]
                        for line in zipped.read("SHA256SUMS").decode().splitlines())
            self.assertEqual(set(sums), expected)
            for name, digest in sums.items():
                self.assertEqual(digest, hashlib.sha256(zipped.read(name)).hexdigest())
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
        (self.plugin / "DISTRIBUTION-README.md").unlink()
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
            (self.plugin / "DISTRIBUTION-README.md", b"C:/Users/private/build"),
            (self.plugin / "COMPATIBILITY.txt", b"D:\\work\\private\\capture"),
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


class SixPluginSuiteTests(FloatingDamagePackageFixture, unittest.TestCase):
    def setUp(self):
        super().setUp()
        fixtures = {
            "plugins/always-show-sockets/src/plugin.cpp": b'.version="1.3.1+rev.1",',
            "plugins/always-show-sockets/always-show-sockets.toml": b"# Reference sockets config\n",
            "plugins/always-show-sockets/DISTRIBUTION-README.md": b"# Sockets\n",
            "plugins/always-show-sockets/CONFIGURATION-GUIDE.md": b"# Configuration\n",
            "plugins/always-show-sockets/RUNTIME-RELEASE-NOTES.md": b"# Changelog\n",
            "plugins/controller-qol/src/plugin_main.cpp": b'.version="1.3.1-rev.75",',
            "plugins/item-roll-ranges/src/plugin.cpp": b'"item-roll-ranges", "Item Roll Ranges", "0.4.0-rev.23",',
            "plugins/item-roll-ranges/item-roll-ranges.toml": b"[display]\nalways_show_ranges = false\n",
            "plugins/map-assistance/src/plugin.cpp": b'.version="0.1.0-rev.1",',
            "plugins/inventory-deposit/src/plugin_main.cpp": b'.version="0.1.0-beta.13",',
            "plugins/inventory-deposit/inventory-deposit.toml": b"[inventory_deposit]\nenabled = true\n",
            "plugins/inventory-deposit/DISTRIBUTION-README.md": b"# Inventory Deposit\nDeposit all inventory items.\n",
            "plugins/inventory-deposit/RUNTIME-RELEASE-NOTES.md": b"# Changelog\nDeposit button.\n",
        }
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
            # The suite's older plugins have many public documentation files.
            # Use inert text fixtures while exercising real ZIP construction.
            if path.is_file() or not path.is_relative_to(self.repo):
                return original_require(path)
            return b"# Public reference document fixture\n"

        with patch("package_runtime.REPOSITORY", self.repo), \
                patch("package_runtime.require_file", side_effect=fixture_file), \
                patch("sys.argv", self.arguments if arguments is None else arguments), \
                redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            package_runtime.main()

    def test_full_suite_has_exactly_six_runtime_zips_and_manifest(self):
        self.run_suite()
        expected_dlls = {
            "Controller QOL Updates.dll", "Item Roll Ranges.dll", "Map Assistance.dll",
            "Inventory Deposit.dll", "Floating Damage.dll", "Always Show Sockets.dll",
        }
        archives = set(self.output.glob("*.zip"))
        self.assertEqual(len(archives), 6)
        manifest = dict(line.split("  ", 1)[::-1]
                        for line in (self.output / "SHA256SUMS").read_text().splitlines())
        self.assertEqual(set(manifest), {path.name for path in archives})
        install = self.root / "install"
        customized = {}
        for config in ("controller-qol-updates.toml", "controller-qol-skills.toml",
                       "item-roll-ranges.toml", "map-assistance.toml", "inventory-deposit.toml", "floating-damage.toml"):
            path = install / "d2rloader/config" / config
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"# customized setting kept\r\nvalue = 99\r\n")
            customized[path] = path.read_bytes()
        actual_dlls = set()
        for archive in archives:
            self.assertEqual(manifest[archive.name], hashlib.sha256(archive.read_bytes()).hexdigest())
            with zipfile.ZipFile(archive) as zipped:
                dlls = [name for name in zipped.namelist() if name.lower().endswith(".dll")]
                self.assertEqual(len(dlls), 1)
                self.assertFalse(any(name.startswith("docs/") and name != "docs/SKILL-DISCOVERY-CONFIG.md"
                                     for name in zipped.namelist()))
                self.assertTrue(dlls[0].startswith("d2rloader/plugins/"))
                actual_dlls.add(Path(dlls[0]).name)
                if Path(dlls[0]).name == "Item Roll Ranges.dll":
                    self.assertNotIn("docs/ITEM-LEVELS-REV45.md", zipped.namelist())
                    self.assertIn("README.md", zipped.namelist())
                    self.assertIn(b"always_show_ranges = false", zipped.read("defaults/item-roll-ranges.toml"))
                self.assertFalse(any(name.lower().startswith("d2rloader/config/")
                                     for name in zipped.namelist()))
                for line in zipped.read("SHA256SUMS").decode().splitlines():
                    digest, name = line.split("  ", 1)
                    self.assertEqual(digest, hashlib.sha256(zipped.read(name)).hexdigest())
                zipped.extractall(install)
                zipped.extractall(self.root / "fresh")
        self.assertEqual(actual_dlls, expected_dlls)
        for path, data in customized.items():
            self.assertEqual(path.read_bytes(), data)
        self.assertFalse((self.root / "fresh/d2rloader/config").exists())
        self.assertEqual(self.run.call_count, 3)
        verified = {Path(call.args[0][1]).name for call in self.run.call_args_list}
        self.assertEqual(verified, {"Inventory Deposit.dll", "Floating Damage.dll", "Always Show Sockets.dll"})

    def test_controller_only_prerelease_keeps_single_plugin_contract(self):
        arguments = self.arguments[:3] + ["--controller-only", "--output", str(self.output)]
        self.run_suite(arguments)
        archives = list(self.output.glob("*.zip"))
        self.assertEqual([path.name for path in archives], ["Controller-QOL-Updates-1.3.1-rev.75-PyreFly.zip"])
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
