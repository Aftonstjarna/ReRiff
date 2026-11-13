#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
int main(void){
    GetModuleFileNameW(NULL,folder,CAP);WCHAR *slash=wcsrchr(folder,L'\\');if(!slash)return 1;*slash=0;path(ini,L"language-test.ini");
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ReRiffLanguageTest";RegisterClassW(&cls);
    busy=TRUE;HWND host=CreateWindowW(cls.lpszClassName,L"Language test",WS_OVERLAPPEDWINDOW,0,0,660,615,NULL,NULL,cls.hInstance,NULL);if(!host)return 2;
    if(!launcher_tabs||!language_checkbox||!latin_checkbox||SendMessageW(launcher_tabs,TCM_GETITEMCOUNT,0,0)!=3)return 3;
    ShowWindow(host,SW_SHOW);SendMessageW(launcher_tabs,TCM_SETCURSEL,1,0);NMHDR event={launcher_tabs,0,TCN_SELCHANGE};SendMessageW(host,WM_NOTIFY,0,(LPARAM)&event);
    if(!IsWindowVisible(language_checkbox)||!IsWindowVisible(latin_checkbox)||IsWindowVisible(external)||!IsWindowVisible(play_button))return 4;
    SetWindowTextW(external,L"https://unsaved.example");changing_language=TRUE;
    WritePrivateProfileStringW(L"languages",L"cjk_enabled",L"true",ini);SendMessageW(host,WM_TIMER,1,0);WCHAR text[128];GetWindowTextW(external,text,128);if(wcscmp(text,L"https://unsaved.example"))return 7;
    WritePrivateProfileStringW(L"languages",L"latin_enabled",L"true",ini);
    SendMessageW(host,WM_COMPLETE,TRUE,0);language_controls();if(SendMessageW(language_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED||SendMessageW(latin_checkbox,BM_GETCHECK,0,0)!=BST_CHECKED)return 5;
    SendMessageW(launcher_tabs,TCM_SETCURSEL,0,0);SendMessageW(host,WM_NOTIFY,0,(LPARAM)&event);
    if(IsWindowVisible(language_checkbox)||IsWindowVisible(latin_checkbox)||!IsWindowVisible(external))return 6;
    DeleteFileW(ini);DestroyWindow(host);puts("PASS: native Languages tab, CJK checkbox, saved setting and page visibility.");return 0;
}
