/* Libre.fm's GNU FM auth.getToken/getSession browser flow. No personal API
 * credentials are required. Session secrets use the shared DPAPI store. */
#define LIBRE_KEY L"72657269666600000000000000000000"
#define LIBRE_SECRET L"00000000000000000000000000000000"
static HWND libre_key_field,libre_account,libre_enabled,libre_percent,libre_link,libre_disconnect,libre_save;
static WCHAR libre_token[33],libre_app[33],libre_url[256];
static int libre_action,libre_last_error;
static DWORD libre_poll_at,libre_poll_deadline;
static BOOL libre_read_key(WCHAR key[64]){
    if(!scrobble_read_credential(L"librefm",L"api_key",key))return FALSE;
    if(!*key)wcscpy(key,LIBRE_KEY);return lfm_hex(key);
}
static void libre_controls(void){
    SetWindowTextW(libre_link,*libre_url?L"Reopen authorization":L"Link account");
    EnableWindow(libre_key_field,!busy);EnableWindow(libre_link,!busy);EnableWindow(libre_disconnect,!busy);EnableWindow(libre_save,!busy);EnableWindow(libre_enabled,!busy);EnableWindow(libre_percent,!busy);
}
static void libre_reload(void){
    WCHAR key[64];scrobble_read_credential(L"librefm",L"api_key",key);SetWindowTextW(libre_key_field,key);SecureZeroMemory(key,sizeof(key));
    WCHAR user[128],text[180],value[32];GetPrivateProfileStringW(L"librefm",L"username",L"",user,128,ini);
    if(*user)_snwprintf(text,180,L"Linked account: %ls",user);else wcscpy(text,L"No Libre.fm account linked");SetWindowTextW(libre_account,text);
    GetPrivateProfileStringW(L"librefm",L"enabled",L"false",value,32,ini);SendMessageW(libre_enabled,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    unsigned percent=GetPrivateProfileIntW(L"librefm",L"scrobble_percent",80,ini);if(percent<1||percent>100)percent=80;_snwprintf(value,32,L"%u",percent);SetWindowTextW(libre_percent,value);libre_controls();
}
static BOOL libre_save_settings(void){
    WCHAR key[64],oldkey[64],session[64],user[128],value[32];GetWindowTextW(libre_percent,value,32);WCHAR *end;long percent=wcstol(value,&end,10);
    if(!*value||*end||percent<1||percent>100){SetWindowTextW(status,L"Choose a listening percentage from 1 to 100.");return FALSE;}
    GetWindowTextW(libre_key_field,key,64);if(!*key)wcscpy(key,LIBRE_KEY);
    if(!lfm_hex(key)){SetWindowTextW(status,L"Enter a 32-character hexadecimal Libre.fm API key.");return FALSE;}
    if(!libre_read_key(oldkey)||!scrobble_read_credential(L"librefm",L"session_key",session)){SetWindowTextW(status,L"Could not read protected Libre.fm credentials. Existing settings were kept.");return FALSE;}
    GetPrivateProfileStringW(L"librefm",L"username",L"",user,128,ini);
    BOOL enabled=SendMessageW(libre_enabled,BM_GETCHECK,0,0)==BST_CHECKED;
    BOOL changed=wcscmp(key,oldkey)!=0;
    if(changed){*session=0;*user=0;enabled=FALSE;}
    if(enabled&&(!lfm_token_valid(session)||!*user)){SecureZeroMemory(session,sizeof(session));SetWindowTextW(status,L"Link your Libre.fm account before enabling scrobbling.");return FALSE;}
    BOOL ok=scrobble_store_credentials(L"librefm",key,LIBRE_SECRET,session,user,enabled,value);SecureZeroMemory(session,sizeof(session));
    if(ok){if(changed){SecureZeroMemory(libre_token,sizeof(libre_token));*libre_url=0;libre_poll_at=libre_poll_deadline=0;}libre_reload();}SetWindowTextW(status,ok?(changed?L"API key saved. Link your Libre.fm account, then enable scrobbling.":L"Libre.fm settings saved. Restart the game."):L"Could not save Libre.fm settings. Existing settings were kept.");return ok;
}
static void libre_unlink(void){
    WCHAR key[64];BOOL ok=libre_read_key(key)&&scrobble_store_credentials(L"librefm",key,LIBRE_SECRET,L"",L"",FALSE,NULL);
    if(ok){SecureZeroMemory(libre_token,sizeof(libre_token));*libre_url=0;libre_poll_at=libre_poll_deadline=0;libre_reload();}
    SetWindowTextW(status,ok?L"Libre.fm disconnected.":L"Could not disconnect Libre.fm.");
}
static void libre_error(int code,DWORD transport,DWORD http){
    libre_last_error=code;
    if(code==14)wcscpy(message,L"Waiting for browser approval. Linking will finish automatically.");
    else if(code==15)wcscpy(message,L"Authorization expired. Click Link account again.");
    else if(code==10)wcscpy(message,L"Libre.fm rejected the API key. Check the saved key.");
    else if(code==13)wcscpy(message,L"Libre.fm rejected ReRiff's request signature.");
    else if(code==26)wcscpy(message,L"Libre.fm reports this API key is suspended.");
    else if(code)_snwprintf(message,1024,L"Libre.fm error %d. Try linking again.",code);
    else _snwprintf(message,1024,L"Libre.fm connection failed (Windows %lu, HTTP %lu). Try again.",(unsigned long)transport,(unsigned long)http);
}
static BOOL libre_request(BOOL finish){
    libre_last_error=-1;
    WCHAR key[64],secret[64],url[512];char signed_text[256],signature[33],json[65537];DWORD http=0,error=0,total=0;BOOL ok=FALSE;
    if(!libre_read_key(key)){wcscpy(message,L"Could not read the Libre.fm API key. Existing settings were kept.");return FALSE;}wcscpy(secret,LIBRE_SECRET);
    if(!lfm_hex(key)||!lfm_hex(secret)){wcscpy(message,L"Enter and save your API key in the Libre.fm tab.");return FALSE;}
    if(finish&&(!lfm_token_valid(libre_token)||wcscmp(key,libre_app))){wcscpy(message,L"Click Link account to start a new authorization.");return FALSE;}
    snprintf(signed_text,sizeof(signed_text),finish?"api_key%lsmethodauth.getSessiontoken%ls%ls":"api_key%lsmethodauth.getToken%ls%ls",key,finish?libre_token:L"",secret);
    if(!lfm_md5(signed_text,signature)){SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(signed_text,sizeof(signed_text));libre_error(0,GetLastError(),0);return FALSE;}
    SecureZeroMemory(secret,sizeof(secret));SecureZeroMemory(signed_text,sizeof(signed_text));
    _snwprintf(url,512,L"https://libre.fm/2.0/?method=%ls&api_key=%ls&api_sig=%hs&format=json%ls%ls",finish?L"auth.getSession":L"auth.getToken",key,signature,finish?L"&token=":L"",finish?libre_token:L"");
    scrobble_auth_fetch(L"ReRiffLauncher/LibreFm",url,json,&http,&error,&total,&ok);
    const char *failure=ok?lfm_field(json,"error"):NULL;int code=failure?(int)strtol(failure,NULL,10):0;
    if(!ok||http!=200||code){libre_error(code,error,http);SecureZeroMemory(json,sizeof(json));return FALSE;}
    if(!finish){
        ok=lfm_text(json,"token",libre_token,33)&&lfm_token_valid(libre_token);
        if(ok){wcscpy(libre_app,key);_snwprintf(libre_url,256,L"https://libre.fm/api/auth/?api_key=%ls&token=%ls",key,libre_token);wcscpy(message,L"Approve ReRiff in your browser. Linking will finish automatically.");}
    }else{
        WCHAR session[64],user[128];const char *object=lfm_field(json,"session");
        ok=object&&lfm_text(object,"key",session,64)&&lfm_token_valid(session)&&lfm_text(object,"name",user,128)&&*user&&!wcschr(user,L'\n')&&!wcschr(user,L'\r');
        if(ok){wcscpy(secret,LIBRE_SECRET);ok=scrobble_store_credentials(L"librefm",key,secret,session,user,FALSE,NULL);SecureZeroMemory(secret,sizeof(secret));
            if(ok){SecureZeroMemory(libre_token,sizeof(libre_token));*libre_url=0;wcscpy(message,L"Account linked. Enable scrobbling, save, then restart the game.");}
            else wcscpy(message,L"Could not save the Libre.fm account to client.ini.");
        }
        SecureZeroMemory(session,sizeof(session));
    }
    if(!ok&&http==200&&wcsncmp(message,L"Could not save",14))wcscpy(message,L"Libre.fm returned an unexpected authorization response. Try again.");
    SecureZeroMemory(json,sizeof(json));return ok;
}
static void libre_open_browser(void){
    if(*libre_url&&(INT_PTR)ShellExecuteW(window,L"open",libre_url,NULL,NULL,SW_SHOWNORMAL)<=32)SetWindowTextW(status,L"Could not open your browser. Click Reopen authorization to try again.");
}

/* Timer checks run on the UI thread; requests use the existing launcher worker. */
static void libre_link_completed(int action,BOOL ok,DWORD now){
    if(action==8&&ok){libre_poll_at=now+5000;libre_poll_deadline=now+600000;}
    if(action==9){
        if(ok)libre_poll_at=libre_poll_deadline=0;
        else if(libre_last_error==14||libre_last_error==0||libre_last_error==8||libre_last_error==11||libre_last_error==16||libre_last_error==29)
            libre_poll_at=now+(libre_last_error==14?5000:30000);
        else {libre_poll_at=libre_poll_deadline=0;SecureZeroMemory(libre_token,sizeof(libre_token));*libre_url=0;}
    }
}
static BOOL libre_poll_due(DWORD now){
    if(busy||!libre_poll_deadline||!*libre_url||!lfm_token_valid(libre_token))return FALSE;
    if((LONG)(now-libre_poll_deadline)>=0){
        libre_poll_at=libre_poll_deadline=0;SecureZeroMemory(libre_token,sizeof(libre_token));*libre_url=0;libre_controls();
        SetWindowTextW(status,L"Linking timed out. Click Link account to try again.");return FALSE;
    }
    return (LONG)(now-libre_poll_at)>=0;
}
