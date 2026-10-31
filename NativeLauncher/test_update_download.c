/* Exercise official GitHub download/checksum/preflight, aborting before install
 * because this disposable fixture deliberately has no native updater helper. */
#define wWinMain reriff_launcher_entry
#include "launcher.c"
#undef wWinMain
static DWORD WINAPI test_update(LPVOID unused) {
    (void)unused;
    BOOL result=check_updates();WCHAR marker[CAP];
    WCHAR *details=wcsstr(message,L"Details: ");
    if(result||update_handoff||!details){wprintf(L"%ls\n",message);return 1;}
    details+=9;_snwprintf(marker,CAP,L"%ls\\prepared.json",details);marker[CAP-1]=0;
    if(GetFileAttributesW(marker)==INVALID_FILE_ATTRIBUTES){wprintf(L"%ls\n",message);return 2;}
    puts("PASS: official GitHub release download, SHA256 verification, installer preflight; stopped before installation");return 0;
}
int wmain(int count,WCHAR **args) {
    if(count!=2)return 3;
    wcsncpy(folder,args[1],CAP-1);folder[CAP-1]=0;path(ini,L"server.ini");
    HANDLE thread=CreateThread(NULL,4*1024*1024,test_update,NULL,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if(!thread)return 4;
    WaitForSingleObject(thread,INFINITE);DWORD result=5;GetExitCodeThread(thread,&result);CloseHandle(thread);return (int)result;
}
