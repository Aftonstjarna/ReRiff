#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#define CAP 4096
#ifndef RERIFF_VERSION
#define RERIFF_VERSION "1.4.0-beta.12"
#endif
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void log_message(const WCHAR *message){(void)message;}
#include "song_sync.h"
static WCHAR folder[CAP];
static BOOL automatic_update,force_auto_reinstall,installed_experimental,requested_experimental;
static void path(WCHAR *out,const WCHAR *name){_snwprintf(out,CAP,L"%ls\\%ls",folder,name);}
#include "update_check_cache.h"
static BOOL write_bytes(const WCHAR *name,const char *data){FILE *f=_wfopen(name,L"wb");if(!f)return FALSE;BOOL ok=fwrite(data,1,strlen(data),f)==strlen(data);fclose(f);return ok;}
int main(void){
    WCHAR temp[MAX_PATH],audio[CAP],cache[CAP],local[CAP];char filename[CAP*3],first[65],second[65];
    CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"rpc",0,folder));CHECK(DeleteFileW(folder));CHECK(CreateDirectoryW(folder,NULL));
    path(local,L"localserver");CHECK(CreateDirectoryW(local,NULL));path(audio,L"song.bin");path(cache,L"hashes.bin");
    strcpy(filename,"hst:");CHECK(WideCharToMultiByte(CP_UTF8,0,audio,-1,filename+4,sizeof(filename)-4,NULL,NULL));
    CHECK(write_bytes(audio,"abc"));song_hash_load(cache);
    CHECK(song_hash(filename,first));CHECK(!strcmp(first,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    CHECK(song_hash_reads==1&&song_hash_hits==0);song_hash_save();song_hash_load(cache);
    CHECK(song_hash(filename,second));CHECK(!strcmp(first,second));CHECK(song_hash_hits==1&&song_hash_reads==0);
    CHECK(write_bytes(audio,"abcd"));CHECK(song_hash(filename,second));CHECK(strcmp(first,second));CHECK(song_hash_reads==1);
    strcpy(first,second);CHECK(write_bytes(audio,"wxyz"));
    HANDLE f=CreateFileW(audio,FILE_WRITE_ATTRIBUTES,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(f!=INVALID_HANDLE_VALUE);
    ULONGLONG ticks=song_clock()+10000000;FILETIME changed={(DWORD)ticks,(DWORD)(ticks>>32)};CHECK(SetFileTime(f,NULL,NULL,&changed));CloseHandle(f);
    CHECK(song_hash(filename,second));CHECK(strcmp(first,second));CHECK(song_hash_reads==2);
    song_hash_entries[0].checked=song_clock()-SONG_HASH_AGE-1;CHECK(song_hash(filename,second));CHECK(song_hash_reads==3);
    song_hash_entries[0].checked=song_clock()+SONG_HASH_AGE;CHECK(song_hash(filename,second));CHECK(song_hash_reads==4);
    CHECK(DeleteFileW(audio));CHECK(!song_hash(filename,second));
    CHECK(write_bytes(audio,"abc"));CHECK(song_hash(filename,first));CHECK(song_hash_reads==5);
    CHECK(!strcmp(first,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));CHECK(DeleteFileW(audio));
    CHECK(write_bytes(cache,"bad"));song_hash_load(cache);CHECK(song_hash_count==0);
    automatic_update=TRUE;CHECK(!update_check_recent());update_check_remember();CHECK(update_check_recent());
    automatic_update=FALSE;CHECK(!update_check_recent());automatic_update=TRUE;
    force_auto_reinstall=TRUE;CHECK(!update_check_recent());force_auto_reinstall=FALSE;
    requested_experimental=TRUE;CHECK(!update_check_recent());requested_experimental=FALSE;
    installed_experimental=TRUE;CHECK(!update_check_recent());installed_experimental=FALSE;
    path(cache,L"localserver\\update-check-cache.bin");FILE *stamp=_wfopen(cache,L"r+b");CHECK(stamp);
    UpdateCheckCache saved;CHECK(fread(&saved,sizeof(saved),1,stamp)==1);saved.checked=update_clock()-UPDATE_CHECK_AGE-1;rewind(stamp);CHECK(fwrite(&saved,sizeof(saved),1,stamp)==1);fclose(stamp);CHECK(!update_check_recent());
    update_check_remember();stamp=_wfopen(cache,L"r+b");CHECK(stamp);CHECK(fread(&saved,sizeof(saved),1,stamp)==1);saved.checked=update_clock()+UPDATE_CHECK_AGE;rewind(stamp);CHECK(fwrite(&saved,sizeof(saved),1,stamp)==1);fclose(stamp);CHECK(!update_check_recent());
    CHECK(write_bytes(cache,"broken"));CHECK(!update_check_recent());CHECK(DeleteFileW(cache));CHECK(RemoveDirectoryW(local));
    path(cache,L"hashes.bin");CHECK(DeleteFileW(cache));CHECK(RemoveDirectoryW(folder));
    free(song_hash_entries);
    /* Keep the complete sync path compiled, without network or user cache access. */
    (void)sync_cached_songs;
    puts("PASS persistent hash reuse; changed size/time; expiry/clock rollback; missing/corrupt files; manual/forced/channel update bypass");return 0;
}
