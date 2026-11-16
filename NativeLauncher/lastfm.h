/* Account linking uses the launcher's existing WinINet transport and native
 * controls. Tokens/session keys are never logged or passed on command lines. */
#include <wincrypt.h>
#include "lastfm_storage.h"
static HWND lfm_enabled,lfm_percent,lfm_account,lfm_link,lfm_disconnect,lfm_save,lfm_key_field,lfm_secret_field;
static WCHAR lfm_token[33],lfm_app[33],lfm_url[256];
static int lfm_action,lfm_last_error;
static DWORD lfm_poll_at,lfm_poll_deadline;
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
    lfm_last_error=code;
    if(code==14)wcscpy(message,L"Waiting for browser approval. Linking will finish automatically.");
    else if(code==15)wcscpy(message,L"Authorization expired. Click Link account again.");
    else if(code==10)wcscpy(message,L"Last.fm rejected the API key. Check the API key in the Last.fm tab.");
    else if(code==13)wcscpy(message,L"Last.fm rejected the signature. Check the shared secret in the Last.fm tab.");
    else if(code==26)wcscpy(message,L"Last.fm reports this API key is suspended.");
    else if(code)_snwprintf(message,1024,L"Last.fm error %d. Try linking again.",code);
    else _snwprintf(message,1024,L"Last.fm connection failed (Windows %lu, HTTP %lu). Try again.",(unsigned long)transport,(unsigned long)http);
}
static BOOL lfm_request(BOOL finish){
    lfm_last_error=-1;
    WCHAR key[64],secret[64],url[512];char signed_text[256],signature[33],json[65537];DWORD http=0,error=0,total=0;BOOL ok=FALSE;
    lfm_read_credential(L"api_key",key);lfm_read_credential(L"shared_secret",secret);
    if(!lfm_hex(key)||!lfm_hex(secret)){wcscpy(message,L"Enter and save your API key and shared secret in the Last.fm tab.");return FALSE;}
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
        if(ok){wcscpy(lfm_app,key);_snwprintf(lfm_url,256,L"https://www.last.fm/api/auth/?api_key=%ls&token=%ls",key,lfm_token);wcscpy(message,L"Approve ReRiff in your browser. Linking will finish automatically.");}
    }else{
        WCHAR session[64],user[128];const char *object=lfm_field(json,"session");
        ok=object&&lfm_text(object,"key",session,64)&&lfm_token_valid(session)&&lfm_text(object,"name",user,128)&&*user&&!wcschr(user,L'\n')&&!wcschr(user,L'\r');
        if(ok){ok=lfm_read_credential(L"shared_secret",secret)&&lfm_store_credentials(key,secret,session,user,FALSE,NULL);SecureZeroMemory(secret,sizeof(secret));
            if(ok){SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;wcscpy(message,L"Account linked. Enable scrobbling, save, then restart the game.");}
            else wcscpy(message,L"Could not save the Last.fm account to client.ini.");
        }
        SecureZeroMemory(session,sizeof(session));
    }
    if(!ok&&http==200&&wcsncmp(message,L"Could not save",14))wcscpy(message,L"Last.fm returned an unexpected authorization response. Try again.");
    SecureZeroMemory(json,sizeof(json));return ok;
}
static BOOL lfm_has_api_key(void){
    WCHAR key[64];return lfm_read_credential(L"api_key",key)&&*key;
}
static void lfm_register_application(void){
    if((INT_PTR)ShellExecuteW(window,L"open",L"https://www.last.fm/api/account/create",NULL,NULL,SW_SHOWNORMAL)<=32)
        SetWindowTextW(status,L"Open https://www.last.fm/api/account/create in your browser to get an API key.");
    else SetWindowTextW(status,L"Create an API account, then enter and save its key and secret here.");
}
static void lfm_controls(void){
    EnableWindow(lfm_key_field,!busy);EnableWindow(lfm_secret_field,!busy);
    SetWindowTextW(lfm_link,*lfm_url?L"Reopen authorization":lfm_has_api_key()?L"Link account":L"Get API key");
    EnableWindow(lfm_link,!busy);EnableWindow(lfm_disconnect,!busy);EnableWindow(lfm_save,!busy);EnableWindow(lfm_enabled,!busy);EnableWindow(lfm_percent,!busy);
}
static void lfm_reload(void){
    WCHAR credential[64];lfm_read_credential(L"api_key",credential);SetWindowTextW(lfm_key_field,credential);lfm_read_credential(L"shared_secret",credential);SetWindowTextW(lfm_secret_field,credential);SecureZeroMemory(credential,sizeof(credential));
    WCHAR user[128],text[180],value[32];GetPrivateProfileStringW(L"lastfm",L"username",L"",user,128,ini);
    if(*user)_snwprintf(text,180,L"Linked account: %ls",user);else wcscpy(text,L"No Last.fm account linked");SetWindowTextW(lfm_account,text);
    GetPrivateProfileStringW(L"lastfm",L"enabled",L"false",value,32,ini);SendMessageW(lfm_enabled,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    unsigned percent=GetPrivateProfileIntW(L"lastfm",L"scrobble_percent",80,ini);if(percent<1||percent>100)percent=80;_snwprintf(value,32,L"%u",percent);SetWindowTextW(lfm_percent,value);lfm_controls();
}
static BOOL lfm_save_settings(void){
    WCHAR key[64],secret[64],oldkey[64],oldsecret[64],session[64],user[128],value[32];
    GetWindowTextW(lfm_percent,value,32);WCHAR *end;long percent=wcstol(value,&end,10);
    if(!*value||*end||percent<1||percent>100){SetWindowTextW(status,L"Choose a listening percentage from 1 to 100.");return FALSE;}
    GetWindowTextW(lfm_key_field,key,64);GetWindowTextW(lfm_secret_field,secret,64);
    if((*key||*secret)&&(!lfm_hex(key)||!lfm_hex(secret))){SecureZeroMemory(secret,sizeof(secret));SetWindowTextW(status,L"Enter a 32-character API key and shared secret, then save settings.");return FALSE;}
    BOOL readable=lfm_read_credential(L"api_key",oldkey)&&lfm_read_credential(L"shared_secret",oldsecret)&&lfm_read_credential(L"session_key",session);
    BOOL changed=!readable||wcscmp(key,oldkey)||wcscmp(secret,oldsecret);
    BOOL enabled=SendMessageW(lfm_enabled,BM_GETCHECK,0,0)==BST_CHECKED;
    GetPrivateProfileStringW(L"lastfm",L"username",L"",user,128,ini);
    if(changed){*session=0;*user=0;enabled=FALSE;}
    if(enabled&&(!*key||!lfm_token_valid(session)||!*user)){
        SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(oldsecret,sizeof(oldsecret));SecureZeroMemory(session,sizeof(session));
        SetWindowTextW(status,L"Link your account before enabling scrobbling.");return FALSE;
    }
    BOOL ok=lfm_store_credentials(key,secret,session,user,enabled,value);
    SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(oldsecret,sizeof(oldsecret));SecureZeroMemory(session,sizeof(session));
    if(ok){if(changed){SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;}lfm_reload();}
    SetWindowTextW(status,ok?(changed?L"Settings saved. Link your Last.fm account before enabling scrobbling.":L"Last.fm settings saved and credentials protected. Restart the game."):L"Could not protect or save settings. Existing settings were kept.");return ok;
}
static void lfm_unlink(void){
    BOOL ok=WritePrivateProfileStringW(L"lastfm",L"enabled",L"false",ini)&&WritePrivateProfileStringW(L"lastfm",L"session_key",L"",ini)&&WritePrivateProfileStringW(L"lastfm",L"username",L"",ini);
    SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;lfm_reload();SetWindowTextW(status,ok?L"Last.fm disconnected. Application credentials were kept.":L"Could not disconnect Last.fm.");
}
static void lfm_open_browser(void){
    if(*lfm_url&&(INT_PTR)ShellExecuteW(window,L"open",lfm_url,NULL,NULL,SW_SHOWNORMAL)<=32)SetWindowTextW(status,L"Could not open your browser. Click Reopen authorization to try again.");
}

/* Timer checks run on the UI thread; requests use the existing launcher worker. */
static void lfm_link_completed(int action,BOOL ok,DWORD now){
    if(action==8&&ok){lfm_poll_at=now+5000;lfm_poll_deadline=now+600000;}
    if(action==9){
        if(ok)lfm_poll_at=lfm_poll_deadline=0;
        else if(lfm_last_error==14||lfm_last_error==0||lfm_last_error==8||lfm_last_error==11||lfm_last_error==16||lfm_last_error==29)
            lfm_poll_at=now+(lfm_last_error==14?5000:30000);
        else {lfm_poll_at=lfm_poll_deadline=0;SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;}
    }
}
static BOOL lfm_poll_due(DWORD now){
    if(busy||!lfm_poll_deadline||!*lfm_url||!lfm_token_valid(lfm_token))return FALSE;
    if((LONG)(now-lfm_poll_deadline)>=0){
        lfm_poll_at=lfm_poll_deadline=0;SecureZeroMemory(lfm_token,sizeof(lfm_token));*lfm_url=0;lfm_controls();
        SetWindowTextW(status,L"Linking timed out. Click Link account to try again.");return FALSE;
    }
    return (LONG)(now-lfm_poll_at)>=0;
}
