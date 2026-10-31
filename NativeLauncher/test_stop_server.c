#define wWinMain reriff_launcher_entry
#include "launcher.c"
#undef wWinMain
static DWORD WINAPI check_stop(LPVOID unused) {
    (void)unused;
    if(!stop_server()){wprintf(L"%ls\n",message);return 1;}
    if(!stop_server()){wprintf(L"%ls\n",message);return 2;}
    puts("PASS: owned local server stopped; stopping an already stopped server succeeds");return 0;
}
int wmain(int count,WCHAR **args) {
    if(count!=2)return 3;
    wcsncpy(folder,args[1],CAP-1);folder[CAP-1]=0;
    HANDLE thread=CreateThread(NULL,4*1024*1024,check_stop,NULL,STACK_SIZE_PARAM_IS_A_RESERVATION,NULL);
    if(!thread)return 4;
    WaitForSingleObject(thread,INFINITE);DWORD code=5;GetExitCodeThread(thread,&code);CloseHandle(thread);return (int)code;
}
