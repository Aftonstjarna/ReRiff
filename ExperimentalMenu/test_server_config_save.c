#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(void){
    WCHAR root[MAX_PATH],folder[MAX_PATH],client[MAX_PATH],temp[MAX_PATH],value[64];
    CHECK(GetTempPathW(MAX_PATH,root));CHECK(GetTempFileNameW(root,L"rrs",0,folder));CHECK(DeleteFileW(folder));CHECK(CreateDirectoryW(folder,NULL));
    _snwprintf(config_path,MAX_PATH,L"%ls\\server.ini",folder);_snwprintf(client,MAX_PATH,L"%ls\\client.ini",folder);_snwprintf(temp,MAX_PATH,L"%ls.tmp",client);
    CHECK(WritePrivateProfileStringW(L"server",L"external_url",L"https://example.com",config_path));
    CHECK(WritePrivateProfileStringW(L"launcher",L"skip_launcher",L"false",config_path));
    CHECK(WritePrivateProfileStringW(L"graphics",L"ui_scale",L"55",config_path));
    CHECK(WritePrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"true",config_path));
    load_settings();CHECK(!pending.skip_launcher);pending.skip_launcher=TRUE;pending.only=TRUE;CHECK(save_settings());
    load_settings();CHECK(pending.skip_launcher&&pending.only);
    GetPrivateProfileStringW(L"graphics",L"ui_scale",L"",value,64,config_path);CHECK(!wcscmp(value,L"55"));
    GetPrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"",value,64,config_path);CHECK(!wcscmp(value,L"true"));
    pending.skip_launcher=FALSE;pending.only=FALSE;CHECK(CreateDirectoryW(temp,NULL));CHECK(!save_settings());CHECK(RemoveDirectoryW(temp));
    load_settings();CHECK(pending.skip_launcher&&pending.only);
    pending.skip_launcher=FALSE;pending.only=FALSE;
    HANDLE lock=CreateFileW(config_path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(lock!=INVALID_HANDLE_VALUE);
    CHECK(!save_settings());CloseHandle(lock);load_settings();CHECK(pending.skip_launcher&&pending.only);
    pending.skip_launcher=FALSE;pending.only=FALSE;CHECK(save_settings());load_settings();CHECK(!pending.skip_launcher&&!pending.only);
    DeleteFileW(client);DeleteFileW(config_path);RemoveDirectoryW(folder);
    puts("PASS: native Servers save writes split client/server owners, reloads Skip launcher, preserves other preferences, rejects staging failures and rolls back partial commit");return 0;
}
