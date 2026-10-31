/* Engine-rendered experiment for the hash-pinned original x86 Game.exe.
 * Runs only on the game's own menu callbacks. No windows or input polling. */
#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#define DllMain NotificationDllMain
#define ReRiffShowServerToastW NativeServerToastW
#include "../NativeLauncher/notification.c"
#undef ReRiffShowServerToastW
#undef DllMain

typedef void (__thiscall *ScreenFn)(void *,void *,void *);
typedef void (__thiscall *InputFn)(void *,int);
typedef void (__thiscall *TickFn)(void *);
typedef void (__thiscall *EnableFn)(void *,BYTE);
typedef void (__thiscall *ColourFn)(void *,int,const float *);
typedef void (__thiscall *RawTextFn)(void *,const WCHAR *);
typedef void (__thiscall *KeyTextFn)(void *,const char *);
typedef const char *(__thiscall *TitleFn)(void *);
typedef struct Settings { WCHAR external[2048], fallback[2048]; BOOL only, enabled; } Settings;
static ScreenFn original_init, original_cleanup;
static InputFn original_input;
static TickFn original_tick;
static EnableFn enable_input;
static RawTextFn raw_text;
static KeyTextFn key_text;
static const char **original_labels;
static WCHAR config_path[MAX_PATH], log_path[MAX_PATH];
static Settings pending;
static void *page_owner;
static BOOL server_page;
static WCHAR heading[96]=L"Servers — applies after restart";
static volatile LONG installed;
#define SERVERS_ID 8

static void log_line(const char *line) {
    HANDLE file=CreateFileW(log_path,FILE_APPEND_DATA,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file!=INVALID_HANDLE_VALUE) {
        DWORD written; WriteFile(file,line,(DWORD)strlen(line),&written,NULL);
        WriteFile(file,"\r\n",2,&written,NULL); CloseHandle(file);
    }
}

static void load_settings(void) {
    WCHAR value[32];
    GetPrivateProfileStringW(L"server",L"external_url",L"",pending.external,2048,config_path);
    GetPrivateProfileStringW(L"server",L"fallback_url",L"",pending.fallback,2048,config_path);
    GetPrivateProfileStringW(L"server",L"external_only",L"false",value,32,config_path);
    pending.only=!_wcsicmp(value,L"true") || !_wcsicmp(value,L"yes") || !wcscmp(value,L"1");
    GetPrivateProfileStringW(L"server",L"networking_enabled",L"true",value,32,config_path);
    pending.enabled=_wcsicmp(value,L"false") && _wcsicmp(value,L"no") && wcscmp(value,L"0");
}

static BOOL save_settings(void) {
    WCHAR temp[MAX_PATH];
    if (wcslen(config_path)+5>=MAX_PATH) return FALSE;
    wcscpy(temp,config_path); wcscat(temp,L".tmp");
    if (!CopyFileW(config_path,temp,FALSE)) {
        if (GetLastError()!=ERROR_FILE_NOT_FOUND) return FALSE;
        HANDLE file=CreateFileW(temp,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
        if (file==INVALID_HANDLE_VALUE) return FALSE;
        CloseHandle(file);
    }
    BOOL ok=WritePrivateProfileStringW(L"server",L"external_url",pending.external,temp)
        && WritePrivateProfileStringW(L"server",L"fallback_url",pending.fallback,temp)
        && WritePrivateProfileStringW(L"server",L"external_only",pending.only?L"true":L"false",temp)
        && WritePrivateProfileStringW(L"server",L"networking_enabled",pending.enabled?L"true":L"false",temp);
    if (ok) {
        WritePrivateProfileStringW(NULL,NULL,NULL,temp);
        ok=MoveFileExW(temp,config_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
        WritePrivateProfileStringW(NULL,NULL,NULL,config_path);
    }
    if (!ok) DeleteFileW(temp);
    return ok;
}

static BOOL readable(const void *p,size_t count) {
    MEMORY_BASIC_INFORMATION info;
    if (!p || !VirtualQuery(p,&info,sizeof(info)) || info.State!=MEM_COMMIT
        || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return FALSE;
    uintptr_t start=(uintptr_t)p, end=(uintptr_t)info.BaseAddress+info.RegionSize;
    return start<=end && count<=end-start;
}

static void *item_text(void *menu,int id) {
    /* ILSpy: gaMainMenu.initScreens/tick; getChild reads its vector at +376. */
    void *item=((void **)menu)[62+id];
    if (!readable(item,384)) return NULL;
    void **begin=*(void ***)((BYTE *)item+376);
    void **end=*(void ***)((BYTE *)item+380);
    if ((uintptr_t)end<=(uintptr_t)begin || !readable(begin,sizeof(void *))) return NULL;
    return readable(*begin,484)?*begin:NULL;
}

static void keep_servers_available(void *menu) {
    void *item=((void **)menu)[62+SERVERS_ID];
    if (!readable(item,404)) return;
    /* enableInput always clears the interaction latch at +356, even when
     * already enabled. Calling it every frame breaks mouse press/release.
     * Its original enabled input mask at +76 is 0xffffffff. */
    if (*(DWORD *)((BYTE *)item+76)!=0xffffffffu) enable_input(item,TRUE);
    static const float focus[4]={0.0f,1.0f,1.0f,0.8f};
    static const float normal[4]={1.0f,1.0f,1.0f,0.9f};
    ColourFn colour=(ColourFn)(*(void ***)menu)[26];
    colour(menu,SERVERS_ID,((int *)menu)[88]==SERVERS_ID?focus:normal);
}

static void __thiscall hook_tick(void *menu) {
    original_tick(menu);
    keep_servers_available(menu);
}

static void render(void *menu) {
    static const WCHAR *labels[11]={L"", L"Settings apply after restart",
        L"Use local server", L"Use configured external server", L"",
        L"Disable networking", L"Save settings", L"Reload saved settings",
        L"Server settings", L"Back without saving", L"Save and return"};
    WCHAR only[64];
    swprintf(only,64,L"External only: %ls",pending.only?L"On":L"Off");
    for (int id=1;id<=10;id++) {
        void *text=item_text(menu,id);
        if (!text) continue;
        if (server_page) raw_text(text,id==4?only:id==5?(pending.enabled?L"Disable networking":L"Enable networking"):labels[id]);
        else if (id==SERVERS_ID) raw_text(text,L"Servers");
        else key_text(text,original_labels[id]);
        void *item=((void **)menu)[62+id];
        /* Original initScreens: selectable width = half row height + text width. */
        ((float *)item)[12]=((float *)item)[13]*0.5f+((float *)text)[12];
    }
    void *title=((void **)menu)[36];
    if (readable(title,484)) {
        if (server_page) raw_text(title,heading);
        else {
            TitleFn get_title=(TitleFn)(*(void ***)menu)[13];
            key_text(title,get_title(menu));
        }
    }
    keep_servers_available(menu);
}

static void __thiscall hook_init(void *menu,void *screen,void *lower) {
    original_init(menu,screen,lower);
    page_owner=menu; server_page=FALSE; render(menu);
    log_line(item_text(menu,SERVERS_ID)?"Engine main menu initialized; Friends replaced with Servers."
        :"Engine main menu has no Friends item; Servers entry unavailable.");
}

static void __thiscall hook_cleanup(void *menu,void *screen,void *lower) {
    if (page_owner==menu) { server_page=FALSE; page_owner=NULL; }
    original_cleanup(menu,screen,lower);
}

static void __thiscall hook_input(void *menu,int selection) {
    if (!server_page || page_owner!=menu) {
        if (selection!=SERVERS_ID) { original_input(menu,selection); return; }
        page_owner=menu; load_settings(); server_page=TRUE;
        wcscpy(heading,L"Servers — applies after restart"); render(menu);
        log_line("Engine Servers page opened."); return;
    }
    switch (selection) {
    case 2:
        pending.external[0]=0; wcscpy(pending.fallback,L"http://127.0.0.1:5144"); pending.only=FALSE; pending.enabled=TRUE;
        wcscpy(heading,L"Local selected — Save to apply after restart"); break;
    case 3: {
        Settings selected=pending; load_settings();
        wcscpy(selected.external,pending.external); pending=selected; pending.only=TRUE; pending.enabled=TRUE;
        wcscpy(heading,*pending.external?L"External selected — Save to apply after restart":L"No external URL configured in server.ini");
        break;
    }
    case 4: pending.only=!pending.only; break;
    case 5:
        pending.enabled=!pending.enabled;
        wcscpy(heading,pending.enabled?L"Networking enabled — Save to apply after restart":L"Networking disabled — addresses retained"); break;
    case 6: case 10:
        if (save_settings()) {
            wcscpy(heading,L"Saved — restart game to apply");
            if (selection==10) server_page=FALSE;
            log_line("Server settings saved; active endpoint unchanged.");
        } else { wcscpy(heading,L"Save failed — original settings retained"); log_line("Server settings save failed."); }
        break;
    case 7: load_settings(); wcscpy(heading,L"Reloaded — applies after restart"); break;
    case 9: case 2000: server_page=FALSE; break;
    default: return; /* Do not forward page selections into gameplay/quit handlers. */
    }
    render(menu);
}

static BOOL supported_file(const WCHAR *path) {
    static const char expected[]="2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2";
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (file==INVALID_HANDLE_VALUE) return FALSE;
    HCRYPTPROV provider=0; HCRYPTHASH hash=0; BOOL ok=FALSE;
    BYTE buffer[32768],digest[32]; DWORD count,size=32; char hex[65]; LARGE_INTEGER bytes;
    if (GetFileSizeEx(file,&bytes) && bytes.QuadPart==8635392
        && CryptAcquireContextW(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)
        && CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)) {
        BOOL reading=TRUE;
        for (;;) {
            if (!ReadFile(file,buffer,sizeof(buffer),&count,NULL)) { reading=FALSE; break; }
            if (!count) break;
            if (!CryptHashData(hash,buffer,count,0)) { reading=FALSE; break; }
        }
        if (reading && CryptGetHashParam(hash,HP_HASHVAL,digest,&size,0)) {
            for (int i=0;i<32;i++) snprintf(hex+i*2,3,"%02x",digest[i]);
            ok=!strcmp(hex,expected);
        }
    }
    if (hash) CryptDestroyHash(hash);
    if (provider) CryptReleaseContext(provider,0);
    CloseHandle(file); return ok;
}

/* All addresses are RVAs from original bytes, not Ghidra's inferred prototypes. */
static BOOL validate_image_at(BYTE *base,uintptr_t address_base) {
    static const DWORD entries[27]={0x340aa4,0x33de0a,0x33eddc,0x340a54,0x1f5891,
        0x340358,0x33ee99,0x33de10,0x1f58a5,0x2da614,0x2dd174,0x1f58b9,0x1f58cd,
        0x340abe,0x2da506,0x33fd14,0x33f23c,0x2d970f,0x2d979e,0x2d97f8,0x2d97fe,
        0x2d9e24,0x2d9e84,0x2d9dc4,0x33e88c,0x33de16,0x33e504};
    if (!readable(base+0x7a7470,sizeof(entries))) return FALSE;
    void **table=(void **)(base+0x7a7470);
    for (int i=0;i<27;i++) if ((uintptr_t)table[i]!=address_base+entries[i]) return FALSE;
    const DWORD slots[4]={3,15,16,24}, targets[4]={0x7a74f8,0x7a74f0,0x7a74ec,0x7a74e0};
    for (int i=0;i<4;i++) {
        BYTE *thunk=base+entries[slots[i]];
        if (!readable(thunk,6) || thunk[0]!=0xff || thunk[1]!=0x25
            || *(DWORD *)(thunk+2)!=(DWORD)(address_base+targets[i])) return FALSE;
    }
    static const BYTE raw_start[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x08};
    static const BYTE key_start[]={0x55,0x8b,0xec,0x53,0x56,0x8b,0x75,0x08};
    static const BYTE enable_start[]={0x55,0x8b,0xec,0x83,0xec,0x08,0x53,0x8a,0x5d,0x08};
    return readable(base+0x50930,sizeof(raw_start)) && readable(base+0x508a0,sizeof(key_start))
        && readable(base+0x4b4a0,sizeof(enable_start))
        && !memcmp(base+0x50930,raw_start,sizeof(raw_start))
        && !memcmp(base+0x508a0,key_start,sizeof(key_start))
        && !memcmp(base+0x4b4a0,enable_start,sizeof(enable_start));
}

static BOOL validate_image(BYTE *base) { return validate_image_at(base,(uintptr_t)base); }

__declspec(dllexport) BOOL __cdecl ReRiffStartEngineMenu(void) {
    if (InterlockedCompareExchange(&installed,1,0)) return installed==2;
    WCHAR path[MAX_PATH]; DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    if (!length || length>=MAX_PATH) return FALSE;
    WCHAR *slash=wcsrchr(path,L'\\');
    if (!slash || (size_t)(slash-path)+30>=MAX_PATH) return FALSE;
    wcscpy(config_path,path); config_path[slash-path+1]=0;
    wcscpy(log_path,config_path); wcscat(log_path,L"ReRiff-engine-menu.log");
    wcscat(config_path,L"server.ini");
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if (!supported_file(path) || !validate_image(base)) {
        log_line("Refused: unsupported executable or unexpected menu bytes."); return FALSE;
    }
    HMODULE pinned;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (const WCHAR *)(uintptr_t)&ReRiffStartEngineMenu,&pinned)) return FALSE;
    void **table=(void **)(base+0x7a7470); DWORD protection;
    if (!VirtualProtect(table,27*sizeof(void *),PAGE_READWRITE,&protection)) return FALSE;
    original_init=(ScreenFn)table[15]; original_cleanup=(ScreenFn)table[16]; original_input=(InputFn)table[24];
    original_tick=(TickFn)table[3]; enable_input=(EnableFn)(base+0x4b4a0);
    raw_text=(RawTextFn)(base+0x50930); key_text=(KeyTextFn)(base+0x508a0);
    original_labels=(const char **)(base+0x7a7434);
    InterlockedExchangePointer(&table[3],(void *)&hook_tick);
    InterlockedExchangePointer(&table[15],(void *)&hook_init);
    InterlockedExchangePointer(&table[16],(void *)&hook_cleanup);
    InterlockedExchangePointer(&table[24],(void *)&hook_input);
    DWORD ignored; VirtualProtect(table,27*sizeof(void *),protection,&ignored);
    InterlockedExchange(&installed,2); log_line("Engine menu hooks installed."); return TRUE;
}

/* Menu startup and the optional existing server toast have separate jobs. */
__declspec(dllexport) void __cdecl ReRiffShowServerToastW(const WCHAR *endpoint) {
    ReRiffStartEngineMenu(); NativeServerToastW(endpoint);
}

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    return NotificationDllMain(instance,reason,reserved);
}
