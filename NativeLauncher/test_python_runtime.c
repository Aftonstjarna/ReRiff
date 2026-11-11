#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CAP 32768
#include "python_runtime.h"
#define CHECK(c) do{if(!(c)){printf("FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned prompts;
static int WINAPI decline(HWND owner,LPCWSTR text,LPCWSTR title,UINT flags){(void)owner;(void)title;if(!(flags&MB_YESNO)||!wcsstr(text,L"private Python"))abort();prompts++;return IDNO;}
int main(int argc,char **argv){
    WCHAR temporary[CAP],root[CAP],escaped[64];CHECK(GetTempPathW(CAP,temporary));CHECK(GetTempFileNameW(temporary,L"rrp",0,root));CHECK(DeleteFileW(root)&&CreateDirectoryW(root,NULL));
    python_runtime_prompt=decline;
    CHECK(!ensure_private_python(NULL,root,FALSE)&&prompts==0);
    CHECK(!ensure_private_python(NULL,root,TRUE)&&prompts==1);
    WCHAR local[CAP];_snwprintf(local,CAP,L"%ls\\localserver",root);CHECK(GetFileAttributesW(local)==INVALID_FILE_ATTRIBUTES);
    CHECK(python_ps_literal(L"C:\\test's folder",escaped,64)&&!wcscmp(escaped,L"C:\\test''s folder"));
    CHECK(!python_ps_literal(L"long",escaped,2));
    if(argc==2){WCHAR runtime[CAP];CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,runtime,CAP));CHECK(private_python_ready(runtime));}
    CHECK(RemoveDirectoryW(root));puts("PASS: Python consent, headless refusal, no files on decline, PowerShell path quoting and optional pinned-runtime verification");return 0;
}
