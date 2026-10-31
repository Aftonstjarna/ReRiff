#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain

int main(void) {
    GetModuleFileNameW(NULL,folder,CAP);WCHAR *slash=wcsrchr(folder,L'\\');if(!slash)return 1;*slash=0;
    path(ini,L"server.ini");
    WritePrivateProfileStringW(L"server",L"external_url",L"https://old.example",ini);
    WritePrivateProfileStringW(L"server",L"external_only",L"true",ini);
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ReRiffPersistenceTest";
    RegisterClassW(&cls);
    busy=TRUE;
    HWND host=CreateWindowW(cls.lpszClassName,L"Persistence test",WS_OVERLAPPEDWINDOW,0,0,650,550,NULL,NULL,cls.hInstance,NULL);
    if(!host)return 2;
    WCHAR temporary[CAP];_snwprintf(temporary,CAP,L"%ls.new",ini);temporary[CAP-1]=0;
    WritePrivateProfileStringW(L"server",L"external_url",L"",temporary);
    WritePrivateProfileStringW(L"server",L"fallback_url",L"http://127.0.0.1:5144",temporary);
    WritePrivateProfileStringW(L"server",L"external_only",L"false",temporary);
    WritePrivateProfileStringW(NULL,NULL,NULL,temporary);
    if(!MoveFileExW(temporary,ini,MOVEFILE_REPLACE_EXISTING))return 3;
    SendMessageW(host,WM_COMPLETE,TRUE,1);
    WCHAR value[4096];GetWindowTextW(external,value,4096);if(*value)return 4;
    if(SendMessageW(only_checkbox,BM_GETCHECK,0,0)!=BST_UNCHECKED)return 5;
    /* Keep the worker disabled: this checks dispatch without launching a game. */
    busy=TRUE;
    SetWindowTextW(external,L"https://unsaved.example");
    SendMessageW(host,WM_COMMAND,PLAY,0);
    GetPrivateProfileStringW(L"server",L"external_url",L"missing",value,4096,ini);if(*value)return 9;
    SetWindowTextW(external,L"");
    busy=FALSE;
    if(!save())return 6;
    if(!needs_local_server())return 10;
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"false",ini);
    if(needs_local_server())return 12;
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"true",ini);
    WritePrivateProfileStringW(L"server",L"fallback_url",L"",ini);
    if(needs_local_server())return 11;
    WritePrivateProfileStringW(L"server",L"fallback_url",L"http://127.0.0.1:5144",ini);
    GetPrivateProfileStringW(L"server",L"external_only",L"",value,4096,ini);if(wcscmp(value,L"false"))return 7;
    GetPrivateProfileStringW(L"server",L"external_url",L"missing",value,4096,ini);if(*value)return 8;
    DeleteFileW(ini);
    puts("PASS: game-close reload preserves atomically saved game settings across launcher Save/Play");
    return 0;
}
