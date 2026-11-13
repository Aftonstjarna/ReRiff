#define wWinMain LauncherMainForTest
#include "launcher.c"
#undef wWinMain
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    WCHAR tmp[CAP],file[CAP],logs[CAP];CHECK(GetTempPathW(CAP,tmp));CHECK(GetTempFileNameW(tmp,L"rud",0,folder));CHECK(DeleteFileW(folder)&&CreateDirectoryW(folder,NULL));
    path(ini,L"server.ini");CHECK(WritePrivateProfileStringW(L"diagnostics",L"logging_enabled",L"false",ini));
    wcscpy(update_work,folder);wcscpy(update_failure,L"Could not download the release package. Download exceeded the 64 MiB limit.");
    path(file,L"failure");HANDLE f=CreateFileW(file,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);CHECK(f!=INVALID_HANDLE_VALUE);DWORD n;
    const char sample[]="IOException: Cannot save update: disk full. Podsiad\xc5\x82o\n";CHECK(WriteFile(f,sample,sizeof(sample)-1,&n,NULL));CloseHandle(f);
    WCHAR *out=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,UPDATE_DETAILS_CAP*sizeof(WCHAR));CHECK(out);update_details_text(out);
    CHECK(wcsstr(out,L"64 MiB")&&wcsstr(out,L"disk full")&&wcsstr(out,L"Podsiad\x0142o"));
    path(logs,L"logs");CHECK(GetFileAttributesW(logs)==INVALID_FILE_ATTRIBUTES);
    // Long UTF-16 logs retain the end and stay bounded.
    path(file,L"update.log");f=CreateFileW(file,GENERIC_WRITE,0,NULL,CREATE_NEW,0,NULL);CHECK(f!=INVALID_HANDLE_VALUE);
    WCHAR block[12000];for(unsigned i=0;i<12000;i++)block[i]=L'x';block[0]=0xfeff;
    CHECK(WriteFile(f,block,sizeof(block),&n,NULL));const WCHAR end[]=L"\r\nActual installer failure";CHECK(WriteFile(f,end,sizeof(end)-2,&n,NULL));CloseHandle(f);
    update_details_text(out);CHECK(wcsstr(out,L"last 16 KiB")&&wcsstr(out,L"Actual installer failure")&&wcslen(out)<UPDATE_DETAILS_CAP);
    path(file,L"localserver");CHECK(CreateDirectoryW(file,NULL));update_remember(update_work);
    update_failure[0]=update_work[0]=0;update_details_text(out);CHECK(wcsstr(out,L"Latest retained update attempt")&&wcsstr(out,L"disk full"));
    show_update_details();CHECK(update_details_window);HWND edit=GetDlgItem(update_details_window,1);CHECK(edit&&(GetWindowLongW(edit,GWL_STYLE)&ES_READONLY)&&(GetWindowLongW(edit,GWL_STYLE)&WS_VSCROLL));
    SendMessageW(update_details_window,WM_SIZE,0,MAKELPARAM(900,600));RECT rect;CHECK(GetWindowRect(edit,&rect)&&rect.right-rect.left==876&&rect.bottom-rect.top==576);
    DestroyWindow(update_details_window);CHECK(!update_details_window);HeapFree(GetProcessHeap(),0,out);
    path(file,L"localserver\\update-details.ini");DeleteFileW(file);path(file,L"localserver");CHECK(RemoveDirectoryW(file));
    path(file,L"update.log");DeleteFileW(file);path(file,L"failure");DeleteFileW(file);DeleteFileW(ini);CHECK(RemoveDirectoryW(folder));
    puts("PASS: readable temporary errors, UTF-8/UTF-16 tails, bounded read-only resizable viewer, and no logs created when logging is off.");return 0;
}
