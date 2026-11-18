"""Re:Start must ship in the core installer even without optional font packs."""
import hashlib
import shutil
import tempfile
import unittest
import zipfile
from pathlib import Path
from build_release import ROOT, bundle_core_artwork

class CoreArtworkBundleTests(unittest.TestCase):
    def test_first_launch_artwork_is_self_contained(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'images').mkdir();(root/'scripts').mkdir()
            shutil.copy2(ROOT/'images/reriff-prestige-achievement.png',root/'images/reriff-prestige-achievement.png')
            shutil.copy2(ROOT/'scripts/build-prestige-artwork.py',root/'scripts/build-prestige-artwork.py')
            common=root/'bundle';stage=root/'stage'
            component=bundle_core_artwork(root,common,stage)
            for key in ('dat','zip'):
                self.assertEqual(hashlib.sha256((common/component[key]).read_bytes()).hexdigest(),component[key+'_sha256'])
            with zipfile.ZipFile(common/component['zip']) as archive:
                self.assertEqual(archive.namelist(),['graphics/reriff/achievements/batchtextures.brres'])
                texture=archive.read(archive.namelist()[0])
                self.assertIn(b'first_prestige\0',texture)
                self.assertIn(b'DDS ',texture)
            # This fixture has no optional components or downloaded fonts.
            self.assertFalse((common/'components/ReRiff-CJK.zip').exists())
            self.assertFalse((common/'components/ReRiff-Latin.zip').exists())

if __name__=='__main__':unittest.main()
