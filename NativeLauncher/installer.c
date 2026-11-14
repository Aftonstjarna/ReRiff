#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#ifndef RERIFF_VERSION
#define RERIFF_VERSION "development"
#endif
#define CAP 32768
#include "python_runtime.h"
static HWND setup_window, path_edit, status_text, progress_bar, install_button;
static HWND options[5];
static BOOL installing=FALSE, completed=FALSE;
static WCHAR setup_arguments[CAP], selected_game[CAP];
static int WINAPI setup_prompt(HWND owner,LPCWSTR text,LPCWSTR title,UINT flags){
    (void)owner;(void)title;
    if((flags&MB_YESNO)==MB_YESNO)return SendMessageW(options[3],BM_GETCHECK,0,0)==BST_CHECKED?IDYES:IDNO; /* Consent was collected before starting. */
    SetWindowTextW(status_text,text);return IDOK;
}
static int install(LPWSTR arguments) {
    WCHAR root[CAP],command[CAP],game[CAP]=L"",log[CAP];
    GetModuleFileNameW(NULL,root,CAP);WCHAR *slash=wcsrchr(root,L'\\');if(!slash)return 1;*slash=0;
    BOOL logging=FALSE;

    {
        int count=0;WCHAR **args=CommandLineToArgvW(arguments,&count);
        if(args){for(int i=1;i+1<count;i++)if(!wcscmp(args[i],L"--game-dir")){wcsncpy(game,args[i+1],CAP-1);game[CAP-1]=0;break;}LocalFree(args);}
    }
    if(!*game)return 1;
    _snwprintf(command,CAP,L"%ls\\Game.exe",game);command[CAP-1]=0;
    if(GetFileAttributesW(command)==INVALID_FILE_ATTRIBUTES)return 1;
    {logging=rr_logging_enabled(game,strstr(RERIFF_VERSION,"-")!=NULL);if(wcsstr(arguments,L"--no-logging"))logging=FALSE;else if(wcsstr(arguments,L"--logging"))logging=TRUE;}
    python_logging_override=logging;
    if(setup_window)PostMessageW(setup_window,WM_APP+1,0,0);
    if(!ensure_private_python(NULL,game,setup_window!=NULL))return 1;
    if(setup_window)PostMessageW(setup_window,WM_APP+1,1,0);
    const WCHAR *bootstrap=L"import runpy,sys;sys.path.insert(0,sys.argv[1]);sys.argv=sys.argv[2:];runpy.run_path(sys.argv[0],run_name='__main__')";
    /* The full command line is parsed above; forward only its option tail. */
    if(*arguments==L'"'){arguments++;while(*arguments&&*arguments!=L'"')arguments++;if(*arguments)arguments++;}
    else while(*arguments&&*arguments!=L' '&&*arguments!=L'\t')arguments++;
    _snwprintf(command,CAP,L"\"%ls\\localserver\\windows-python\\python.exe\" -X utf8 -c \"%ls\" \"%ls\" \"%ls\\installer.py\" --platform windows %ls",game,bootstrap,root,root,arguments);
    command[CAP-1]=0;
    _snwprintf(log,CAP,L"%ls\\logs",*game?game:root);log[CAP-1]=0;if(logging)CreateDirectoryW(log,NULL);
    _snwprintf(log,CAP,L"%ls\\logs\\installer.log",*game?game:root);log[CAP-1]=0;
    SECURITY_ATTRIBUTES security={sizeof(security),NULL,TRUE};
    HANDLE output=CreateFileW(logging?log:L"NUL",GENERIC_WRITE,FILE_SHARE_READ,&security,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    STARTUPINFOW start;PROCESS_INFORMATION process;ZeroMemory(&start,sizeof(start));ZeroMemory(&process,sizeof(process));start.cb=sizeof(start);
    start.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;start.wShowWindow=SW_HIDE;
    start.hStdOutput=output;start.hStdError=output;start.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
    if(output==INVALID_HANDLE_VALUE || !CreateProcessW(NULL,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,root,&start,&process)){
        if(output!=INVALID_HANDLE_VALUE)CloseHandle(output);
        return 1;
    }
    CloseHandle(output);CloseHandle(process.hThread);WaitForSingleObject(process.hProcess,INFINITE);
    DWORD code=1;GetExitCodeProcess(process.hProcess,&code);CloseHandle(process.hProcess);
    return (int)code;
}

static DWORD WINAPI install_worker(void *unused){
    (void)unused;
    int result=install(setup_arguments);
    PostMessageW(setup_window,WM_APP+2,(WPARAM)result,0);
    return 0;
}
static HWND control(HWND parent,LPCWSTR kind,LPCWSTR text,DWORD style,int x,int y,int w,int h,int id){
    HWND item=CreateWindowExW(!wcscmp(kind,L"EDIT")?WS_EX_CLIENTEDGE:0,kind,text,
        WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    SendMessageW(item,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return item;
}
static void load_options(void){
    WCHAR config[CAP],value[16];GetWindowTextW(path_edit,selected_game,CAP);
    _snwprintf(config,CAP,L"%ls\\server.ini",selected_game);
    GetPrivateProfileStringW(L"languages",L"cjk_enabled",GetPrivateProfileIntW(L"diagnostics",L"cjk_render_probe",0,config)?L"true":L"false",value,16,config);
    SendMessageW(options[0],BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"languages",L"latin_enabled",L"false",value,16,config);
    SendMessageW(options[1],BM_SETCHECK,(!_wcsicmp(value,L"true")||!wcscmp(value,L"1"))?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(options[2],BM_SETCHECK,rr_logging_enabled(selected_game,strstr(RERIFF_VERSION,"-")!=NULL||!strcmp(RERIFF_VERSION,"development"))?BST_CHECKED:BST_UNCHECKED,0);
}
static LRESULT CALLBACK setup_proc(HWND window,UINT message,WPARAM wparam,LPARAM lparam){
    switch(message){
    case WM_COMMAND:
        if(LOWORD(wparam)==12&&HIWORD(wparam)==EN_KILLFOCUS&&!installing){load_options();break;}
        if(LOWORD(wparam)==10&&!installing){
            WCHAR file[CAP]=L"";OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
            dialog.lpstrFilter=L"Riff Racer (Game.exe)\0Game.exe\0";dialog.lpstrFile=file;dialog.nMaxFile=CAP;
            dialog.lpstrTitle=L"Choose Riff Racer's Game.exe";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
            if(GetOpenFileNameW(&dialog)){WCHAR *end=wcsrchr(file,L'\\');if(end)*end=0;SetWindowTextW(path_edit,file);load_options();}
        }else if(LOWORD(wparam)==11){
            if(completed){DestroyWindow(window);break;}if(installing)break;
            WCHAR executable[CAP];GetWindowTextW(path_edit,selected_game,CAP);
            if(wcschr(selected_game,L'"')||_snwprintf(executable,CAP,L"%ls\\Game.exe",selected_game)<0||GetFileAttributesW(executable)==INVALID_FILE_ATTRIBUTES){
                SetWindowTextW(status_text,L"Choose the game folder containing Game.exe.");break;}
            WCHAR runtime[CAP];_snwprintf(runtime,CAP,L"%ls\\localserver\\windows-python",selected_game);
            if(!private_python_ready(runtime)&&SendMessageW(options[3],BM_GETCHECK,0,0)!=BST_CHECKED){
                SetWindowTextW(status_text,L"Private Python is required. Check its download option to continue.");break;}
            _snwprintf(setup_arguments,CAP,L"setup --game-dir \"%ls\" %ls %ls %ls",selected_game,
                SendMessageW(options[0],BM_GETCHECK,0,0)==BST_CHECKED?L"--cjk":L"--no-cjk",
                SendMessageW(options[1],BM_GETCHECK,0,0)==BST_CHECKED?L"--latin":L"--no-latin",
                SendMessageW(options[2],BM_GETCHECK,0,0)==BST_CHECKED?L"--logging":L"--no-logging");
            installing=TRUE;EnableWindow(path_edit,FALSE);EnableWindow(GetDlgItem(window,10),FALSE);EnableWindow(install_button,FALSE);
            for(int i=0;i<5;i++)EnableWindow(options[i],FALSE);
            SetWindowLongPtrW(progress_bar,GWL_STYLE,GetWindowLongPtrW(progress_bar,GWL_STYLE)|PBS_MARQUEE);
            SetWindowTextW(status_text,L"Preparing installation. Please wait...");
            SendMessageW(progress_bar,PBM_SETMARQUEE,TRUE,30);
            HANDLE worker=CreateThread(NULL,4*1024*1024,install_worker,NULL,0,NULL);
            if(worker)CloseHandle(worker);else PostMessageW(window,WM_APP+2,1,0);
        }break;
    case WM_APP+1:
        SetWindowTextW(status_text,wparam?L"Installing ReRiff and selected language packs. Please wait...":L"Checking, downloading and verifying private Python. Please wait...");break;
    case WM_APP+2:
        installing=FALSE;completed=!wparam;SendMessageW(progress_bar,PBM_SETMARQUEE,FALSE,0);
        SetWindowLongPtrW(progress_bar,GWL_STYLE,GetWindowLongPtrW(progress_bar,GWL_STYLE)&~PBS_MARQUEE);
        SendMessageW(progress_bar,PBM_SETPOS,wparam?0:100,0);
        SetWindowTextW(status_text,wparam?L"Installation failed. Check your connection and disk space. If logging is enabled, see logs in the game folder. Existing saves are retained.":L"ReRiff installed. Start ReRiff-Launcher.exe beside Game.exe. Your settings and saved server data were preserved.");
        SetWindowTextW(install_button,wparam?L"Retry":L"Close");EnableWindow(install_button,TRUE);
        if(wparam){EnableWindow(path_edit,TRUE);EnableWindow(GetDlgItem(window,10),TRUE);for(int i=0;i<5;i++)EnableWindow(options[i],TRUE);}
        if(!wparam&&SendMessageW(options[4],BM_GETCHECK,0,0)==BST_CHECKED)ShellExecuteW(window,L"open",L"https://github.com/Aftonstjarna/ReRiff",NULL,NULL,SW_SHOWNORMAL);
        break;
    case WM_CLOSE:if(!installing)DestroyWindow(window);break;
    case WM_DESTROY:PostQuitMessage(0);break;
    default:return DefWindowProcW(window,message,wparam,lparam);
    }return 0;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,LPWSTR arguments,int show){
    (void)previous;
    if(*arguments)return install(GetCommandLineW());
    INITCOMMONCONTROLSEX common={sizeof(common),ICC_PROGRESS_CLASS};InitCommonControlsEx(&common);
    WNDCLASSW cls={0};cls.lpfnWndProc=setup_proc;cls.hInstance=instance;cls.lpszClassName=L"ReRiffSetup";
    cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_BTNFACE+1);RegisterClassW(&cls);
    RECT bounds={0,0,620,470};AdjustWindowRectEx(&bounds,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,WS_EX_CONTROLPARENT);
    setup_window=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"ReRiff setup",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,NULL,NULL,instance,NULL);
    control(setup_window,L"STATIC",L"Close Riff Racer before installing. Select its game folder:",0,20,18,580,24,0);
    path_edit=control(setup_window,L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,20,47,470,26,12);
    control(setup_window,L"BUTTON",L"Browse...",WS_TABSTOP,500,47,100,26,10);
    const WCHAR *labels[]={L"Enable Chinese, Japanese and Korean fonts (about 105 MiB)",L"Enable Latin/European fonts, including Polish (small download)",L"Enable diagnostic logging",L"Download verified private Python from python.org if required",L"Open the GitHub repository after installation"};
    for(int i=0;i<5;i++)options[i]=control(setup_window,L"BUTTON",labels[i],WS_TABSTOP|BS_AUTOCHECKBOX,20,90+i*34,580,28,20+i);
    control(setup_window,L"STATIC",L"Fonts do not translate menus. Python stays in the game folder; system Python, PATH and registry are unchanged. Existing settings and saves are retained.",0,20,266,580,50,0);
    progress_bar=control(setup_window,PROGRESS_CLASSW,L"",PBS_MARQUEE,20,330,580,22,0);
    status_text=control(setup_window,L"STATIC",L"Choose your options, then select Install.",0,20,365,580,55,0);
    install_button=control(setup_window,L"BUTTON",L"Install",WS_TABSTOP|BS_DEFPUSHBUTTON,490,429,110,28,11);
    if(!setup_window)return 1;
    load_options();python_runtime_prompt=setup_prompt;
    ShowWindow(setup_window,show);MSG message;
    while(GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(setup_window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    return 0;
}
