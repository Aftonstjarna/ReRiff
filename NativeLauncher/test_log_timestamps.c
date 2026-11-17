#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "logging.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
 WCHAR temp[MAX_PATH],name[MAX_PATH];CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"rrt",0,name));
 rr_log_append(name,"test","first\r\nUTF-8: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\n  continuation\n");
 FILE *file=_wfopen(name,L"rb");CHECK(file);char line[256];unsigned count=0;
 const char *expected[]={"[test] first\r\n","[test] UTF-8: \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\r\n","[test]   continuation\r\n"};
 CHECK(fgets(line,sizeof(line),file));CHECK(strstr(line,"[diagnostic]")!=NULL);while(!strchr(line,'\n'))CHECK(fgets(line,sizeof(line),file));
 while(fgets(line,sizeof(line),file)){CHECK(count<3);CHECK(strlen(line)>27);CHECK(line[0]=='['&&line[11]=='T'&&line[24]=='Z'&&line[25]==']'&&line[26]==' ');CHECK(!strcmp(line+27,expected[count++]));}
 fclose(file);CHECK(count==3);CHECK(DeleteFileW(name));
 CHECK(CreateDirectoryW(name,NULL));WCHAR local[MAX_PATH],context[MAX_PATH],target[MAX_PATH];
 _snwprintf(local,MAX_PATH,L"%ls\\localserver",name);CHECK(CreateDirectoryW(local,NULL));
 _snwprintf(context,MAX_PATH,L"%ls\\diagnostic-context.ini",local);
 CHECK(WritePrivateProfileStringW(L"diagnostic",L"player",L"Same name",context));
 CHECK(WritePrivateProfileStringW(L"diagnostic",L"account_id",L"22222222-2222-2222-2222-222222222222",context));
 CHECK(WritePrivateProfileStringW(L"diagnostic",L"server_id",L"11111111-1111-1111-1111-111111111111",context));
 CHECK(WritePrivateProfileStringW(L"diagnostic",L"token",L"DO-NOT-LOG",context));
 _snwprintf(target,MAX_PATH,L"%ls\\test.log",name);char first[3072],second[3072];
 rr_diagnostic_header(target,first,sizeof(first));rr_diagnostic_header(target,second,sizeof(second));CHECK(!strcmp(first,second));
 CHECK(strstr(first,"player=Same name")&&strstr(first,"account_id=22222222-2222-2222-2222-222222222222"));
 CHECK(!strstr(first,"DO-NOT-LOG")&&!strstr(first,"client_id=unavailable"));
 CHECK(WritePrivateProfileStringW(L"diagnostic",L"account_id",L"33333333-3333-3333-3333-333333333333",context));
 rr_diagnostic_header(target,second,sizeof(second));CHECK(strcmp(first,second));CHECK(strstr(second,"account_id=33333333-3333-3333-3333-333333333333"));
 CHECK(DeleteFileW(context));_snwprintf(context,MAX_PATH,L"%ls\\diagnostic-client-id.txt",local);CHECK(DeleteFileW(context));CHECK(RemoveDirectoryW(local));CHECK(RemoveDirectoryW(name));
puts("PASS UTC millisecond timestamps on every native UTF-8/multiline log line");return 0;
}
