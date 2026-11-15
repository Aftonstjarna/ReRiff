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
    CHECK(WritePrivateProfileStringW(L"lastfm",L"session_key",L"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",ini));
    CHECK(WritePrivateProfileStringW(L"lastfm",L"username",L"fixture",ini));
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LastFmInlineFixture";cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);CHECK(RegisterClassW(&cls));
    busy=TRUE;HWND host=CreateWindowW(cls.lpszClassName,L"Last.fm inline fixture",WS_OVERLAPPEDWINDOW,0,0,660,615,NULL,NULL,cls.hInstance,NULL);CHECK(host);busy=FALSE;lfm_controls();ShowWindow(host,SW_SHOW);
    SendMessageW(launcher_tabs,TCM_SETCURSEL,3,0);select_launcher_page();CHECK(IsWindowVisible(lfm_link)&&IsWindowVisible(lfm_enabled)&&!IsWindowVisible(external));CHECK(GetParent(lfm_link)==host);
    SetWindowTextW(lfm_percent,L"101");lfm_save_settings();GetPrivateProfileStringW(L"lastfm",L"scrobble_percent",L"unset",value,64,ini);CHECK(!wcscmp(value,L"unset"));
    SetWindowTextW(lfm_percent,L"80");SendMessageW(lfm_enabled,BM_SETCHECK,BST_CHECKED,0);lfm_save_settings();GetPrivateProfileStringW(L"lastfm",L"enabled",L"",value,64,ini);CHECK(!wcscmp(value,L"true"));
    if(argc==2){DWORD until=GetTickCount()+4000;MSG msg;while((LONG)(until-GetTickCount())>0){while(PeekMessageW(&msg,NULL,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}Sleep(10);}}
    lfm_unlink();GetPrivateProfileStringW(L"lastfm",L"session_key",L"",value,64,ini);CHECK(!*value);GetPrivateProfileStringW(L"lastfm",L"api_key",L"",value,64,ini);CHECK(lfm_hex(value));
    SendMessageW(launcher_tabs,TCM_SETCURSEL,0,0);select_launcher_page();CHECK(!IsWindowVisible(lfm_enabled)&&IsWindowVisible(external));DestroyWindow(host);
    path(file,L"client.ini");DeleteFileW(file);DeleteFileW(ini);RemoveDirectoryW(folder);
    puts("PASS: Last.fm signature, JSON, opaque tokens, inline controls, threshold validation, settings persistence and disconnect");return 0;
}
