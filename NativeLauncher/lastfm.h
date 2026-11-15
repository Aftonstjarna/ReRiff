/* Account linking uses the launcher's existing WinINet transport and native
 * controls. Tokens/session keys are never logged or passed on command lines. */
#include <wincrypt.h>
static HWND lfm_enabled,lfm_percent,lfm_account,lfm_link,lfm_finish,lfm_disconnect,lfm_save,lfm_browser;
static WCHAR lfm_token[33],lfm_app[33],lfm_url[256];
static int lfm_action;
static BOOL lfm_hex(const WCHAR *value){if(wcslen(value)!=32)return FALSE;for(int i=0;i<32;i++)if(!iswxdigit(value[i])||value[i]>127)return FALSE;return TRUE;}
static BOOL lfm_token_valid(const WCHAR *value){if(wcslen(value)!=32)return FALSE;for(int i=0;i<32;i++)if(value[i]>127||(!iswalnum(value[i])&&value[i]!=L'-'&&value[i]!=L'_'))return FALSE;return TRUE;}
static BOOL lfm_md5(const char *input,char out[33]){
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[16];DWORD size=16;BOOL ok=FALSE;
    if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)
        &&CryptCreateHash(provider,CALG_MD5,0,0,&hash)&&CryptHashData(hash,(const BYTE *)input,(DWORD)strlen(input),0)
        &&CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)){
        for(int i=0;i<16;i++)snprintf(out+i*2,3,"%02x",digest[i]);ok=TRUE;
    }
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);return ok;
}
static const char *lfm_field(const char *json,const char *name){
    const char *p=release_space(json);if(*p++!='{')return NULL;
    for(;;){char key[128];p=release_space(p);if(*p=='}')return NULL;p=release_string(p,key,sizeof(key));if(!p)return NULL;p=release_space(p);if(*p++!=':')return NULL;p=release_space(p);
        if(!strcmp(key,name))return p;p=release_skip(p,0);if(!p)return NULL;p=release_space(p);if(*p++!=',')return NULL;}
}
static BOOL lfm_text(const char *json,const char *name,WCHAR *out,int capacity){
    char value[1024];const char *p=lfm_field(json,name);return p&&release_string(p,value,sizeof(value))&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value,-1,out,capacity)>0;
}
static void lfm_error(int code,DWORD transport,DWORD http){
    if(code==14)wcscpy(message,L"Approve access in your browser, then click Finish linking.");
    else if(code==15)wcscpy(message,L"Authorization expired. Click Link account again.");
    else if(code==10)wcscpy(message,L"Last.fm rejected the API key. Check api_key in client.ini.");
    else if(code==13)wcscpy(message,L"Last.fm rejected the signature. Check shared_secret in client.ini.");
    else if(code==26)wcscpy(message,L"Last.fm reports this API key is suspended.");
    else if(code)_snwprintf(message,1024,L"Last.fm error %d. Try linking again.",code);
    else _snwprintf(message,1024,L"Last.fm connection failed (Windows %lu, HTTP %lu). Try again.",(unsigned long)transport,(unsigned long)http);
}
static BOOL lfm_request(BOOL finish){
    WCHAR key[64],secret[64],url[512];char signed_text[256],signature[33],json[65537];DWORD http=0,error=0,total=0;BOOL ok=FALSE;
    GetPrivateProfileStringW(L"lastfm",L"api_key",L"",key,64,ini);GetPrivateProfileStringW(L"lastfm",L"shared_secret",L"",secret,64,ini);
    if(!lfm_hex(key)||!lfm_hex(secret)){wcscpy(message,L"Add api_key and shared_secret to [lastfm] in client.ini.");return FALSE;}
    if(finish&&(!lfm_token_valid(lfm_token)||wcscmp(key,lfm_app))){wcscpy(message,L"Click Link account to start a new authorization.");return FALSE;}
    snprintf(signed_text,sizeof(signed_text),finish?"api_key%lsmethodauth.getSessiontoken%ls%ls":"api_key%lsmethodauth.getToken%ls%ls",key,finish?lfm_token:L"",secret);
    if(!lfm_md5(signed_text,signature)){SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(signed_text,sizeof(signed_text));lfm_error(0,GetLastError(),0);return FALSE;}
    SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(signed_text,sizeof(signed_text));
    _snwprintf(url,512,L"https://ws.audioscrobbler.com/2.0/?method=%ls&api_key=%ls&api_sig=%hs&format=json%ls%ls",finish?L"auth.getSession":L"auth.getToken",key,signature,finish?L"&token=":L"",finish?lfm_token:L"");
    HINTERNET internet=InternetOpenW(L"ReRiffLauncher/LastFm",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0),request=NULL;
    if(internet){DWORD timeout=15000;InternetSetOptionW(internet,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));InternetSetOptionW(internet,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));InternetSetOptionW(internet,INTERNET_OPTION_SEND_TIMEOUT,&timeout,sizeof(timeout));
        request=InternetOpenUrlW(internet,url,L"Accept: application/json\r\n",(DWORD)-1,INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_AUTO_REDIRECT|INTERNET_FLAG_NO_COOKIES,0);
    }
    if(request){DWORD size=sizeof(http);HttpQueryInfoW(request,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&http,&size,NULL);
        while(total<sizeof(json)-1){DWORD got=0;if(!InternetReadFile(request,json+total,(DWORD)sizeof(json)-1-total,&got)){error=GetLastError();break;}if(!got){ok=TRUE;break;}total+=got;}
    }else error=GetLastError();
    if(request)InternetCloseHandle(request);if(internet)InternetCloseHandle(internet);json[total]=0;
    const char *failure=ok?lfm_field(json,"error"):NULL;int code=failure?(int)strtol(failure,NULL,10):0;
    if(!ok||http!=200||code){lfm_error(code,error,http);SecureZeroMemory(json,sizeof(json));return FALSE;}
    if(!finish){
        ok=lfm_text(json,"token",lfm_token,33)&&lfm_token_valid(lfm_token);
        if(ok){wcscpy(lfm_app,key);_snwprintf(lfm_url,256,L"https://www.last.fm/api/auth/?api_key=%ls&token=%ls",key,lfm_token);wcscpy(message,L"Approve ReRiff in your browser, then click Finish linking.");}
    }else{
        WCHAR session[64],user[128];const char *object=lfm_field(json,"session");
        ok=object&&lfm_text(object,"key",session,64)&&lfm_token_valid(session)&&lfm_text(object,"name",user,128)&&*user&&!wcschr(user,L'\n')&&!wcschr(user,L'\r');
        if(ok){ok=WritePrivateProfileStringW(L"lastfm",L"enabled",L"false",ini)&&WritePrivateProfileStringW(L"lastfm",L"username",user,ini)&&WritePrivateProfileStringW(L"lastfm",L"session_key",session,ini);
            if(ok){SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;wcscpy(message,L"Account linked. Enable scrobbling, save, then restart the game.");}
            else wcscpy(message,L"Could not save the Last.fm account to client.ini.");
        }
        SecureZeroMemory(session,sizeof(session));
    }
    if(!ok&&http==200&&wcsncmp(message,L"Could not save",14))wcscpy(message,L"Last.fm returned an unexpected authorization response. Try again.");
    SecureZeroMemory(json,sizeof(json));return ok;
}
static void lfm_controls(void){
    EnableWindow(lfm_link,!busy);EnableWindow(lfm_finish,!busy&&lfm_token_valid(lfm_token));EnableWindow(lfm_disconnect,!busy);EnableWindow(lfm_save,!busy);EnableWindow(lfm_enabled,!busy);EnableWindow(lfm_percent,!busy);EnableWindow(lfm_browser,!busy&&*lfm_url);
}
static void lfm_reload(void){
    WCHAR user[128],text[180],value[32];GetPrivateProfileStringW(L"lastfm",L"username",L"",user,128,ini);
    if(*user)_snwprintf(text,180,L"Linked account: %ls",user);else wcscpy(text,L"No Last.fm account linked");SetWindowTextW(lfm_account,text);
    GetPrivateProfileStringW(L"lastfm",L"enabled",L"false",value,32,ini);SendMessageW(lfm_enabled,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    unsigned percent=GetPrivateProfileIntW(L"lastfm",L"scrobble_percent",80,ini);if(percent<1||percent>100)percent=80;_snwprintf(value,32,L"%u",percent);SetWindowTextW(lfm_percent,value);lfm_controls();
}
static void lfm_save_settings(void){
    WCHAR value[32],session[64],user[128];GetWindowTextW(lfm_percent,value,32);WCHAR *end;long percent=wcstol(value,&end,10);
    if(!*value||*end||percent<1||percent>100){SetWindowTextW(status,L"Choose a listening percentage from 1 to 100.");return;}
    BOOL enabled=SendMessageW(lfm_enabled,BM_GETCHECK,0,0)==BST_CHECKED;
    GetPrivateProfileStringW(L"lastfm",L"session_key",L"",session,64,ini);GetPrivateProfileStringW(L"lastfm",L"username",L"",user,128,ini);
    if(enabled&&(!lfm_token_valid(session)||!*user)){SetWindowTextW(status,L"Link your account before enabling scrobbling.");return;}
    BOOL ok=WritePrivateProfileStringW(L"lastfm",L"scrobble_percent",value,ini)&&WritePrivateProfileStringW(L"lastfm",L"enabled",enabled?L"true":L"false",ini);
    SetWindowTextW(status,ok?L"Last.fm settings saved. Restart the game to apply them.":L"Could not save Last.fm settings.");SecureZeroMemory(session,sizeof(session));
}
static void lfm_unlink(void){
    BOOL ok=WritePrivateProfileStringW(L"lastfm",L"enabled",L"false",ini)&&WritePrivateProfileStringW(L"lastfm",L"session_key",L"",ini)&&WritePrivateProfileStringW(L"lastfm",L"username",L"",ini);
    SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;lfm_reload();SetWindowTextW(status,ok?L"Last.fm disconnected. Application credentials were kept.":L"Could not disconnect Last.fm.");
}
static void lfm_open_browser(void){
    if(*lfm_url&&(INT_PTR)ShellExecuteW(window,L"open",lfm_url,NULL,NULL,SW_SHOWNORMAL)<=32)SetWindowTextW(status,L"Could not open your browser. Click Open authorization to try again.");
}
