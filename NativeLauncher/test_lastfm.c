#define wWinMain launcher_main
#include "launcher.c"
#undef wWinMain
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d\n",__LINE__);return 1;}}while(0)
int wmain(int argc,WCHAR **argv){
    char hash[33];CHECK(lfm_md5("api_keytestmethodauth.getTokensecret",hash));CHECK(!strcmp(hash,"0bf56d6ed6962e4fd19bdf818c538e65"));
    WCHAR text[128];CHECK(lfm_text("{\"session\":{\"name\":\"snow\"},\"token\":\"abc\"}","token",text,128)&&!wcscmp(text,L"abc"));
    CHECK(lfm_text(lfm_field("{\"session\":{\"name\":\"snow\"}}","session"),"name",text,128)&&!wcscmp(text,L"snow"));
    CHECK(!lfm_text("{\"other\":\"token\"}","token",text,128));CHECK(!lfm_hex(L"invalid"));CHECK(lfm_token_valid(L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ-_"));
    if(argc==2&&wcscmp(argv[1],L"--preview")){wcsncpy(ini,argv[1],CAP-1);BOOL ok=lfm_request(FALSE);printf("Native auth token received: %s\n",ok?"yes":"no");if(!ok)wprintf(L"%ls\n",message);SecureZeroMemory(lfm_token,sizeof(lfm_token));SecureZeroMemory(lfm_url,sizeof(lfm_url));return ok?0:1;}
    WCHAR tmp[CAP],file[CAP],value[64];CHECK(GetTempPathW(CAP,tmp));CHECK(GetTempFileNameW(tmp,L"lfm",0,folder));CHECK(DeleteFileW(folder)&&CreateDirectoryW(folder,NULL));path(ini,L"server.ini");
    CHECK(WritePrivateProfileStringW(L"lastfm",L"api_key",L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",ini));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"shared_secret",L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",ini));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"session_key",L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",ini));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"username",L"fixture",ini));
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LastFmInlineFixture";cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);CHECK(RegisterClassW(&cls));
    busy=TRUE;HWND host=CreateWindowW(cls.lpszClassName,L"Last.fm inline fixture",WS_OVERLAPPEDWINDOW,0,0,660,615,NULL,NULL,cls.hInstance,NULL);CHECK(host);busy=FALSE;lfm_controls();ShowWindow(host,SW_SHOW);
    SendMessageW(launcher_tabs,TCM_SETCURSEL,3,0);select_launcher_page();CHECK(IsWindowVisible(lfm_link)&&IsWindowVisible(lfm_enabled)&&!IsWindowVisible(external));CHECK(GetParent(lfm_link)==host);
    GetWindowTextW(lfm_link,text,128);CHECK(!wcscmp(text,L"Link account"));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"api_key",L"",ini));lfm_controls();
    CHECK(!lfm_has_api_key());GetWindowTextW(lfm_link,text,128);CHECK(!wcscmp(text,L"Get API key"));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"api_key",L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",ini));lfm_controls();
    CHECK(lfm_has_api_key());GetWindowTextW(lfm_link,text,128);CHECK(!wcscmp(text,L"Link account"));
    wcscpy(lfm_token,L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");wcscpy(lfm_url,L"https://www.last.fm/api/auth/");
    lfm_link_completed(8,TRUE,1000);CHECK(!lfm_poll_due(5999)&&lfm_poll_due(6000));
    busy=TRUE;CHECK(!lfm_poll_due(6000));busy=FALSE;
    lfm_last_error=14;lfm_link_completed(9,FALSE,6000);CHECK(!lfm_poll_due(10999)&&lfm_poll_due(11000));
    lfm_controls();GetWindowTextW(lfm_link,text,128);CHECK(!wcscmp(text,L"Reopen authorization"));
    lfm_last_error=0;lfm_link_completed(9,FALSE,11000);CHECK(!lfm_poll_due(40999)&&lfm_poll_due(41000));
    lfm_last_error=15;lfm_link_completed(9,FALSE,41000);CHECK(!*lfm_url&&!lfm_poll_due(50000));
    wcscpy(lfm_token,L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ");wcscpy(lfm_url,L"https://www.last.fm/api/auth/");lfm_link_completed(8,TRUE,1000);
    CHECK(!lfm_poll_due(601000)&&!*lfm_url);lfm_link_completed(9,TRUE,601000);CHECK(!lfm_poll_deadline);
    SetWindowTextW(lfm_percent,L"101");lfm_save_settings();GetPrivateProfileStringW(L"lastfm",L"scrobble_percent",L"unset",value,64,ini);CHECK(!wcscmp(value,L"unset"));
    SetWindowTextW(lfm_percent,L"80");SendMessageW(lfm_enabled,BM_SETCHECK,BST_CHECKED,0);lfm_save_settings();GetPrivateProfileStringW(L"lastfm",L"enabled",L"",value,64,ini);CHECK(!wcscmp(value,L"true"));
    SetWindowTextW(lfm_percent,L"85");
    CHECK(WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"true",ini));
    CHECK(lfm_save_settings());
    GetPrivateProfileStringW(L"launcher",L"skip_launcher",L"",value,64,ini);CHECK(!wcscmp(value,L"true"));
    SetWindowTextW(lfm_secret_field,L"invalid");CHECK(!lfm_save_settings());CHECK(lfm_read_credential(L"shared_secret",value)&&!wcscmp(value,L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));lfm_reload();
    GetPrivateProfileStringW(L"lastfm",L"scrobble_percent",L"",value,64,ini);CHECK(!wcscmp(value,L"85"));
    WCHAR protected_value[4096];GetPrivateProfileStringW(L"lastfm",L"shared_secret",L"",protected_value,4096,ini);CHECK(!wcsncmp(protected_value,L"dpapi:",6));CHECK(!wcsstr(protected_value,L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    CHECK(lfm_read_credential(L"shared_secret",value)&&!wcscmp(value,L"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
    CHECK(lfm_read_credential(L"session_key",value)&&lfm_token_valid(value));
    CHECK((GetWindowLongPtrW(lfm_secret_field,GWL_STYLE)&ES_PASSWORD)!=0);
    if(argc==3&&!wcscmp(argv[1],L"--storage-test")){path(file,L"client.ini");CHECK(CopyFileW(file,argv[2],FALSE));}
    CHECK(WritePrivateProfileStringW(L"lastfm",L"session_key",L"dpapi:00",ini));CHECK(!lfm_read_credential(L"session_key",value));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"session_key",L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",ini));
    /* Tab repaint must never cover page controls or the shared footer. */
    RECT tabs,field;GetWindowRect(launcher_tabs,&tabs);GetWindowRect(lfm_key_field,&field);CHECK(tabs.bottom<=field.top);
    for(int cycle=0;cycle<3;cycle++)for(int page=0;page<4;page++){
        SendMessageW(launcher_tabs,TCM_SETCURSEL,page,0);select_launcher_page();
        RedrawWindow(host,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
        CHECK(IsWindowVisible(play_button)&&IsWindowVisible(status));
        CHECK(IsWindowVisible(lfm_key_field)==(page==3));
    }
    if(argc==2){DWORD until=GetTickCount()+4000;MSG msg;while((LONG)(until-GetTickCount())>0){while(PeekMessageW(&msg,NULL,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}Sleep(10);}}
    SetWindowTextW(lfm_key_field,L"cccccccccccccccccccccccccccccccc");CHECK(lfm_save_settings());
    CHECK(lfm_read_credential(L"session_key",value)&&!*value);GetPrivateProfileStringW(L"lastfm",L"enabled",L"",value,64,ini);CHECK(!wcscmp(value,L"false"));
    lfm_unlink();GetPrivateProfileStringW(L"lastfm",L"session_key",L"",value,64,ini);CHECK(!*value);CHECK(lfm_read_credential(L"api_key",value)&&lfm_hex(value));
    SendMessageW(launcher_tabs,TCM_SETCURSEL,0,0);select_launcher_page();CHECK(!IsWindowVisible(lfm_enabled)&&!IsWindowVisible(lfm_key_field)&&!IsWindowVisible(lfm_secret_field)&&!IsWindowVisible(lfm_save)&&IsWindowVisible(external));DestroyWindow(host);
    path(file,L"client.ini");DeleteFileW(file);DeleteFileW(ini);RemoveDirectoryW(folder);
    puts("PASS: Last.fm signature, JSON, opaque tokens, inline controls, threshold validation, settings persistence and disconnect");return 0;
}
