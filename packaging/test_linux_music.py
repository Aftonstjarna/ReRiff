"""Linux music linking must preserve user files and honor opt-out/dry-run."""
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import installer


class MusicLinkTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.game = self.root/'steamapps/common/Riff Racer'
        self.game.mkdir(parents=True)
        (self.game/'Game.exe').write_bytes(b'fixture')
        self.user = self.root/'steamapps/compatdata/351990/pfx/drive_c/users/steamuser'
        self.user.mkdir(parents=True)
        self.music = self.root/'My Music ü'
        self.bundle = self.root/'bundle'
        (self.bundle/'payload').mkdir(parents=True)
        (self.bundle/'manifest.json').write_text(json.dumps({'version': 'test', 'files': {}, 'patches': {}, 'required_existing': {}}))
        self.addCleanup(patch.stopall)
        patch.object(installer, 'default_music_dir', return_value=self.music).start()

    def install(self, **kwargs):
        return installer.install(self.bundle, self.game, None, 'linux', **kwargs)

    def test_link_xdg_music_and_preserve_song_bytes(self):
        self.music.mkdir()
        (self.music/'song.flac').write_bytes(b'original song bytes')
        self.install()
        link = self.user/'Music'
        self.assertTrue(link.is_symlink())
        self.assertEqual(link.resolve(), self.music)
        self.assertEqual((link/'song.flac').read_bytes(), b'original song bytes')
        before = link.lstat().st_ino
        self.install()
        self.assertEqual(link.lstat().st_ino, before)

    def test_nonempty_prefix_music_is_never_replaced(self):
        destination = self.user/'Music'
        destination.mkdir()
        (destination/'keep.flac').write_bytes(b'keep')
        self.install()
        self.assertFalse(destination.is_symlink())
        self.assertEqual((destination/'keep.flac').read_bytes(), b'keep')
        self.assertFalse(self.music.exists())

    def test_empty_prefix_folder_and_editing_existing_link(self):
        (self.user/'Music').mkdir()
        self.install()
        other = self.root/'Different Music'
        self.install(music_dir=other)
        self.assertEqual((self.user/'Music').resolve(), other)
        self.assertTrue(self.music.is_dir())
        state = json.loads((self.game/'localserver/install-state.json').read_text())
        self.assertEqual(state['music_dir'], str(other))

    def test_opt_out_survives_automatic_update(self):
        self.install(music_link=False)
        self.install()
        self.assertFalse((self.user/'Music').exists())
        self.assertFalse(self.music.exists())
        self.assertFalse(json.loads((self.game/'localserver/install-state.json').read_text())['music_link'])

    def test_dry_run_does_not_create_link_or_music_folder(self):
        self.install(dry_run=True)
        self.assertFalse((self.user/'Music').exists())
        self.assertFalse(self.music.exists())
        self.assertFalse((self.game/'localserver').exists())

    def test_missing_prefix_is_not_initialized_by_installer(self):
        game = self.root/'other-game'
        game.mkdir()
        (game/'Game.exe').write_bytes(b'fixture')
        installer.install(self.bundle, game, None, 'linux')
        self.assertFalse((game/'prefix').exists())
        self.assertFalse(self.music.exists())

    def test_failed_install_never_links_music(self):
        with patch.object(installer, 'replace', side_effect=OSError('fixture write failure')):
            with self.assertRaises(OSError):
                self.install()
        self.assertFalse((self.user/'Music').exists())
        self.assertFalse(self.music.exists())

    def test_xdg_config_is_read_as_data(self):
        # Restore this helper alone to test real parsing without touching HOME.
        patch.stopall()
        home = self.root/'home'
        config = home/'.config'
        config.mkdir(parents=True)
        (config/'user-dirs.dirs').write_text('XDG_MUSIC_DIR="$HOME/Music ü $(never execute)"\n')
        with patch.object(Path, 'home', return_value=home), patch.dict(os.environ, {'XDG_CONFIG_HOME': str(config)}):
            self.assertEqual(installer.default_music_dir(), home/'Music ü $(never execute)')


if __name__ == '__main__':
    unittest.main()
