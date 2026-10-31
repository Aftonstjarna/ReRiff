#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
int main(void) {
    WCHAR file[MAX_PATH];DWORD length=GetModuleFileNameW(NULL,file,MAX_PATH);
    if(!length || length+8>=MAX_PATH)return 1;
    wcscat(file,L".played");FILE *out=_wfopen(file,L"wb");
    if(!out)return 2;
    fputs("play invoked",out);fclose(out);return 0;
}
