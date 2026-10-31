#define wWinMain launcher_main
#include "launcher.c"
#undef wWinMain
#include <assert.h>
int main(void) {
    GetCurrentDirectoryW(CAP,folder);path(ini,L"server.ini");
    WCHAR marker[CAP],title[256];path(marker,L"localserver\\client-channel.ini");
    CreateDirectoryW(L"localserver",NULL);
    WritePrivateProfileStringW(L"client",L"channel",L"experimental",marker);
    load_channel();assert(installed_experimental && requested_experimental);
    WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ChannelTest";
    RegisterClassW(&cls);busy=TRUE;
    HWND hwnd=CreateWindowW(cls.lpszClassName,L"",WS_OVERLAPPED,0,0,660,545,NULL,NULL,cls.hInstance,NULL);
    assert(hwnd);GetWindowTextW(hwnd,title,256);assert(wcsstr(title,L"Experimental"));
    SendMessageW(experimental_checkbox,BM_SETCHECK,BST_UNCHECKED,0);
    SendMessageW(hwnd,WM_COMMAND,EXPERIMENTAL,0);assert(!requested_experimental && installed_experimental);
    wcscpy(message,L"Simulated unavailable release");SendMessageW(hwnd,WM_COMPLETE,0,0);
    assert(requested_experimental && SendMessageW(experimental_checkbox,BM_GETCHECK,0,0)==BST_CHECKED);
    WritePrivateProfileStringW(L"client",L"channel",L"stable",marker);load_channel();channel_title();
    GetWindowTextW(hwnd,title,256);assert(!wcsstr(title,L"Experimental"));
    assert(!installed_experimental && !requested_experimental);
    DestroyWindow(hwnd);DeleteFileW(marker);RemoveDirectoryW(L"localserver");
    puts("Channel title and failed-switch rollback passed");return 0;
}
