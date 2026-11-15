#ifndef RERIFF_CONFIG_H
#define RERIFF_CONFIG_H
/* Route known client sections while retaining old installs until migrated. */
static inline const WCHAR *rr_config_path(const WCHAR *file,const WCHAR *section,WCHAR *out) {
    if(!file||!section)return file;
    const WCHAR *base=wcsrchr(file,L'\\');base=base?base+1:file;
    if(_wcsicmp(base,L"server.ini"))return file;
    const WCHAR *name=NULL;
    if(!_wcsicmp(section,L"lastfm")||!_wcsicmp(section,L"graphics")||!_wcsicmp(section,L"languages")||!_wcsicmp(section,L"launcher")||!_wcsicmp(section,L"diagnostics"))name=L"client.ini";
    if(!_wcsicmp(section,L"progression"))name=L"singleplayer.ini";
    if(!name)return file;
    size_t n=(size_t)(base-file);if(n+wcslen(name)>=32768)return file;
    wcsncpy(out,file,n);wcscpy(out+n,name);return out;
}
static inline DWORD rr_profile_string(LPCWSTR section,LPCWSTR key,LPCWSTR fallback,LPWSTR out,DWORD capacity,LPCWSTR file) {
    WCHAR routed[32768];const WCHAR *path=rr_config_path(file,section,routed);
    if(path!=file) {
        DWORD n=GetPrivateProfileStringW(section,key,L"\x1",out,capacity,path);
        if(n!=1||out[0]!=1)return n;
    }
    return GetPrivateProfileStringW(section,key,fallback,out,capacity,file);
}
static inline UINT rr_profile_int(LPCWSTR section,LPCWSTR key,INT fallback,LPCWSTR file) {
    WCHAR value[64];rr_profile_string(section,key,L"",value,64,file);
    return *value?(UINT)wcstol(value,NULL,0):(UINT)fallback;
}
static inline BOOL rr_profile_write(LPCWSTR section,LPCWSTR key,LPCWSTR value,LPCWSTR file) {
    WCHAR routed[32768];return WritePrivateProfileStringW(section,key,value,rr_config_path(file,section,routed));
}
#define GetPrivateProfileStringW rr_profile_string
#define GetPrivateProfileIntW rr_profile_int
#define WritePrivateProfileStringW rr_profile_write
#endif
