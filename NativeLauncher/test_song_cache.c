#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#define CAP 32768
#pragma GCC diagnostic ignored "-Wunused-function"
static void log_message(const WCHAR *message) { wprintf(L"%ls\n", message); }
#include "song_sync.h"

static int contains(const unsigned char *data,DWORD size,const char *needle){size_t n=strlen(needle);if(n==0||n>size)return 0;for(DWORD i=0;i+n<=size;i++)if(!memcmp(data+i,needle,n))return 1;return 0;}
static DWORD record(unsigned char *out, const char *name, const char *id) {
    const char *fields[7] = {name, id, "Title", "Artist", "", "genre", "Album"};
    DWORD used = 0;
    for (int i = 0; i < 7; i++) {
        size_t length = strlen(fields[i]);
        memcpy(out + used, fields[i], length + 1);
        used += (DWORD)length + 1;
    }
    memset(out + used, 0, 29);
    return used + 29;
}

int main(void) {
    unsigned char cache[1024];
    DWORD used = 0;
    DWORD header[4] = {1128350512u, 19u, 2u, 18u};
    memcpy(cache, header, 16);
    used = 16;
    used += record(cache + used, "audio\\music\\bundled\\fixture.mp3", "bundled1");
    used += record(cache + used, "hst:C:\\music\\fixture.flac", "local1");

    char *filtered = NULL;
    DWORD filtered_size = 0;
    if (!song_cache_bundled((const char *)cache, used, &filtered, &filtered_size)) return 1;
    DWORD count = 0;
    memcpy(&count, filtered + 8, 4);
    if (count != 1 || filtered_size >= used) { free(filtered); return 2; }
    if (contains((const unsigned char *)filtered, filtered_size, "hst:")) { free(filtered); return 3; }
    if (!contains((const unsigned char *)filtered, filtered_size, "audio\\music\\bundled\\fixture.mp3")) { free(filtered); return 4; }
    free(filtered);

    WCHAR key[160];
    song_cache_key_for(L"89acfcfd-f368-4f78-8061-962ebaf6d336", key, 160);
    if (wcscmp(key, L"server-89acfcfd-f368-4f78-8061-962ebaf6d336")) return 5;
    song_cache_key_for(L"../unsafe", key, 160);
    if (wcsstr(key, L"..") || wcsstr(key, L"unsafe")) return 6;

    WCHAR cache_path[CAP], cache_sidecar[CAP], sp_sidecar[CAP];
    if (!song_cache_paths(L"server-probe", cache_path, cache_sidecar)) return 7;
    if (!song_cache_paths(L"singleplayer", cache_path, sp_sidecar)) return 7;
    DeleteFileW(cache_path);
    DeleteFileW(sp_sidecar);
    DeleteFileW(cache_sidecar);
    WCHAR other[CAP], other_sidecar[CAP];
    if (!song_cache_paths(L"server-other", other, other_sidecar)) return 7;
    DeleteFileW(other_sidecar);
    if (!song_cache_write(cache_path, (const char *)cache, used)) return 8;
    if (!song_cache_prepare(L"singleplayer")) return 9;
    if (GetFileAttributesW(sp_sidecar) == INVALID_FILE_ATTRIBUTES) return 10;
    if (!song_cache_prepare(L"server-probe")) return 11;
    char *probe_data = NULL; DWORD probe_size = 0;
    if (!song_cache_read(cache_path, &probe_data, &probe_size)) return 12;
    if (contains((const unsigned char *)probe_data, probe_size, "hst:")) { free(probe_data); return 13; }
    free(probe_data);
    if (!song_cache_commit(L"server-probe")) return 14;
    if (GetFileAttributesW(cache_sidecar) == INVALID_FILE_ATTRIBUTES) return 15;
    if (!song_cache_read(cache_sidecar, &probe_data, &probe_size) || contains((const unsigned char *)probe_data, probe_size, "hst:")) { free(probe_data); return 16; }
    free(probe_data);
    if (!song_cache_prepare(L"server-other")) return 17;
    if (!song_cache_read(cache_path, &probe_data, &probe_size) || contains((const unsigned char *)probe_data, probe_size, "hst:")) { free(probe_data); return 18; }
    free(probe_data);
    puts("PASS per-server cache keys, bundled-only multiplayer bootstrap, migration and sidecar commit");
    return 0;
}
