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
static inline const WCHAR *rr_log_group(const WCHAR *category) {
    if(!_wcsicmp(category,L"client")||!_wcsicmp(category,L"network")||!_wcsicmp(category,L"news"))return L"connections";
    if(!_wcsicmp(category,L"engine_menu")||!_wcsicmp(category,L"singleplayer"))return L"game";
    if(!_wcsicmp(category,L"unicode")||!_wcsicmp(category,L"metadata"))return L"text";
    return category;
}
static inline int rr_log_setting(const WCHAR *game,const WCHAR *category) {
    WCHAR file[32768],key[80],value[32];
    _snwprintf(file,32768,L"%ls\\server.ini",game);file[32767]=0;
    _snwprintf(key,80,L"log_%ls",category);key[79]=0;
    GetPrivateProfileStringW(L"diagnostics",key,L"",value,32,file);
    if(!*value)return -1;
    return !_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
}
static inline BOOL rr_log_selected(const WCHAR *game,const WCHAR *category) {
    const WCHAR *group=rr_log_group(category);int selected=rr_log_setting(game,group);
    if(selected>=0)return selected;
    /* Preserve legacy component choices until the user changes the group. */
    if(_wcsicmp(category,group))return rr_log_setting(game,category)!=0;
    if(!_wcsicmp(group,L"connections"))return rr_log_setting(game,L"client")!=0||rr_log_setting(game,L"network")!=0||rr_log_setting(game,L"news")!=0;
    if(!_wcsicmp(group,L"game"))return rr_log_setting(game,L"engine_menu")!=0||rr_log_setting(game,L"singleplayer")!=0;
    if(!_wcsicmp(group,L"text"))return rr_log_setting(game,L"unicode")!=0||rr_log_setting(game,L"metadata")!=0;
    return TRUE;
}
/* Shared with managed writers: no persistent write handles or mixed encodings. */
static inline void rr_log_append(const WCHAR *path,const char *component,const char *line) {
    HANDLE mutex=CreateMutexW(NULL,FALSE,L"Local\\ReRiff.Diagnostics");if(!mutex)return;
    DWORD wait=WaitForSingleObject(mutex,1000);
    if(wait==WAIT_OBJECT_0||wait==WAIT_ABANDONED){
        HANDLE file=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file!=INVALID_HANDLE_VALUE){
            DWORD n;WriteFile(file,"[",1,&n,NULL);WriteFile(file,component,(DWORD)strlen(component),&n,NULL);
            WriteFile(file,"] ",2,&n,NULL);WriteFile(file,line,(DWORD)strlen(line),&n,NULL);WriteFile(file,"\r\n",2,&n,NULL);CloseHandle(file);
        }
        ReleaseMutex(mutex);
    }
    CloseHandle(mutex);
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
