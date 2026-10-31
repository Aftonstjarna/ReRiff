"""Thin transport adapter for native News/version policy."""
import ctypes
import json
import os
from pathlib import Path


def install(server):
    from flask import request
    library = Path(os.environ.get('RERIFF_NEWS_LIBRARY') or Path(__file__).with_name(
        'reriff-news.dll' if os.name == 'nt' else 'libreriff-news.so'))
    native = None
    if library.is_file():
        native = ctypes.CDLL(str(library)).reriff_news
        native.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_size_t]
        native.restype = ctypes.c_size_t

    def news_response(entries):
        if native is None:
            return entries
        version_file = Path(server['BASE_DIR'])/'VERSION'
        if not version_file.is_file():
            version_file = Path(__file__).resolve().parents[2]/'VERSION'
        version = os.environ.get('RERIFF_SERVER_VERSION')
        if version is None:
            version = version_file.read_text().strip() if version_file.is_file() else ''
        encoded = json.dumps({'news': entries, 'server_version': version,
                              'client_version': request.headers.get('X-ReRiff-Client-Version', '')}).encode()
        size = native(encoded, None, 0)
        if size > 8*1024*1024:
            raise ValueError('News response too large')
        buffer = ctypes.create_string_buffer(size)
        native(encoded, buffer, size)
        result = json.loads(buffer.value)
        if 'error' in result:
            raise ValueError(result['error'])
        return result['news']

    server['news_response'] = news_response
