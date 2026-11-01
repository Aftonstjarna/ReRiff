"""Exercise the actual launcher entry point with Steam-style forwarded arguments."""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--wine', required=True)
    parser.add_argument('--prefix', required=True)
    parser.add_argument('--launcher', type=Path, default=ROOT/'build/ReRiff-Launcher.exe')
    parser.add_argument('--game-fixture', type=Path, default=ROOT/'build/experimental-engine-menu/fake-game.exe')
    args = parser.parse_args()
    host = ['/usr/bin/distrobox-host-exec'] if Path('/usr/bin/distrobox-host-exec').exists() else []
    with tempfile.TemporaryDirectory(prefix='reriff windows steam ') as temporary:
        game = Path(temporary)
        launcher = game/'ReRiff-Launcher.exe'
        shutil.copy2(args.launcher, launcher)
        shutil.copy2(args.game_fixture, game/'Game.exe')
        settings = b'[server]\nexternal_url=\nfallback_url=\nnetworking_enabled=false\n'
        (game/'server.ini').write_bytes(settings)
        forwarded = 'Z:'+str(game/'Game.exe').replace('/', '\\')
        def run(options, expected):
            result = subprocess.run(host+['timeout','45','xvfb-run','-a','env',
                'WINEPREFIX='+args.prefix,'WINEDEBUG=-all','WINEDLLOVERRIDES=mscoree,mshtml=',
                args.wine,str(launcher),*options], capture_output=True, text=True)
            if result.returncode != expected:
                raise AssertionError((options, result.returncode, result.stdout, result.stderr))
            assert (game/'server.ini').read_bytes() == settings
        run(['--self-test', forwarded], 0)
        for flag in ('--skip', '--skip-launcher', '--launcher-skip'):
            marker = game/'Game.exe.played'
            marker.unlink(missing_ok=True)
            run([flag, forwarded, '--windowed', 'value with spaces'], 0)
            assert marker.read_bytes() == b'play invoked'
        settings += b'\n[launcher]\nskip_launcher=true\n'
        (game/'server.ini').write_bytes(settings)
        marker.unlink()
        run([forwarded], 0)
        assert marker.read_bytes() == b'play invoked'
        run(['--skip', 'Z:\\another-installation\\Game.exe'], 2)
        run(['%command%'], 2)
        print('PASS: Steam-forwarded entry point, both Play aliases, invalid paths, and unchanged settings')


if __name__ == '__main__':
    main()
