import hashlib
from pathlib import Path
import tempfile
import unittest
import zipfile
from package_always_show_sockets import NAMES,privacy,verify
class PackageTests(unittest.TestCase):
    def archive(self,extra=None,corrupt=False):
        directory=tempfile.TemporaryDirectory();self.addCleanup(directory.cleanup)
        path=Path(directory.name)/'test.zip'
        files={n:b'document' for n in NAMES if not n.endswith('.dll')}
        # Minimal PE fixture with no data sections; native artifact checks run separately.
        data=bytearray(128);data[:2]=b'MZ';data[60:64]=(64).to_bytes(4,'little');data[64:68]=b'PE\0\0'
        files['d2rloader/plugins/Always Show Sockets.dll']=bytes(data)
        checks=''.join(hashlib.sha256(v).hexdigest()+'  '+n+'\n' for n,v in files.items())
        if corrupt:files['README.md']=b'changed'
        files['SHA256SUMS']=checks.encode()
        if extra:files[extra]=b'unexpected'
        with zipfile.ZipFile(path,'w') as z:
            for n,v in files.items():z.writestr(n,v)
        return path
    def test_valid(self):verify(self.archive())
    def test_extra_config(self):
        with self.assertRaises(ValueError):verify(self.archive('d2rloader/config/custom.toml'))
    def test_traversal(self):
        with self.assertRaises(ValueError):verify(self.archive('../escape'))
    def test_checksum(self):
        with self.assertRaises(ValueError):verify(self.archive(corrupt=True))
    def test_machine_paths(self):
        for data in (b'C:\\Users\\private\\file', '/home/private/file'.encode(), 'I:\\private\\file'.encode('utf-16-le')):
            with self.assertRaises(ValueError):privacy('README.md',data)
if __name__=='__main__':unittest.main()
