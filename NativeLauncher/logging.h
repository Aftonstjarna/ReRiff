#ifndef RERIFF_LOGGING_H
#define RERIFF_LOGGING_H
#include "config.h"
static inline BOOL rr_logging_enabled(const WCHAR *game,BOOL fallback) {
    WCHAR file[32768],value[32];
    _snwprintf(file,32768,L"%ls\\server.ini",game);file[32767]=0;
    GetPrivateProfileStringW(L"diagnostics",L"logging_enabled",L"",value,32,file);
    if(*value)return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    _snwprintf(file,32768,L"%ls\\localserver\\client-channel.ini",game);file[32767]=0;
    GetPrivateProfileStringW(L"client",L"channel",L"",value,32,file);
    return *value?!_wcsicmp(value,L"experimental"):fallback;
}
static inline BOOL rr_log_selected(const WCHAR *game,const WCHAR *category) {
    WCHAR file[32768],key[80],value[32];
    _snwprintf(file,32768,L"%ls\\server.ini",game);file[32767]=0;
    _snwprintf(key,80,L"log_%ls",category);key[79]=0;
    GetPrivateProfileStringW(L"diagnostics",key,L"true",value,32,file);
    return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
}
static inline BOOL rr_log_enabled(const WCHAR *game,const WCHAR *category,BOOL fallback) {
    return rr_logging_enabled(game,fallback)&&rr_log_selected(game,category);
}
static inline BOOL rr_log_ini(const WCHAR *ini_path,const WCHAR *category) {
    WCHAR root[32768];wcsncpy(root,ini_path,32767);root[32767]=0;WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return FALSE;*slash=0;
    return rr_log_enabled(root,category,FALSE);
}
static inline BOOL rr_logging_ini(const WCHAR *ini_path) {
    WCHAR root[32768];wcsncpy(root,ini_path,32767);root[32767]=0;WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return FALSE;*slash=0;
    return rr_logging_enabled(root,FALSE);
}
#endif
