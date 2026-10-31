/* Exact-file identity handshake for the game's existing v19 song cache.
 * Cache layout: 16-byte header, seven NUL-terminated UTF-8 strings and 29
 * scalar bytes per record. Derived from original gaSaveGame.writeSongCache.
 * Never sends local filenames, audio bytes, scores or account credentials. */
#include <shlobj.h>
#include <wincrypt.h>
#include <stdlib.h>

typedef struct { char *data; size_t used, capacity; BOOL failed; } SongBuffer;
static DWORD song_timeout=5000;
static void song_append(SongBuffer *b, const char *s) {
    size_t n=strlen(s);
    if(b->failed)return;
    if(b->used+n+1>8*1024*1024){b->failed=TRUE;return;}
    if(b->used+n+1>b->capacity){
        size_t cap=(b->used+n+1)*2;
        char *p=(char*)realloc(b->data,cap);
        if(!p){b->failed=TRUE;return;}b->data=p;b->capacity=cap;
    }
    memcpy(b->data+b->used,s,n+1);b->used+=n;
}
static void song_json(SongBuffer *b,const char *s) {
    song_append(b,"\"");
    for(;*s;s++){
        unsigned char c=(unsigned char)*s;char escaped[8];
        if(c=='"'||c=='\\'){escaped[0]='\\';escaped[1]=(char)c;escaped[2]=0;}
        else if(c<32)snprintf(escaped,sizeof(escaped),"\\u%04x",c);
        else{escaped[0]=(char)c;escaped[1]=0;}
        song_append(b,escaped);
    }
    song_append(b,"\"");
}

static BOOL song_hash(const char *filename,char hex[65]) {
    WCHAR name[CAP];
    if(strncmp(filename,"hst:",4))return FALSE;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename+4,-1,name,CAP))return FALSE;
    HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;BOOL ok=FALSE;
    BYTE digest[32],block[65536];DWORD count=0,size=sizeof(digest);
    LARGE_INTEGER before_size,after_size;FILETIME before_time,after_time;
    if(GetFileSizeEx(file,&before_size)&&GetFileTime(file,NULL,NULL,&before_time)
       &&CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)
       &&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)){
        BOOL reading=TRUE;
        for(;;){
            if(!ReadFile(file,block,sizeof(block),&count,NULL)){reading=FALSE;break;}
            if(!count)break;
            if(!CryptHashData(hash,block,count,0)){reading=FALSE;break;}
        }
        if(reading&&GetFileSizeEx(file,&after_size)&&GetFileTime(file,NULL,NULL,&after_time)
           &&before_size.QuadPart==after_size.QuadPart&&!CompareFileTime(&before_time,&after_time)
           &&CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)){
            for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
            ok=TRUE;
        }
    }
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);
    CloseHandle(file);return ok;
}

static BOOL song_request(const WCHAR *base,const char *body) {
    WCHAR url[CAP];URL_COMPONENTSW parts;WCHAR host[1024],urlpath[CAP];
    _snwprintf(url,CAP,L"%ls%ls",base,body?L"/reriff/songs/register":L"/classes/Version");url[CAP-1]=0;
    ZeroMemory(&parts,sizeof(parts));parts.dwStructSize=sizeof(parts);
    parts.lpszHostName=host;parts.dwHostNameLength=1024;parts.lpszUrlPath=urlpath;parts.dwUrlPathLength=CAP;
    if(!InternetCrackUrlW(url,0,0,&parts)||(parts.nScheme!=INTERNET_SCHEME_HTTP&&parts.nScheme!=INTERNET_SCHEME_HTTPS))return FALSE;
    HINTERNET session=InternetOpenW(L"ReRiffSongSync/1",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL,0);
    if(!session)return FALSE;
    DWORD timeout=song_timeout;InternetSetOptionW(session,INTERNET_OPTION_CONNECT_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionW(session,INTERNET_OPTION_RECEIVE_TIMEOUT,&timeout,sizeof(timeout));
    InternetSetOptionW(session,INTERNET_OPTION_SEND_TIMEOUT,&timeout,sizeof(timeout));
    HINTERNET connection=InternetConnectW(session,host,parts.nPort,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);
    DWORD flags=INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_NO_UI|INTERNET_FLAG_NO_AUTO_REDIRECT;
    if(parts.nScheme==INTERNET_SCHEME_HTTPS)flags|=INTERNET_FLAG_SECURE;
    HINTERNET req=connection?HttpOpenRequestW(connection,body?L"POST":L"GET",urlpath,NULL,NULL,NULL,flags,0):NULL;
    BOOL ok=FALSE;DWORD code=0,size=sizeof(code);
    if(req&&HttpSendRequestW(req,body?L"Content-Type: application/json\r\n":NULL,body?(DWORD)-1:0,
        (LPVOID)body,body?(DWORD)strlen(body):0)&&HttpQueryInfoW(req,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,&code,&size,NULL))
        ok=code==200;
    if(ok&&!body){
        char response[16385];DWORD used=0,count=0;BOOL reading=TRUE;
        while(used<sizeof(response)-1){
            if(!InternetReadFile(req,response+used,(DWORD)sizeof(response)-1-used,&count)){reading=FALSE;break;}
            if(!count)break;used+=count;
        }
        response[used]=0;char *first=response;while(*first==' '||*first=='\t'||*first=='\r'||*first=='\n')first++;
        ok=reading&&*first=='{'&&strstr(response,"\"results\"")!=NULL;
    }
    if(req)InternetCloseHandle(req);if(connection)InternetCloseHandle(connection);InternetCloseHandle(session);
    return ok;
}

static void sync_cached_songs(const WCHAR *config) {
    WCHAR timeout_value[64],*timeout_end;
    GetPrivateProfileStringW(L"server",L"timeout_seconds",L"5",timeout_value,64,config);
    double seconds=wcstod(timeout_value,&timeout_end);
    while(*timeout_end==' '||*timeout_end=='\t')timeout_end++;
    if(*timeout_end||!(seconds>0&&seconds<=60))return;
    song_timeout=(DWORD)(seconds*1000);
    WCHAR cachepath[CAP],appdata[MAX_PATH];
    if(FAILED(SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,appdata)))return;
    _snwprintf(cachepath,CAP,L"%ls\\Octane\\DAT\\cache.bin",appdata);cachepath[CAP-1]=0;
    HANDLE file=CreateFileW(cachepath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return;
    DWORD size=GetFileSize(file,NULL),read=0;
    if(size<16||size>16*1024*1024){CloseHandle(file);return;}
    char *cache=(char*)malloc(size);
    if(!cache){CloseHandle(file);return;}
    BOOL loaded=ReadFile(file,cache,size,&read,NULL)&&read==size;CloseHandle(file);
    DWORD header[4];if(loaded)memcpy(header,cache,16);
    if(!loaded||header[0]!=1128350512||header[1]!=19||header[3]!=18||header[2]>10000){free(cache);return;}
    SongBuffer body={0};song_append(&body,"{\"songs\":[");
    char *cursor=cache+16,*end=cache+size;unsigned registered=0,skipped=0;BOOL valid=TRUE;
    for(DWORD i=0;i<header[2]&&valid;i++){
        const char *fields[7];
        for(int j=0;j<7;j++){
            fields[j]=cursor;char *zero=(char*)memchr(cursor,0,(size_t)(end-cursor));
            if(!zero){valid=FALSE;break;}cursor=zero+1;
        }
        if(!valid||end-cursor<29){valid=FALSE;break;}cursor+=29;
        if(strncmp(fields[0],"hst:",4))continue; /* Bundled tracks have fixed shared IDs. */
        char hash[65];
        if(!song_hash(fields[0],hash)){skipped++;continue;}
        if(registered++)song_append(&body,",");
        song_append(&body,"{\"song_id\":");song_json(&body,fields[1]);
        song_append(&body,",\"sha256\":");song_json(&body,hash);
        song_append(&body,",\"title\":");song_json(&body,fields[2]);
        song_append(&body,",\"artist\":");song_json(&body,fields[3]);song_append(&body,"}");
    }
    song_append(&body,"]}");valid=valid&&cursor==end&&!body.failed;
    free(cache);
    if(valid&&registered){
        WCHAR base[CAP],only[32];
        GetPrivateProfileStringW(L"server",L"external_url",L"",base,CAP,config);
        GetPrivateProfileStringW(L"server",L"external_only",L"false",only,32,config);
        BOOL externalOnly=!_wcsicmp(only,L"true")||!wcscmp(only,L"1")||!_wcsicmp(only,L"yes");
        size_t n=wcslen(base);while(n&&base[n-1]=='/')base[--n]=0;
        if(!n||!song_request(base,NULL)){
            if(externalOnly)base[0]=0;
            else GetPrivateProfileStringW(L"server",L"fallback_url",L"",base,CAP,config);
        }
        n=wcslen(base);while(n&&base[n-1]=='/')base[--n]=0;
        BOOL ok=n&&song_request(base,body.data);
        WCHAR note[256];_snwprintf(note,256,L"Song identity sync: %ls; files=%u unavailable=%u",ok?L"registered":L"unavailable",registered,skipped);
        log_message(note);
    }else if(!valid)log_message(L"Song identity sync: unsupported or malformed cache; no registration sent.");
    free(body.data);
}
