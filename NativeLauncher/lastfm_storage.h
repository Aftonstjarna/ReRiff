/* DPAPI user-scoped ASCII credentials. Never fall back to plaintext on write. */
static BOOL lfm_protect(const WCHAR *value,WCHAR out[4096]){
    *out=0;if(!*value)return TRUE;
    char ascii[64];int n=WideCharToMultiByte(CP_UTF8,0,value,-1,ascii,sizeof(ascii),NULL,NULL);if(n<=0)return FALSE;
    HMODULE dll=LoadLibraryW(L"crypt32.dll");if(!dll)return FALSE;
    typedef BOOL (WINAPI *Protect)(DATA_BLOB*,LPCWSTR,DATA_BLOB*,PVOID,CRYPTPROTECT_PROMPTSTRUCT*,DWORD,DATA_BLOB*);
    Protect protect=(Protect)GetProcAddress(dll,"CryptProtectData");DATA_BLOB input={(DWORD)n-1,(BYTE*)ascii},output={0};
    BOOL ok=protect&&protect(&input,L"ReRiff scrobbling",NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&output);
    if(ok&&output.cbData*2+7<4096){wcscpy(out,L"dpapi:");for(DWORD i=0;i<output.cbData;i++)swprintf(out+6+i*2,3,L"%02x",output.pbData[i]);}else ok=FALSE;
    if(output.pbData){SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);}SecureZeroMemory(ascii,sizeof(ascii));FreeLibrary(dll);return ok;
}
static BOOL scrobble_read_credential(const WCHAR *section,const WCHAR *name,WCHAR out[64]){
    WCHAR stored[4096];*out=0;DWORD n=GetPrivateProfileStringW(section,name,L"",stored,4096,ini);
    if(n>=4095)return FALSE;
    if(wcsncmp(stored,L"dpapi:",6)){if(n>=64)return FALSE;wcscpy(out,stored);SecureZeroMemory(stored,sizeof(stored));return TRUE;}
    size_t len=n-6;if(!len||len%2||len>4000)return FALSE;BYTE bytes[2000];
    for(size_t i=0;i<len;i++){WCHAR c=stored[6+i];int digit=c>=L'0'&&c<=L'9'?c-L'0':c>=L'a'&&c<=L'f'?c-L'a'+10:-1;if(digit<0)return FALSE;if(i%2)bytes[i/2]|=(BYTE)digit;else bytes[i/2]=(BYTE)(digit<<4);}
    HMODULE dll=LoadLibraryW(L"crypt32.dll");if(!dll)return FALSE;
    typedef BOOL (WINAPI *Unprotect)(DATA_BLOB*,LPWSTR*,DATA_BLOB*,PVOID,CRYPTPROTECT_PROMPTSTRUCT*,DWORD,DATA_BLOB*);
    Unprotect unprotect=(Unprotect)GetProcAddress(dll,"CryptUnprotectData");DATA_BLOB input={(DWORD)len/2,bytes},output={0};
    BOOL ok=unprotect&&unprotect(&input,NULL,NULL,NULL,NULL,CRYPTPROTECT_UI_FORBIDDEN,&output);
    if(ok){int count=output.cbData<64?MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(char*)output.pbData,output.cbData,out,63):0;ok=count>0;if(ok)out[count]=0;}
    if(output.pbData){SecureZeroMemory(output.pbData,output.cbData);LocalFree(output.pbData);}FreeLibrary(dll);SecureZeroMemory(stored,sizeof(stored));return ok;
}
static BOOL lfm_read_credential(const WCHAR *name,WCHAR out[64]){return scrobble_read_credential(L"lastfm",name,out);}
/* Prepare every encrypted value before replacing the config, preserving other sections. */
static BOOL scrobble_store_credentials(const WCHAR *section,const WCHAR *key,const WCHAR *secret,const WCHAR *session,const WCHAR *user,BOOL enabled,const WCHAR *percent){
    WCHAR a[4096],b[4096],c[4096],routed[CAP],temporary[CAP];
    if(!lfm_protect(key,a)||!lfm_protect(secret,b)||!lfm_protect(session,c))return FALSE;
    const WCHAR *file=rr_config_path(ini,section,routed);
    if(wcslen(file)+20>=CAP)return FALSE;_snwprintf(temporary,CAP,L"%ls.lastfm-new",file);
    BOOL exists=GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES;
    if(exists&&!CopyFileW(file,temporary,FALSE))return FALSE;
    if(!exists){HANDLE h=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(h==INVALID_HANDLE_VALUE)return FALSE;CloseHandle(h);}
    BOOL ok=WritePrivateProfileStringW(section,L"api_key",a,temporary)&&WritePrivateProfileStringW(section,L"shared_secret",b,temporary)&&WritePrivateProfileStringW(section,L"session_key",c,temporary)&&WritePrivateProfileStringW(section,L"username",user,temporary)&&WritePrivateProfileStringW(section,L"enabled",enabled?L"true":L"false",temporary);
    if(ok&&percent)ok=WritePrivateProfileStringW(section,L"scrobble_percent",percent,temporary);
    WritePrivateProfileStringW(NULL,NULL,NULL,temporary);
    if(ok)ok=MoveFileExW(temporary,file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok)DeleteFileW(temporary);return ok;
}

static BOOL lfm_store_credentials(const WCHAR *key,const WCHAR *secret,const WCHAR *session,const WCHAR *user,BOOL enabled,const WCHAR *percent){return scrobble_store_credentials(L"lastfm",key,secret,session,user,enabled,percent);}
