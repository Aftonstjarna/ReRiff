/* Performance cache, not an integrity/security authority. Recheck opened-file
 * identity, size and timestamps; expire after one day even if metadata matches.
 * Paths are hashed locally and never stored in this sidecar or sent to servers. */
#define SONG_HASH_LIMIT 10000
#define SONG_HASH_AGE (24ULL*60*60*10000000)
typedef struct {
    BYTE path[32]; DWORD volume,index_high,index_low,size_high,size_low;
    FILETIME created,written; ULONGLONG checked; char hex[65];
} SongHashEntry;
static SongHashEntry *song_hash_entries;
static unsigned song_hash_count,song_hash_hits,song_hash_reads;
static WCHAR song_hash_sidecar[CAP];
static BOOL song_hash_dirty;
static ULONGLONG song_clock(void){FILETIME time;GetSystemTimeAsFileTime(&time);return ((ULONGLONG)time.dwHighDateTime<<32)|time.dwLowDateTime;}
static BOOL song_path_key(const WCHAR *name,BYTE digest[32]){
    HCRYPTPROV provider=0;HCRYPTHASH hash=0;DWORD size=32;BOOL ok=FALSE;
    if(CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash))
        ok=CryptHashData(hash,(const BYTE *)name,(DWORD)(wcslen(name)*sizeof(WCHAR)),0)&&CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0);
    if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);return ok;
}
static BOOL song_same_file(const SongHashEntry *entry,const BY_HANDLE_FILE_INFORMATION *info){
    return entry->volume==info->dwVolumeSerialNumber&&entry->index_high==info->nFileIndexHigh&&entry->index_low==info->nFileIndexLow
        &&entry->size_high==info->nFileSizeHigh&&entry->size_low==info->nFileSizeLow
        &&!CompareFileTime(&entry->created,&info->ftCreationTime)&&!CompareFileTime(&entry->written,&info->ftLastWriteTime);
}
static void song_hash_load(const WCHAR *name){
    free(song_hash_entries);song_hash_entries=calloc(SONG_HASH_LIMIT,sizeof(SongHashEntry));
    song_hash_dirty=FALSE;
    song_hash_count=song_hash_hits=song_hash_reads=0;wcsncpy(song_hash_sidecar,name,CAP-1);song_hash_sidecar[CAP-1]=0;
    if(!song_hash_entries)return;
    FILE *file=_wfopen(name,L"rb");if(!file)return;
    DWORD header[3];
    if(fread(header,sizeof(header),1,file)==1&&header[0]==0x31485352&&header[1]==sizeof(SongHashEntry)&&header[2]<=SONG_HASH_LIMIT
       &&fread(song_hash_entries,sizeof(SongHashEntry),header[2],file)==header[2]&&fgetc(file)==EOF){
        BOOL valid=TRUE;
        for(unsigned n=0;n<header[2];n++){
            char *hex=song_hash_entries[n].hex;
            if(hex[64])valid=FALSE;
            for(unsigned j=0;j<64;j++)if(!((hex[j]>='0'&&hex[j]<='9')||(hex[j]>='a'&&hex[j]<='f')))valid=FALSE;
        }
        if(valid)song_hash_count=header[2];
    }
    fclose(file);
}
static void song_hash_save(void){
    if(!song_hash_dirty||!song_hash_entries||!*song_hash_sidecar)return;
    WCHAR temporary[CAP];if(wcslen(song_hash_sidecar)>CAP-32)return;
    _snwprintf(temporary,CAP,L"%ls.%lu.tmp",song_hash_sidecar,(unsigned long)GetCurrentProcessId());
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return;
    DWORD header[]={0x31485352,sizeof(SongHashEntry),song_hash_count},written,bytes=song_hash_count*sizeof(SongHashEntry);
    BOOL ok=WriteFile(file,header,sizeof(header),&written,NULL)&&written==sizeof(header)
        &&WriteFile(file,song_hash_entries,bytes,&written,NULL)&&written==bytes&&FlushFileBuffers(file);
    CloseHandle(file);
    if(ok)ok=MoveFileExW(temporary,song_hash_sidecar,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    if(!ok)DeleteFileW(temporary);
}
static BOOL song_hash_cached(const BYTE key[32],const BY_HANDLE_FILE_INFORMATION *info,char hex[65]){
    ULONGLONG now=song_clock();
    for(unsigned n=0;n<song_hash_count;n++){
        SongHashEntry *entry=song_hash_entries+n;
        if(!memcmp(entry->path,key,32)&&song_same_file(entry,info)&&now>=entry->checked&&now-entry->checked<SONG_HASH_AGE){
            memcpy(hex,entry->hex,65);song_hash_hits++;return TRUE;
        }
    }
    return FALSE;
}
static void song_hash_store(const BYTE key[32],const BY_HANDLE_FILE_INFORMATION *info,const char hex[65]){
    if(!song_hash_entries)return;
    unsigned n=0;for(;n<song_hash_count;n++)if(!memcmp(song_hash_entries[n].path,key,32))break;
    if(n==SONG_HASH_LIMIT){
        n=0;for(unsigned j=1;j<song_hash_count;j++)if(song_hash_entries[j].checked<song_hash_entries[n].checked)n=j;
    }
    song_hash_dirty=TRUE;
    if(n==song_hash_count)song_hash_count++;
    SongHashEntry *entry=song_hash_entries+n;memset(entry,0,sizeof(*entry));memcpy(entry->path,key,32);
    entry->volume=info->dwVolumeSerialNumber;entry->index_high=info->nFileIndexHigh;entry->index_low=info->nFileIndexLow;
    entry->size_high=info->nFileSizeHigh;entry->size_low=info->nFileSizeLow;
    entry->created=info->ftCreationTime;entry->written=info->ftLastWriteTime;entry->checked=song_clock();memcpy(entry->hex,hex,65);
}
