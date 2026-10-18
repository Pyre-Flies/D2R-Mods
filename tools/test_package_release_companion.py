import hashlib
import io
import unittest
import zipfile
from release_companion import verify


def digest(data):
    return hashlib.sha256(data).hexdigest()


def fixture(extra=None, corrupt_dll=False):
    files = {
        "d2rloader/plugins/unhoarder.dll": b"reviewed DLL",
        "Reimagined.mpq/data/global/excel/sounds.txt": b"reviewed table",
        "Reimagined.mpq/data/global/excel_launcher_clean/sounds.txt": b"reviewed table",
        "reference/filter.json": b"// reference only",
    }
    if extra:
        files.update(extra)
    files["SHA256SUMS"] = "".join(digest(v) + "  " + k + "\n" for k, v in files.items()).encode()
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w") as archive:
        for name, data in files.items():
            archive.writestr(name, data)
    data = stream.getvalue()
    expected = {"sha256": digest(data), "dllSha256": digest(b"other DLL" if corrupt_dll else b"reviewed DLL"), "soundsSha256": digest(b"reviewed table")}
    return data, expected


class CompanionTests(unittest.TestCase):
    def test_exact_artifact_and_both_tables(self):
        self.assertEqual(verify(*fixture()), b"reviewed DLL")

    def test_changed_archive_refused(self):
        data, expected = fixture()
        with self.assertRaises(ValueError):
            verify(data + b"changed", expected)

    def test_changed_dll_refused_even_with_known_archive(self):
        with self.assertRaises(ValueError):
            verify(*fixture(corrupt_dll=True))

    def test_config_and_traversal_refused(self):
        for path in ("d2rloader/config/filter.json", "defaults/settings.toml", "../outside.txt", "C:/outside.txt", "d2rloader/plugins/other.dll"):
            with self.subTest(path=path), self.assertRaises(ValueError):
                verify(*fixture({path: b"must not install"}))

    def test_wrong_clean_source_refused(self):
        with self.assertRaises(ValueError):
            verify(*fixture({"Reimagined.mpq/data/global/excel_launcher_clean/sounds.txt": b"wrong routing"}))


if __name__ == "__main__":
    unittest.main()
