#ifndef RERIFF_GRAPHICS_SETTINGS_H
#define RERIFF_GRAPHICS_SETTINGS_H
/* -1 means no override: retain the original game's saved display mode. */
static inline int rr_fullscreen_mode(const WCHAR *file){
    WCHAR value[16];GetPrivateProfileStringW(L"graphics",L"fullscreen",L"",value,16,file);
    if(*value){if(!wcscmp(value,L"0"))return 0;if(!wcscmp(value,L"1"))return 1;if(!wcscmp(value,L"2"))return 2;return -1;}
    GetPrivateProfileStringW(L"graphics",L"borderless",L"false",value,16,file);
    if(!_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes"))return 1;
    GetPrivateProfileStringW(L"graphics",L"exclusive_fullscreen",L"",value,16,file);
    if(!*value)return -1;
    return !_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes")?2:0;
}
static inline BOOL rr_save_fullscreen_mode(const WCHAR *file,int mode){
    if(mode < -1||mode>2)return FALSE;
    return WritePrivateProfileStringW(L"graphics",L"fullscreen",mode<0?NULL:mode==0?L"0":mode==1?L"1":L"2",file)
        &&WritePrivateProfileStringW(L"graphics",L"borderless",NULL,file)
        &&WritePrivateProfileStringW(L"graphics",L"exclusive_fullscreen",NULL,file);
}
#endif
