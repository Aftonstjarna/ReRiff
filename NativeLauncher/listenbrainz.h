/* Independent token configuration; never put tokens in URLs, logs or arguments. */
static HWND lb_token,lb_enabled,lb_percent,lb_save,lb_get_token;
static BOOL lb_token_valid(const WCHAR *token){
    if(wcslen(token)!=36)return FALSE;BOOL nonzero=FALSE;
    for(int i=0;i<36;i++){
        if(i==8||i==13||i==18||i==23){if(token[i]!=L'-')return FALSE;}
        else {if(token[i]>127||!iswxdigit(token[i]))return FALSE;if(token[i]!=L'0')nonzero=TRUE;}
    }
    return nonzero;
}
static void lb_controls(void){EnableWindow(lb_token,!busy);EnableWindow(lb_enabled,!busy);EnableWindow(lb_percent,!busy);EnableWindow(lb_save,!busy);EnableWindow(lb_get_token,!busy);}
static void lb_reload(void){
    WCHAR token[64],value[32];scrobble_read_credential(L"listenbrainz",L"user_token",token);SetWindowTextW(lb_token,token);SecureZeroMemory(token,sizeof(token));
    GetPrivateProfileStringW(L"listenbrainz",L"enabled",L"false",value,32,ini);
    SendMessageW(lb_enabled,BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    unsigned percent=GetPrivateProfileIntW(L"listenbrainz",L"scrobble_percent",80,ini);if(percent<1||percent>100)percent=80;
    _snwprintf(value,32,L"%u",percent);SetWindowTextW(lb_percent,value);lb_controls();
}
static BOOL lb_save_settings(void){
    WCHAR token[64],previous[64],value[32],encrypted[4096],routed[CAP],temporary[CAP];
    GetWindowTextW(lb_token,token,64);GetWindowTextW(lb_percent,value,32);
    WCHAR *end;long percent=wcstol(value,&end,10);BOOL enabled=SendMessageW(lb_enabled,BM_GETCHECK,0,0)==BST_CHECKED;
    if(!*value||*end||percent<1||percent>100){SetWindowTextW(status,L"Choose a listening percentage from 1 to 100.");SecureZeroMemory(token,sizeof(token));return FALSE;}
    if((*token&&!lb_token_valid(token))||(enabled&&!*token)){SetWindowTextW(status,L"Enter your 36-character ListenBrainz user token, or disable scrobbling to clear it.");SecureZeroMemory(token,sizeof(token));return FALSE;}
    if(!scrobble_read_credential(L"listenbrainz",L"user_token",previous)){SecureZeroMemory(token,sizeof(token));SetWindowTextW(status,L"Could not read the protected token. Settings were kept; use the original Windows profile or Proton prefix.");return FALSE;}
    SecureZeroMemory(previous,sizeof(previous));
    BOOL ok=lfm_protect(token,encrypted);SecureZeroMemory(token,sizeof(token));
    if(!ok){SetWindowTextW(status,L"Could not protect the ListenBrainz token. Existing settings were kept.");return FALSE;}
    const WCHAR *file=rr_config_path(ini,L"listenbrainz",routed);
    if(wcslen(file)+24>=CAP)return FALSE;_snwprintf(temporary,CAP,L"%ls.listenbrainz-new",file);
    BOOL exists=GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES;
    if(exists&&!CopyFileW(file,temporary,FALSE))ok=FALSE;
    if(!exists){HANDLE h=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(h==INVALID_HANDLE_VALUE)ok=FALSE;else CloseHandle(h);}
    if(ok)ok=WritePrivateProfileStringW(L"listenbrainz",L"user_token",encrypted,temporary)&&WritePrivateProfileStringW(L"listenbrainz",L"enabled",enabled?L"true":L"false",temporary)&&WritePrivateProfileStringW(L"listenbrainz",L"scrobble_percent",value,temporary);
    SecureZeroMemory(encrypted,sizeof(encrypted));WritePrivateProfileStringW(NULL,NULL,NULL,temporary);
    if(ok)ok=MoveFileExW(temporary,file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);if(!ok)DeleteFileW(temporary);
    if(ok)lb_reload();
    SetWindowTextW(status,ok?L"ListenBrainz settings saved and token protected. Restart the game.":L"Could not save ListenBrainz settings. Existing settings were kept.");return ok;
}
static void lb_open_token(void){
    if((INT_PTR)ShellExecuteW(window,L"open",L"https://listenbrainz.org/settings/",NULL,NULL,SW_SHOWNORMAL)<=32)SetWindowTextW(status,L"Open https://listenbrainz.org/settings/ in your browser to get your user token.");
}
