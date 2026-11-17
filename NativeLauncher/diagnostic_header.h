#ifndef RERIFF_DIAGNOSTIC_HEADER_H
#define RERIFF_DIAGNOSTIC_HEADER_H
#include <wincrypt.h>
/* Non-secret snapshot only. Never use these fields to authenticate a player. */
static inline void rr_diagnostic_clean(WCHAR *value) {
    for(WCHAR *p=value;*p;p++)if(*p<32||(*p>=127&&*p<160)||*p==0x2028||*p==0x2029||*p=='['||*p==']')*p=' ';
}
static inline BOOL rr_diagnostic_uuid(const WCHAR *value) {
    if(wcslen(value)!=36)return FALSE;
    for(unsigned i=0;i<36;i++)if(i==8||i==13||i==18||i==23){if(value[i]!='-')return FALSE;}
        else if(!((value[i]>='0'&&value[i]<='9')||(value[i]>='a'&&value[i]<='f')||(value[i]>='A'&&value[i]<='F')))return FALSE;
    return TRUE;
}
static inline void rr_diagnostic_header(const WCHAR *path,char *out,size_t capacity) {
    WCHAR root[32768],context[32768],player[257],account[64],server[64],owner[32],started[32],version[128];
    wcsncpy(root,path,32767);root[32767]=0;WCHAR *slash=wcsrchr(root,L'\\');if(slash)*slash=0;
    slash=wcsrchr(root,L'\\');if(slash&&!_wcsicmp(slash+1,L"logs"))*slash=0;
    _snwprintf(context,32768,L"%ls\\localserver\\diagnostic-context.ini",root);context[32767]=0;
    GetPrivateProfileStringW(L"diagnostic",L"player",L"unavailable",player,257,context);
    GetPrivateProfileStringW(L"diagnostic",L"account_id",L"",account,64,context);
    GetPrivateProfileStringW(L"diagnostic",L"server_id",L"",server,64,context);
    GetPrivateProfileStringW(L"diagnostic",L"process_id",L"",owner,32,context);
    GetPrivateProfileStringW(L"diagnostic",L"process_started",L"",started,32,context);
    FILETIME created,exited,kernel,user_time;ULONGLONG birth=0;
    if(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user_time))birth=((ULONGLONG)created.dwHighDateTime<<32)|created.dwLowDateTime;
    BOOL current=birth&&wcstoul(owner,NULL,10)==GetCurrentProcessId()&&_wcstoui64(started,NULL,10)==birth;
    /* Reuse an already initialized Steam interface; never initialize Steam for
     * diagnostics. Non-Steam clients still retain their ReRiff UUID identity. */
    HMODULE steam=GetModuleHandleW(L"steam_api.dll");
    if(current&&steam){
        typedef int (__cdecl *UserFn)(void);typedef void *(__cdecl *FriendsFn)(void);
        typedef const char *(__thiscall *NameFn)(void *);
        UserFn user=(UserFn)(void *)GetProcAddress(steam,"SteamAPI_GetHSteamUser");
        FriendsFn friends=(FriendsFn)(void *)GetProcAddress(steam,"SteamFriends");
        if(user&&friends&&user()){
            void *instance=friends();
            if(instance){const char *name=((NameFn)(*(void ***)instance)[0])(instance);WCHAR actual[257];
                if(name&&*name&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name,-1,actual,257)){
                    rr_diagnostic_clean(actual);
                    if(wcscmp(player,actual)){wcscpy(player,actual);WritePrivateProfileStringW(L"diagnostic",L"player",actual,context);}
                }
            }
        }
    }
    if(!rr_diagnostic_uuid(account))wcscpy(account,L"unavailable");
    if(!rr_diagnostic_uuid(server))wcscpy(server,L"unavailable");rr_diagnostic_clean(player);
    _snwprintf(context,32768,L"%ls\\localserver\\client-version.txt",root);context[32767]=0;
    wcscpy(version,L"unavailable");FILE *metadata=_wfopen(context,L"rb");
    if(metadata){char text[128];size_t n=fread(text,1,127,metadata);text[n]=0;fclose(metadata);MultiByteToWideChar(CP_UTF8,0,text,-1,version,128);rr_diagnostic_clean(version);}
#ifdef RERIFF_VERSION
#if defined(RERIFF_DEVELOPMENT_BUILD) && RERIFF_DEVELOPMENT_BUILD
    const char *build="development build (" RERIFF_VERSION ")";
#else
    const char *build=RERIFF_VERSION;
#endif
#else
    const char *build="see installed_client";
#endif
    char client[64]="unavailable";
    _snwprintf(context,32768,L"%ls\\localserver\\diagnostic-client-id.txt",root);context[32767]=0;
    HANDLE idfile=CreateFileW(context,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    if(idfile!=INVALID_HANDLE_VALUE){
        HCRYPTPROV provider=0;BYTE bytes[16];
        if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptGenRandom(provider,16,bytes)){
            bytes[6]=(bytes[6]&15)|64;bytes[8]=(bytes[8]&63)|128;
            snprintf(client,sizeof(client),"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7],bytes[8],bytes[9],bytes[10],bytes[11],bytes[12],bytes[13],bytes[14],bytes[15]);
            DWORD written;WriteFile(idfile,client,36,&written,NULL);
        }
        if(provider)CryptReleaseContext(provider,0);CloseHandle(idfile);
    }
    FILE *idreader=_wfopen(context,L"rb");if(idreader){char id[38];size_t n=fread(id,1,37,idreader);id[n]=0;fclose(idreader);WCHAR wide_id[64];
        if(n==36&&MultiByteToWideChar(CP_UTF8,0,id,-1,wide_id,64)&&rr_diagnostic_uuid(wide_id))strcpy(client,id);
    }
    WCHAR process[MAX_PATH];GetModuleFileNameW(NULL,process,MAX_PATH);process[MAX_PATH-1]=0;WCHAR *base=wcsrchr(process,L'\\');base=base?base+1:process;
    char identity[2048];WCHAR wide[1024];
    _snwprintf(wide,1024,L"player=%ls; account_id=%ls; server_id=%ls; identity=%ls; installed_client=%ls; process=%ls",player,account,server,current?L"current":L"last-known",version,base);wide[1023]=0;
    if(!WideCharToMultiByte(CP_UTF8,0,wide,-1,identity,sizeof(identity),NULL,NULL))strcpy(identity,"identity=unavailable");
    snprintf(out,capacity,"[diagnostic] client_id=%s; %s; build=%s; platform=%s; pid=%lu; bits=%u",client,identity,build,GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version")?"Wine/Proton":"Windows",(unsigned long)GetCurrentProcessId(),(unsigned)(sizeof(void *)*8));
}
#endif
