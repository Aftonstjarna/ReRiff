"""Storage/HTTP adapter for the native exact-source song identity registry."""
import ctypes
import json
import os
from pathlib import Path


def install(server):
    from flask import jsonify, request
    library = Path(os.environ.get('RERIFF_CATALOG_LIBRARY') or Path(__file__).with_name(
        'reriff-catalog.dll' if os.name == 'nt' else 'libreriff-catalog.so'))
    native = None
    if library.is_file():
        native = ctypes.CDLL(str(library)).reriff_catalog
        native.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t]
        native.restype = ctypes.c_size_t

    def registry_path():
        return Path(server['DATA_DIR'])/'song-catalog.json'

    def load():
        path = registry_path()
        return json.loads(path.read_text(encoding='utf-8')) if path.is_file() else {'records': {}, 'aliases': {}}

    def aliases():
        return load()['aliases']

    server['catalog_aliases'] = aliases

    @server['app'].route('/reriff/songs/register', methods=['POST'])
    def register_songs():
        request.max_content_length = 8*1024*1024
        if native is None:
            return jsonify({'error': 'Native song catalog is not installed'}), 503
        if request.content_length is not None and request.content_length > 8*1024*1024:
            return jsonify({'error': 'Song manifest too large'}), 413
        try:
            body = request.get_json()
            if not isinstance(body, dict) or not isinstance(body.get('songs'), list):
                return jsonify({'error': 'Expected a songs array'}), 400
            with server['STORAGE_LOCK']:
                encoded = json.dumps({'records': load()['records'], 'songs': body['songs']}).encode('utf-8')
                size = native(encoded, None, 0)
                if size > 32*1024*1024:
                    raise ValueError('Song registry too large')
                buffer = ctypes.create_string_buffer(size)
                native(encoded, buffer, size)
                result = json.loads(buffer.value)
                if 'error' in result:
                    return jsonify(result), 409
                path = registry_path()
                temporary = path.with_suffix('.tmp')
                temporary.write_text(json.dumps(result, ensure_ascii=False), encoding='utf-8')
                os.replace(temporary, path)
            return jsonify({'registered': len(body['songs']), 'aliases': result['aliases']})
        except (ValueError, KeyError, TypeError) as error:
            return jsonify({'error': str(error)}), 400
