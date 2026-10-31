#!/usr/bin/env python3
"""Add native News policy to an existing local backend without changing song sync."""
import argparse
import json
from pathlib import Path
import re
from setup_client import ROOT, GAME_HASH, atomic, backup_directory, sha, transact


def extend_server(text):
    if 'CANNED_CLASSES = ("Configuration", "News", "Version")' in text:
        text = text.replace('CANNED_CLASSES = ("Configuration", "News", "Version")',
                            'CANNED_CLASSES = ("Configuration", "Version")', 1)
    elif 'CANNED_CLASSES = ("Configuration", "Version")' not in text:
        raise ValueError('Unrecognized canned-class configuration; backend unchanged.')
    start = text.index('def query_class(cls):')
    end = text.index('\n@app.route(', start)
    query = text[start:end]
    if 'objs = news_response(objs)' not in query:
        marker = '    with STORAGE_LOCK:\n        objs = load_all_objects(cls)\n'
        if query.count(marker) != 1:
            raise ValueError('Unrecognized News query structure; backend unchanged.')
        query = query.replace(marker, marker+'\n    if cls == "News":\n        objs = news_response(objs)\n', 1)
        text = text[:start]+query+text[end:]
    if '_news_module.install(globals())' not in text:
        marker = 'if __name__ == "__main__":'
        if text.count(marker) != 1 or 'import importlib.util' not in text:
            raise ValueError('Unrecognized backend module startup; backend unchanged.')
        setup = '''_news_spec = importlib.util.spec_from_file_location(
    'reriff_news_adapter', os.path.join(BASE_DIR, 'news_adapter.py'))
_news_module = importlib.util.module_from_spec(_news_spec)
_news_spec.loader.exec_module(_news_module)
_news_module.install(globals())

'''
        text = text.replace(marker, setup+marker, 1)
    return text


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-dir', type=Path, required=True)
    p.add_argument('--platform', choices=('linux', 'windows'), default='linux')
    p.add_argument('--dry-run', action='store_true')
    p.add_argument('--restore', action='store_true')
    args = p.parse_args()
    game = args.game_dir.resolve()
    if sha((game/'Game.exe').read_bytes()) != GAME_HASH:
        raise ValueError('Unsupported Game.exe; backend unchanged.')
    backup_dir = backup_directory(game, migrate=not args.dry_run)
    record_path = backup_dir/('news-'+sha(str(game).encode())+'.json')
    if args.restore:
        record = json.loads(record_path.read_text())
        before = {}
        for name, expected in record['installed'].items():
            if sha((game/name).read_bytes()) != expected:
                raise ValueError('Backend file changed since trial; refusing to overwrite: '+name)
            old = record['before'][name]
            before[name] = (backup_dir/(old+'.bin')).read_bytes() if old else None
            if old and sha(before[name]) != old:
                raise ValueError('Backend backup verification failed.')
        if args.dry_run:
            print('Backend restore validated.'); return
        for name, data in before.items():
            if data is None:
                (game/name).unlink()
            else:
                atomic(game/name, data)
        record_path.unlink()
        print('Backend restored; restart it to load the restored code.'); return
    if record_path.exists():
        raise ValueError('A News backend trial is already recorded; restore before reinstalling.')
    base = Path('localserver/server')
    code = game/base/'riffapi-local.py'
    changed = extend_server(code.read_text())
    library = 'libreriff-news.so' if args.platform == 'linux' else 'reriff-news.dll'
    plan = {str(base/'riffapi-local.py'): changed.encode(),
            str(base/'news_adapter.py'): (ROOT/'vendor/riffapi-local-extended/news_adapter.py').read_bytes(),
            str(base/library): (ROOT/'build/server-sync'/library).read_bytes()}
    if not (game/base/'VERSION').exists():
        install_state = game/'localserver/install-state.json'
        version = json.loads(install_state.read_text())['version'] if install_state.exists() else (ROOT/'VERSION').read_text().strip()
        if not re.fullmatch(r'[0-9]{1,4}\.[0-9]{1,4}\.[0-9]{1,4}', version):
            raise ValueError('Invalid installed server version; backend unchanged.')
        plan[str(base/'VERSION')] = (version+'\n').encode()
    if args.dry_run:
        print('Validated News extension;', len(plan), 'files prepared; existing song library/data untouched.'); return
    transact(game, plan, backup_dir, record_path)
    print('News extension installed with backups. Restart this backend to load it; existing song library/data unchanged.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
