#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

#define CLASS_NAME L"ReRiffServerToast"
#define VISIBLE_MS 6000
static HINSTANCE module;
static volatile LONG active;

typedef struct Toast {
    WCHAR endpoint[2048];
    HWND game;
    HWND window;
    HFONT title_font;
    HFONT body_font;
    DWORD elapsed;
    DWORD wall_start;
} Toast;

static BOOL CALLBACK find_game(HWND window, LPARAM parameter) {
    DWORD process=0; GetWindowThreadProcessId(window,&process);
    if (process!=GetCurrentProcessId() || !IsWindowVisible(window) || IsIconic(window)) return TRUE;
    WCHAR cls[128]; GetClassNameW(window,cls,128);
    if (!wcscmp(cls,CLASS_NAME) || GetWindow(window,GW_OWNER)) return TRUE;
    RECT rect; GetClientRect(window,&rect);
    if (rect.right<480 || rect.bottom<240) return TRUE;
    Toast *toast=(Toast *)parameter;
    if (!toast->game) toast->game=window;
    else {
        RECT old; GetClientRect(toast->game,&old);
        if (rect.right*rect.bottom>old.right*old.bottom) toast->game=window;
    }
    return TRUE;
}

static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    Toast *toast=(Toast *)GetWindowLongPtrW(window,GWLP_USERDATA);
    if (message==WM_NCCREATE) {
        toast=(Toast *)((CREATESTRUCTW *)lp)->lpCreateParams;
        SetWindowLongPtrW(window,GWLP_USERDATA,(LONG_PTR)toast);
    }
    if (message==WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (message==WM_NCHITTEST) return HTTRANSPARENT;
    if (message==WM_ERASEBKGND) return 1;
    if (message==WM_PAINT && toast) {
        PAINTSTRUCT paint; HDC dc=BeginPaint(window,&paint);
        RECT rect; GetClientRect(window,&rect);
        HBRUSH background=CreateSolidBrush(RGB(25,22,36));FillRect(dc,&rect,background);DeleteObject(background);
        RECT stripe={0,0,4,rect.bottom};HBRUSH accent=CreateSolidBrush(RGB(183,107,233));FillRect(dc,&stripe,accent);DeleteObject(accent);
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(230,219,244));
        HGDIOBJ old=SelectObject(dc,toast->title_font);
        RECT title={18,12,rect.right-16,36};DrawTextW(dc,L"ReRiff — Server",-1,&title,DT_SINGLELINE|DT_NOPREFIX);
        SelectObject(dc,toast->body_font);SetTextColor(dc,RGB(200,199,210));
        RECT body={18,41,rect.right-16,rect.bottom-10};
        DrawTextW(dc,toast->endpoint,-1,&body,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        SelectObject(dc,old);EndPaint(window,&paint);return 0;
    }
    if (message==WM_TIMER && toast) {
        if (!IsWindow(toast->game) || GetTickCount()-toast->wall_start>45000 || toast->elapsed>=VISIBLE_MS) {
            DestroyWindow(window);return 0;
        }
        HWND foreground=GetForegroundWindow();
        DWORD process=0;GetWindowThreadProcessId(foreground,&process);
        if (process!=GetCurrentProcessId() || IsIconic(toast->game)) {
            ShowWindow(window,SW_HIDE);return 0;
        }
        RECT client;GetClientRect(toast->game,&client);
        if (client.right<480 || client.bottom<240) {ShowWindow(window,SW_HIDE);return 0;}
        POINT origin={0,0};ClientToScreen(toast->game,&origin);
        int width=420;if (width>client.right-40) width=client.right-40;
        SetWindowPos(window,HWND_TOPMOST,origin.x+client.right-width-24,origin.y+24,width,86,
                     SWP_NOACTIVATE|SWP_SHOWWINDOW);
        toast->elapsed+=100;
        BYTE alpha=toast->elapsed>5200?(BYTE)((VISIBLE_MS-toast->elapsed)*240/800):240;
        SetLayeredWindowAttributes(window,0,alpha,LWA_ALPHA);
        return 0;
    }
    if (message==WM_DESTROY) { KillTimer(window,1);PostQuitMessage(0);return 0; }
    return DefWindowProcW(window,message,wp,lp);
}

static DWORD WINAPI show_toast(LPVOID parameter) {
    Toast *toast=(Toast *)parameter;
    DWORD start=GetTickCount();
    while (!toast->game && GetTickCount()-start<45000) {
        EnumWindows(find_game,(LPARAM)toast);
        if (!toast->game) Sleep(100);
    }
    if (toast->game) {
        // Give the game's own window time to settle before showing anything.
        Sleep(1500);
        WNDCLASSW cls;ZeroMemory(&cls,sizeof(cls));cls.lpfnWndProc=procedure;cls.hInstance=module;cls.lpszClassName=CLASS_NAME;
        RegisterClassW(&cls);
        toast->title_font=CreateFontW(-16,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        toast->body_font=CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        toast->wall_start=GetTickCount();
        toast->window=CreateWindowExW(WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT,
            CLASS_NAME,L"",WS_POPUP|WS_DISABLED,0,0,420,86,toast->game,NULL,module,toast);
        if (toast->window) {
            SetLayeredWindowAttributes(toast->window,0,240,LWA_ALPHA);
            HRGN shape=CreateRoundRectRgn(0,0,421,87,12,12);
            if (!SetWindowRgn(toast->window,shape,FALSE)) DeleteObject(shape);
            SetTimer(toast->window,1,100,NULL);
            MSG msg;while (GetMessageW(&msg,NULL,0,0)>0) {TranslateMessage(&msg);DispatchMessageW(&msg);}
        }
        DeleteObject(toast->title_font);DeleteObject(toast->body_font);
    }
    HeapFree(GetProcessHeap(),0,toast);InterlockedExchange(&active,0);return 0;
}

__declspec(dllexport) void __cdecl ReRiffShowServerToastW(const WCHAR *endpoint) {
    if (InterlockedCompareExchange(&active,1,0)) return;
    Toast *toast=(Toast *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Toast));
    if (!toast) {InterlockedExchange(&active,0);return;}
    lstrcpynW(toast->endpoint,endpoint?endpoint:L"Networking disabled",2048);
    HANDLE thread=CreateThread(NULL,0,show_toast,toast,0,NULL);
    if (thread) CloseHandle(thread);
    else {HeapFree(GetProcessHeap(),0,toast);InterlockedExchange(&active,0);}
}

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    (void)reserved;
    if (reason==DLL_PROCESS_ATTACH) {module=instance;DisableThreadLibraryCalls(instance);}
    return TRUE;
}
