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
    WCHAR caption[256];
    const WCHAR *labels[]={L"Install local server",L"Start local server",L"Stop local server"};
    for(int state=0;state<3;state++){
        local_server_state=state;local_server_controls();GetWindowTextW(stop_button,caption,256);
        if(wcscmp(caption,labels[state]))return 23;
    }
    GetWindowTextW(local_server_hint,caption,256);
    if(!wcsstr(caption,L"Secondary server"))return 24;
    local_server_state=0;local_server_controls();
    WCHAR temporary[CAP];_snwprintf(temporary,CAP,L"%ls.new",ini);temporary[CAP-1]=0;
    WritePrivateProfileStringW(L"server",L"external_url",L"",temporary);
    WritePrivateProfileStringW(L"server",L"fallback_url",L"http://127.0.0.1:5144",temporary);
    WritePrivateProfileStringW(L"server",L"external_only",L"false",temporary);
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"false",temporary);
    WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"true",temporary);
    WritePrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"true",temporary);
    WritePrivateProfileStringW(NULL,NULL,NULL,temporary);
    if(!MoveFileExW(temporary,ini,MOVEFILE_REPLACE_EXISTING))return 3;
    /* The game is still running: polling must notice the atomic replacement. */
    SendMessageW(host,WM_TIMER,1,0);
    WCHAR value[4096];GetWindowTextW(external,value,4096);if(*value)return 4;
    if(SendMessageW(only_checkbox,BM_GETCHECK,0,0)!=BST_UNCHECKED)return 5;
    if(SendMessageW(networking_checkbox,BM_GETCHECK,0,0)!=BST_UNCHECKED)return 13;
    GetWindowTextW(fallback,value,4096);if(wcscmp(value,L"http://127.0.0.1:5144"))return 14;
    if(IsWindowEnabled(external)||IsWindowEnabled(fallback))return 15;
    if(SendMessageW(skip_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED||SendMessageW(auto_update_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED)return 21;
    /* No file change must leave an unsaved launcher edit alone. */
    SetWindowTextW(external,L"https://unsaved.example");
    SendMessageW(host,WM_TIMER,1,0);
    GetWindowTextW(external,value,4096);if(wcscmp(value,L"https://unsaved.example"))return 16;
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"true",ini);
    SendMessageW(host,WM_ACTIVATE,WA_ACTIVE,0);
    if(SendMessageW(networking_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED||!IsWindowEnabled(fallback))return 17;
    SendMessageW(host,WM_COMPLETE,TRUE,1);
    /* Keep the worker disabled: this checks dispatch without launching a game. */
    busy=TRUE;
    SetWindowTextW(external,L"https://unsaved.example");
    SendMessageW(host,WM_COMMAND,PLAY,0);
    GetPrivateProfileStringW(L"server",L"external_url",L"missing",value,4096,ini);if(*value)return 9;
    SetWindowTextW(external,L"");
    busy=FALSE;
    SendMessageW(skip_checkbox,BM_SETCHECK,BST_UNCHECKED,0);
    if(!save())return 6;
    if(launcher_preference(L"skip_launcher")||!launcher_preference(L"auto_update_on_skip"))return 22;
    SendMessageW(networking_checkbox,BM_SETCHECK,BST_UNCHECKED,0);
    if(!save())return 18;
    GetPrivateProfileStringW(L"server",L"networking_enabled",L"missing",value,4096,ini);if(wcscmp(value,L"false"))return 19;
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",value,4096,ini);if(wcscmp(value,L"http://127.0.0.1:5144"))return 20;
    /* The Singleplayer button changes mode without clearing either address. */
    SetWindowTextW(external,L"https://primary.example");
    SetWindowTextW(fallback,L"https://secondary.example");
    SendMessageW(host,WM_COMMAND,LOCAL,0);
    if(SendMessageW(networking_checkbox,BM_GETCHECK,0,0)!=BST_UNCHECKED)return 23;
    GetWindowTextW(external,value,4096);if(wcscmp(value,L"https://primary.example"))return 24;
    GetWindowTextW(fallback,value,4096);if(wcscmp(value,L"https://secondary.example"))return 25;
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"false",ini);
    WritePrivateProfileStringW(L"server",L"networking_enabled",L"true",ini);
    WritePrivateProfileStringW(L"server",L"fallback_url",L"",ini);
    WritePrivateProfileStringW(L"server",L"fallback_url",L"http://127.0.0.1:5144",ini);
    GetPrivateProfileStringW(L"server",L"external_only",L"",value,4096,ini);if(wcscmp(value,L"false"))return 7;
    GetPrivateProfileStringW(L"server",L"external_url",L"missing",value,4096,ini);if(*value)return 8;
    DeleteFileW(ini);
    puts("PASS: live/focus/game-close reload, networking state, retained addresses, unsaved edits and separate Save/Play");
    return 0;
}
