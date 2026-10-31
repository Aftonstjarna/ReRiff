#include "menu.c"
#include <stdio.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition); return 1; } } while (0)

int main(void) {
    CHECK(valid_url(L""));
    CHECK(valid_url(L"https://example.com:443/base"));
    CHECK(valid_url(L"http://127.0.0.1:5144"));
    CHECK(!valid_url(L"https://user:pass@example.com"));
    CHECK(!valid_url(L"https://example.com?q=1"));
    CHECK(!valid_url(L"https://example.com#fragment"));
    CHECK(!valid_url(L"file:///tmp/server"));
    CHECK(!valid_url(L"http://bad host"));
    module=GetModuleHandleW(NULL);
    HWND game=CreateWindowExW(0,L"STATIC",L"Menu test host",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        0,0,800,600,NULL,NULL,module,NULL);
    CHECK(game);
    ReRiffStartExperimentalMenu(); ReRiffStartExperimentalMenu();
    HWND found=NULL;
    for (int i=0;i<100;i++) {
        MSG msg;
        while (PeekMessageW(&msg,NULL,0,0,PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        found=FindWindowW(MENU_CLASS,NULL);
        if (found && GetDlgItem(found,STATUS_ID)) break;
        Sleep(50);
    }
    CHECK(found && GetDlgItem(found,STATUS_ID));
    /* This executable and its server.ini live only in the test output directory. */
    CHECK(WritePrivateProfileStringW(L"unrelated",L"preserve",L"yes",ini_path));
    SetDlgItemTextW(found,EXTERNAL_ID,L"https://example.com/base");
    SetDlgItemTextW(found,FALLBACK_ID,L"http://127.0.0.1:5144");
    CheckDlgButton(found,ONLY_ID,BST_CHECKED);
    SendMessageW(found,WM_COMMAND,SAVE_ID,0);
    WCHAR value[2048];
    GetDlgItemTextW(found,STATUS_ID,value,2048);
    fwprintf(stderr,L"Save status: %ls\n",value);
    GetPrivateProfileStringW(L"server",L"external_url",L"",value,2048,ini_path);
    CHECK(!wcscmp(value,L"https://example.com/base"));
    GetPrivateProfileStringW(L"server",L"external_only",L"",value,2048,ini_path);
    CHECK(!wcscmp(value,L"true"));
    GetPrivateProfileStringW(L"unrelated",L"preserve",L"",value,2048,ini_path);
    CHECK(!wcscmp(value,L"yes"));
    SetDlgItemTextW(found,EXTERNAL_ID,L"https://user:pass@example.com");
    SendMessageW(found,WM_COMMAND,SAVE_ID,0);
    GetPrivateProfileStringW(L"server",L"external_url",L"",value,2048,ini_path);
    CHECK(!wcscmp(value,L"https://example.com/base"));
    SetDlgItemTextW(found,EXTERNAL_ID,L""); SetDlgItemTextW(found,FALLBACK_ID,L"");
    SendMessageW(found,WM_COMMAND,SAVE_ID,0);
    GetPrivateProfileStringW(L"server",L"external_url",L"not-empty",value,2048,ini_path);
    CHECK(!*value);
    SendMessageW(found,WM_CLOSE,0,0); CHECK(!IsWindowVisible(found));
    DeleteFileW(ini_path); DestroyWindow(game);
    puts("PASS: URL validation, duplicate start, controls, save, unrelated settings, rejection, blank endpoints, close");
    return 0;
}
