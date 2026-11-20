/* Exact-file identity handshake for the game's existing v19 song cache.
 * Cache layout: 16-byte header, seven NUL-terminated UTF-8 strings and 29
 * scalar bytes per record. Derived from original gaSaveGame.writeSongCache.
 * Never sends local filenames, audio bytes, scores or account credentials. */
#include <shlobj.h>
#include <wincrypt.h>
/* MinGW winhttp.h conflicts with wininet.h on HTTP_VERSION_INFO. These are
 * the WinHTTP declarations used here; handles use the wininet HINTERNET type. */
#define WINHTTP_ACCESS_TYPE_NO_PROXY 1
#define WINHTTP_NO_PROXY_NAME ((LPCWSTR)0)
#define WINHTTP_NO_PROXY_BYPASS ((LPCWSTR)0)
#define WINHTTP_FLAG_SECURE 0x00800000
#define WINHTTP_NO_REFERER ((LPCWSTR)0)
#define WINHTTP_DEFAULT_ACCEPT_TYPES ((LPCWSTR*)0)
#define WINHTTP_NO_REQUEST_DATA ((LPVOID)0)
#define WINHTTP_QUERY_STATUS_CODE 19
#define WINHTTP_QUERY_FLAG_NUMBER 0x20000000
#define WINHTTP_HEADER_NAME_BY_INDEX ((LPCWSTR)0)
#define WINHTTP_NO_HEADER_INDEX ((LPDWORD)0)
extern HINTERNET WINAPI WinHttpOpen(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD);
extern HINTERNET WINAPI WinHttpConnect(HINTERNET,LPCWSTR,INTERNET_PORT,DWORD);
extern HINTERNET WINAPI WinHttpOpenRequest(HINTERNET,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR*,DWORD);
extern BOOL WINAPI WinHttpSendRequest(HINTERNET,LPCWSTR,DWORD,LPVOID,DWORD,DWORD,DWORD_PTR);
extern BOOL WINAPI WinHttpReceiveResponse(HINTERNET,LPVOID);
extern BOOL WINAPI WinHttpQueryHeaders(HINTERNET,DWORD,LPCWSTR,LPVOID,LPDWORD,LPDWORD);
extern BOOL WINAPI WinHttpReadData(HINTERNET,LPVOID,DWORD,LPDWORD);
extern BOOL WINAPI WinHttpCloseHandle(HINTERNET);
extern BOOL WINAPI WinHttpSetTimeouts(HINTERNET,int,int,int,int);
extern BOOL WINAPI WinHttpCrackUrl(LPCWSTR,DWORD,DWORD,LPURL_COMPONENTS);
#include <stdlib.h>
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

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

#include "song_hash_cache.h"

static BOOL song_hash(const char *filename,char hex[65]) {
    WCHAR name[CAP];
    if(strncmp(filename,"hst:",4))return FALSE;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,filename+4,-1,name,CAP))return FALSE;
    HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    BY_HANDLE_FILE_INFORMATION identity;BYTE key[32];
    BOOL cacheable=GetFileInformationByHandle(file,&identity)&&song_path_key(name,key);
    if(cacheable&&song_hash_cached(key,&identity,hex)){CloseHandle(file);return TRUE;}
    song_hash_reads++;
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
    if(ok&&cacheable)song_hash_store(key,&identity,hex);
    CloseHandle(file);return ok;
}

static BOOL song_request(HINTERNET session,const WCHAR *base,const char *body,WCHAR *server_id,size_t server_id_cap) {
    if(!session)return FALSE;
    const WCHAR *p=base;INTERNET_PORT port=0;BOOL secure=FALSE;
    if(!wcsncmp(p,L"http://",7)){p+=7;port=80;}
    else if(!wcsncmp(p,L"https://",8)){p+=8;port=443;secure=TRUE;}
    else return FALSE;
    const WCHAR *slash=wcschr(p,L'/');size_t host_length=slash?(size_t)(slash-p):wcslen(p);
    if(!host_length||host_length>=1024)return FALSE;
    WCHAR host[1024];wcsncpy(host,p,host_length);host[host_length]=0;
    WCHAR *colon=wcschr(host,L':');
    if(colon){*colon=0;WCHAR *end=NULL;unsigned long value=wcstoul(colon+1,&end,10);if(!end||*end||!value||value>65535)return FALSE;port=(INTERNET_PORT)value;}
    HINTERNET connection=WinHttpConnect(session,host,port,0);
    DWORD flags=secure?WINHTTP_FLAG_SECURE:0;
    HINTERNET req=connection?WinHttpOpenRequest(connection,body?L"POST":L"GET",body?L"/reriff/songs/register":L"/classes/Version",NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,flags):NULL;
    BOOL ok=FALSE;DWORD code=0,size=sizeof(code);
    if(req){
        DWORD body_size=body?(DWORD)strlen(body):0;
        if(WinHttpSendRequest(req,body?L"Content-Type: application/json\r\n":NULL,-1L,
            body?(LPVOID)body:WINHTTP_NO_REQUEST_DATA,body_size,body_size,0)
            &&WinHttpReceiveResponse(req,NULL)
            &&WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&code,&size,WINHTTP_NO_HEADER_INDEX))
            ok=code==200;
        if(ok&&!body){
            char response[16385];DWORD used=0,count=0;BOOL reading=TRUE;
            while(used<sizeof(response)-1){
                if(!WinHttpReadData(req,response+used,(DWORD)sizeof(response)-1-used,&count)){reading=FALSE;break;}
                if(!count)break;used+=count;
            }
            response[used]=0;char *first=response;while(*first==' '||*first=='\t'||*first=='\r'||*first=='\n')first++;
            ok=reading&&*first=='{'&&strstr(response,"\"results\"")!=NULL;
            if(server_id&&server_id_cap){
                server_id[0]=0;
                char *marker=strstr(response,"\"ServerID\"");
                if(marker){char *colon2=strchr(marker,':');if(colon2){char *q=colon2+1;while(*q==' '||*q=='\t')q++;if(*q=='\"'){q++;size_t n=0;while(q[n]&&q[n]!='\"'&&n+1<server_id_cap){server_id[n]=(WCHAR)(unsigned char)q[n];n++;}server_id[n]=0;}}}
            }
        }
    }
    if(req)WinHttpCloseHandle(req);if(connection)WinHttpCloseHandle(connection);
    return ok;
}

/* Per-server v19 song cache. The game has one cache.bin, so the launcher
 * swaps in the cache for the selected server identity. Singleplayer is its
 * own identity and is never registered with a remote catalog. */
static BOOL song_cache_paths(const WCHAR *key,WCHAR *cache,WCHAR *sidecar) {
    WCHAR appdata[MAX_PATH],directory[MAX_PATH];
    if(FAILED(SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,appdata)))return FALSE;
    _snwprintf(directory,MAX_PATH,L"%ls\\Octane\\DAT",appdata);directory[MAX_PATH-1]=0;
    SHCreateDirectoryExW(NULL,directory,NULL);
    _snwprintf(cache,CAP,L"%ls\\cache.bin",directory);cache[CAP-1]=0;
    _snwprintf(sidecar,CAP,L"%ls\\cache.%ls.bin",directory,key);sidecar[CAP-1]=0;
    return cache[CAP-1]==0&&sidecar[CAP-1]==0;
}
static BOOL song_cache_read(const WCHAR *path,char **data,DWORD *size) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    DWORD length=GetFileSize(file,NULL),read=0;
    if(length<16||length>16*1024*1024){CloseHandle(file);return FALSE;}
    char *bytes=(char*)malloc(length);
    if(!bytes){CloseHandle(file);return FALSE;}
    BOOL ok=ReadFile(file,bytes,length,&read,NULL)&&read==length;
    CloseHandle(file);
    if(!ok){free(bytes);return FALSE;}
    *data=bytes;*size=length;return TRUE;
}
static BOOL song_cache_write(const WCHAR *path,const char *data,DWORD size) {
    WCHAR temporary[CAP];_snwprintf(temporary,CAP,L"%ls.tmp-%lu",path,(unsigned long)GetCurrentProcessId());temporary[CAP-1]=0;
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    DWORD wrote=0;BOOL ok=WriteFile(file,data,size,&wrote,NULL)&&wrote==size&&FlushFileBuffers(file);
    CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);
    return ok;
}
static BOOL song_cache_bundled(const char *data,DWORD size,char **output,DWORD *out_size) {
    if(size<16)return FALSE;
    DWORD header[4];memcpy(header,data,16);
    if(header[0]!=1128350512||header[1]!=19||header[3]!=18||header[2]>10000)return FALSE;
    char *result=(char*)malloc(size);
    if(!result)return FALSE;
    memcpy(result,data,16);
    DWORD cursor=16,target=16,count=0;
    for(DWORD i=0;i<header[2];i++){
        DWORD start=cursor;
        for(int field=0;field<7;field++){
            char *zero=(char*)memchr(data+cursor,0,size-cursor);
            if(!zero){free(result);return FALSE;}
            cursor=(DWORD)(zero-data)+1;
        }
        if(size-cursor<29){free(result);return FALSE;}
        cursor+=29;
        if(strncmp(data+start,"hst:",4)){DWORD length=cursor-start;memcpy(result+target,data+start,length);target+=length;count++;}
    }
    if(cursor!=size){free(result);return FALSE;}
    memcpy(result+8,&count,4);
    *output=result;*out_size=target;return TRUE;
}
static BOOL song_cache_prepare(const WCHAR *key) {
    WCHAR cache[CAP],sidecar[CAP],sp_sidecar[CAP];
    if(!song_cache_paths(key,cache,sidecar)||!song_cache_paths(L"singleplayer",cache,sp_sidecar))return FALSE;
    char *data=NULL;DWORD size=0;BOOL have=song_cache_read(cache,&data,&size);
    if(have&&GetFileAttributesW(sp_sidecar)==INVALID_FILE_ATTRIBUTES)song_cache_write(sp_sidecar,data,size);
    BOOL result=TRUE;
    if(GetFileAttributesW(sidecar)!=INVALID_FILE_ATTRIBUTES)result=CopyFileW(sidecar,cache,FALSE)!=0;
    else if(!_wcsicmp(key,L"singleplayer")){if(have)result=song_cache_write(sidecar,data,size);}
    else if(have){char *filtered=NULL;DWORD filtered_size=0;if(song_cache_bundled(data,size,&filtered,&filtered_size)){result=song_cache_write(cache,filtered,filtered_size);free(filtered);}}
    free(data);return result;
}
static BOOL song_cache_commit(const WCHAR *key) {
    WCHAR cache[CAP],sidecar[CAP];if(!song_cache_paths(key,cache,sidecar))return FALSE;
    char *data=NULL;DWORD size=0;if(!song_cache_read(cache,&data,&size))return FALSE;
    BOOL ok=song_cache_write(sidecar,data,size);free(data);return ok;
}
static BOOL song_cache_safe(const WCHAR *value) {
    if(!value||!*value)return FALSE;size_t length=0;
    for(const WCHAR *p=value;*p;p++,length++){
        if(length>=120)return FALSE;
        WCHAR c=*p;if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'))return FALSE;
    }
    return TRUE;
}
static void song_cache_hashed(const WCHAR *value,WCHAR *key,size_t capacity) {
    unsigned long long hash=1469598103934665603ULL;
    for(const WCHAR *p=value;*p;p++){hash^=(unsigned long long)(unsigned int)*p;hash*=1099511628211ULL;}
    _snwprintf(key,capacity,L"server-%016llx",hash);key[capacity-1]=0;
}
static void song_cache_key_for(const WCHAR *server_id,WCHAR *key,size_t capacity) {
    if(song_cache_safe(server_id)){
        _snwprintf(key,capacity,L"server-%ls",server_id);key[capacity-1]=0;
    }else song_cache_hashed(server_id?server_id:L"",key,capacity);
}
static BOOL song_cache_identity(const WCHAR *config,WCHAR *key,size_t capacity,WCHAR *base,WCHAR *server_id,BOOL *multiplayer) {
    wcscpy(key,L"singleplayer");base[0]=0;server_id[0]=0;if(multiplayer)*multiplayer=FALSE;
    WCHAR networking[32];GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",networking,32,config);
    if(!_wcsicmp(networking,L"false")||!_wcsicmp(networking,L"no")||!wcscmp(networking,L"0"))return TRUE;
    WCHAR timeout_value[64],*timeout_end;GetPrivateProfileStringW(L"server",L"timeout_seconds",L"5",timeout_value,64,config);
    double seconds=wcstod(timeout_value,&timeout_end);while(*timeout_end==' '||*timeout_end=='\t')timeout_end++;
    if(*timeout_end||!(seconds>0&&seconds<=60))return TRUE;
    song_timeout=(DWORD)(seconds*1000);
    HINTERNET session=WinHttpOpen(L"ReRiffSongSync/1",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if(!session)return TRUE;
    if(session)WinHttpSetTimeouts(session,song_timeout,song_timeout,song_timeout,song_timeout);
    WCHAR external[CAP],fallback[CAP],only[32];
    GetPrivateProfileStringW(L"server",L"external_url",L"",external,CAP,config);
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",fallback,CAP,config);
    GetPrivateProfileStringW(L"server",L"external_only",L"false",only,32,config);
    BOOL external_only=!_wcsicmp(only,L"true")||!wcscmp(only,L"1")||!_wcsicmp(only,L"yes");
    WCHAR candidates[2][CAP];wcscpy(candidates[0],external);wcscpy(candidates[1],external_only?L"":fallback);
    for(int i=0;i<2;i++){
        size_t n=wcslen(candidates[i]);while(n&&candidates[i][n-1]=='/')candidates[i][--n]=0;
        if(!n)continue;
        WCHAR id[256]={0};
        if(song_request(session,candidates[i],NULL,id,256)){
            wcscpy(base,candidates[i]);wcscpy(server_id,id);if(multiplayer)*multiplayer=TRUE;song_cache_key_for(id[0]?id:candidates[i],key,capacity);break;
        }
    }
    WinHttpCloseHandle(session);
    return TRUE;
}


static void sync_cached_songs(const WCHAR *config,const WCHAR *selected) {
    WCHAR networking[32];
    GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",networking,32,config);
    if(!_wcsicmp(networking,L"false")||!_wcsicmp(networking,L"no")||!wcscmp(networking,L"0"))return;
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
    ULONGLONG sync_started=GetTickCount64();
    WCHAR sidecar[CAP];_snwprintf(sidecar,CAP,L"%ls\\Octane\\DAT\\ReRiff-song-hashes.bin",appdata);sidecar[CAP-1]=0;
    song_hash_load(sidecar);
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
    song_hash_save();
    WCHAR timing[256];_snwprintf(timing,256,L"Timing: song hashing %llu ms; reused=%u read=%u",(unsigned long long)(GetTickCount64()-sync_started),song_hash_hits,song_hash_reads);log_message(timing);
    if(valid&&registered){
        /* Keep one WinHTTP session through probe and registration, allowing
         * connection/TLS reuse without caching server health or skipping the
         * identity handshake that must finish before the game starts. */
        HINTERNET session=WinHttpOpen(L"ReRiffSongSync/1",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
        if(session)WinHttpSetTimeouts(session,song_timeout,song_timeout,song_timeout,song_timeout);
        WCHAR base[CAP],only[32];
        if(selected&&*selected){wcscpy(base,selected);base[CAP-1]=0;}
        else{
            GetPrivateProfileStringW(L"server",L"external_url",L"",base,CAP,config);
            GetPrivateProfileStringW(L"server",L"external_only",L"false",only,32,config);
            BOOL externalOnly=!_wcsicmp(only,L"true")||!wcscmp(only,L"1")||!_wcsicmp(only,L"yes");
            size_t m=wcslen(base);while(m&&base[m-1]=='/')base[--m]=0;
            if(!m||!song_request(session,base,NULL,NULL,0)){
                if(externalOnly)base[0]=0;
                else GetPrivateProfileStringW(L"server",L"fallback_url",L"",base,CAP,config);
            }
        }
        size_t n=wcslen(base);while(n&&base[n-1]=='/')base[--n]=0;
        BOOL ok=n&&song_request(session,base,body.data,NULL,0);
        WCHAR note[256];_snwprintf(note,256,L"Song identity sync: %ls; files=%u unavailable=%u",ok?L"registered":L"unavailable",registered,skipped);
        log_message(note);
        if(session)WinHttpCloseHandle(session);
    }else if(!valid)log_message(L"Song identity sync: unsupported or malformed cache; no registration sent.");
    free(body.data);
    _snwprintf(timing,256,L"Timing: song sync total %llu ms",(unsigned long long)(GetTickCount64()-sync_started));log_message(timing);
    free(song_hash_entries);song_hash_entries=NULL;song_hash_count=0;
}
