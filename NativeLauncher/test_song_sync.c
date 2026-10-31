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
static void log_message(const WCHAR *message) { wprintf(L"%ls\n",message); }
#include "song_sync.h"

int wmain(int argc,WCHAR **argv) {
    WCHAR appdata[MAX_PATH],directory[CAP],destination[CAP];
    if(argc!=3||FAILED(SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,appdata)))return 1;
    _snwprintf(directory,CAP,L"%ls\\Octane\\DAT",appdata);directory[CAP-1]=0;
    if(SHCreateDirectoryExW(NULL,directory,NULL)!=ERROR_SUCCESS&&GetFileAttributesW(directory)==INVALID_FILE_ATTRIBUTES)return 2;
    _snwprintf(destination,CAP,L"%ls\\cache.bin",directory);destination[CAP-1]=0;
    if(!CopyFileW(argv[1],destination,FALSE))return 3;
    sync_cached_songs(argv[2]);
    return 0;
}
