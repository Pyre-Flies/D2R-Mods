import hashlib
from pathlib import Path
import tempfile
import unittest
import zipfile
from package_always_show_sockets import NAMES,privacy,verify,validate_version
class PackageTests(unittest.TestCase):
    def archive(self,extra=None,corrupt=False):
        directory=tempfile.TemporaryDirectory();self.addCleanup(directory.cleanup)
        path=Path(directory.name)/'test.zip'
        files={}
        data=bytearray(128);data[:2]=b'MZ';data[60:64]=(64).to_bytes(4,'little');data[64:68]=b'PE\0\0'
        if corrupt:data[120]=0xFF
        files['d2rloader/plugins/Always Show Sockets.dll']=bytes(data)
        if extra:files[extra]=b'unexpected'
        with zipfile.ZipFile(path,'w') as z:
            for n,v in files.items():z.writestr(n,v)
        return path
    def test_valid(self):verify(self.archive())
    def test_extra_config(self):
        with self.assertRaises(ValueError):verify(self.archive('d2rloader/config/custom.toml'))
    def test_lean_dll_only_layout(self):
        with zipfile.ZipFile(self.archive()) as z:
            self.assertEqual(z.namelist(),['d2rloader/plugins/Always Show Sockets.dll'])
            self.assertNotIn('defaults/always-show-sockets.toml',z.namelist())
            self.assertNotIn('README.md',z.namelist())
            self.assertNotIn('SHA256SUMS',z.namelist())
    def test_traversal(self):
        with self.assertRaises(ValueError):verify(self.archive('../escape'))
    def test_checksum(self):
        reference=self.archive()
        digest=hashlib.sha256(reference.read_bytes()).hexdigest()
        verify(reference,digest)
        with self.assertRaises(ValueError):verify(self.archive(corrupt=True),digest)
    def test_machine_paths(self):
        for data in (b'C:\\Users\\private\\file', '/home/private/file'.encode(), 'I:\\private\\file'.encode('utf-16-le')):
            with self.assertRaises(ValueError):privacy('README.md',data)
    def test_release_and_beta_version_channels(self):
        validate_version('1.3.1+rev.1')
        validate_version('1.3.1+rev.1','always-show-sockets-1.3.1+rev.1')
        validate_version('0.1.0-beta.15','beta-always-show-sockets-0.1.0-beta.15')
    def test_bad_versions_and_mistagged_channels(self):
        for version in ('1.3.1','1.3.1+rev','1.3.1+rev.x','0.1.0-beta.15oops'):
            with self.assertRaises(ValueError):validate_version(version)
        for version,tag in (('1.3.1+rev.1','beta-always-show-sockets-1.3.1+rev.1'),('0.1.0-beta.15','always-show-sockets-1.3.1+rev.1'),('1.3.1+rev.1','always-show-sockets-1.3.1+rev.2')):
            with self.assertRaises(ValueError):validate_version(version,tag)
if __name__=='__main__':unittest.main()
