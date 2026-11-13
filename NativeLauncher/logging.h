#ifndef RERIFF_LOGGING_H
#define RERIFF_LOGGING_H
static inline BOOL rr_logging_enabled(const WCHAR *game,BOOL fallback) {
    WCHAR file[32768],value[32];
    _snwprintf(file,32768,L"%ls\\server.ini",game);file[32767]=0;
    GetPrivateProfileStringW(L"diagnostics",L"logging_enabled",L"",value,32,file);
    if(*value)return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    _snwprintf(file,32768,L"%ls\\localserver\\client-channel.ini",game);file[32767]=0;
    GetPrivateProfileStringW(L"client",L"channel",L"",value,32,file);
    return *value?!_wcsicmp(value,L"experimental"):fallback;
}
static inline BOOL rr_logging_ini(const WCHAR *ini_path) {
    WCHAR root[32768];wcsncpy(root,ini_path,32767);root[32767]=0;WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return FALSE;*slash=0;
    return rr_logging_enabled(root,FALSE);
}
#endif
