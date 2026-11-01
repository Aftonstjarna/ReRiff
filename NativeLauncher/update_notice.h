/* Persist only a completed unattended update; the next main menu consumes it. */
static BOOL record_update_notice(const WCHAR *game){
    WCHAR name[CAP],version[64];char bytes[64];DWORD read=0;
    _snwprintf(name,CAP,L"%ls\\localserver\\client-version.txt",game);name[CAP-1]=0;
    HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    BOOL ok=ReadFile(file,bytes,sizeof(bytes)-1,&read,NULL);CloseHandle(file);
    if(!ok||!read)return FALSE;bytes[read]=0;
    while(read&&(bytes[read-1]=='\r'||bytes[read-1]=='\n'||bytes[read-1]==' '))bytes[--read]=0;
    if(!read||bytes[0]<'0'||bytes[0]>'9')return FALSE;
    for(DWORD i=0;i<read;i++)if((bytes[i]<'0'||bytes[i]>'9')&&bytes[i]!='.'&&bytes[i]!='-')return FALSE;
    if(!MultiByteToWideChar(CP_UTF8,0,bytes,-1,version,64))return FALSE;
    _snwprintf(name,CAP,L"%ls\\localserver\\update-notification.ini",game);name[CAP-1]=0;
    return WritePrivateProfileStringW(L"update",L"version",version,name);
}
