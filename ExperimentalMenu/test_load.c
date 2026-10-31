#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

static void pump(void) {
    MSG message;
    while (PeekMessageW(&message,NULL,0,0,PM_REMOVE)) {
        TranslateMessage(&message); DispatchMessageW(&message);
    }
}

static void f8(void) {
    INPUT input; ZeroMemory(&input,sizeof(input));
    input.type=INPUT_KEYBOARD; input.ki.wVk=VK_F8;
    SendInput(1,&input,sizeof(input));
    for (int i=0;i<10;i++) { pump(); Sleep(20); }
    input.ki.dwFlags=KEYEVENTF_KEYUP; SendInput(1,&input,sizeof(input));
    for (int i=0;i<10;i++) { pump(); Sleep(20); }
}

int main(void) {
    HWND host=CreateWindowExW(0,L"STATIC",L"ReRiff DLL load test",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        0,0,800,600,NULL,NULL,GetModuleHandleW(NULL),NULL);
    HMODULE dll=LoadLibraryW(L"ReRiff.Notify.dll");
    if (!host || !dll) { fprintf(stderr,"Load failure %lu\n",GetLastError()); return 1; }
    typedef void (__cdecl *ToastFn)(const WCHAR *);
    ToastFn toast=(ToastFn)(void *)GetProcAddress(dll,"ReRiffShowServerToastW");
    if (!toast) { fputs("Missing client-compatible export\n",stderr); return 2; }
    toast(L"Experimental menu test");
    HWND panel=NULL;
    for (int i=0;i<100;i++) {
        pump(); Sleep(50);
        panel=FindWindowW(L"ReRiffExperimentalMenu",NULL);
        if (panel && GetDlgItem(panel,105)) break;
    }
    if (!panel) { fputs("DLL did not create panel\n",stderr); return 3; }
    SetForegroundWindow(host); f8();
    if (!IsWindowVisible(panel)) { fputs("F8 did not open panel\n",stderr); return 4; }
    SetForegroundWindow(panel); f8();
    if (IsWindowVisible(panel)) { fputs("F8 did not close panel\n",stderr); return 5; }
    puts("PASS: real DLL load, existing toast export, F8 open and close");
    /* DLL remains loaded for process lifetime, as with the managed P/Invoke. */
    DestroyWindow(host); return 0;
}
