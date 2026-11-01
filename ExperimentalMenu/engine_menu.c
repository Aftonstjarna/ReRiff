/* Engine-rendered experiment for the hash-pinned original x86 Game.exe.
 * Runs on the game's own menu callbacks and reads its existing input device. */
#ifndef UNICODE
#define UNICODE
#endif
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

#ifndef RERIFF_VERSION
#define RERIFF_VERSION "development"
#endif
#define MENU_WIDE_(x) L##x
#define MENU_WIDE(x) MENU_WIDE_(x)

typedef void (__thiscall *ScreenFn)(void *,void *,void *);
typedef void (__thiscall *InputFn)(void *,int);
typedef void (__thiscall *TickFn)(void *);
typedef void (__thiscall *EnableFn)(void *,BYTE);
typedef void (__thiscall *ColourFn)(void *,int,const float *);
typedef void (__thiscall *RawTextFn)(void *,const WCHAR *);
typedef void (__thiscall *KeyTextFn)(void *,const char *);
typedef const char *(__thiscall *TitleFn)(void *);
typedef struct Settings { WCHAR external[2048], fallback[2048]; BOOL only, enabled, skip_launcher; } Settings;
static ScreenFn original_init, original_cleanup;
static InputFn original_input;
static TickFn original_tick, base_tick;
static EnableFn enable_input;
static RawTextFn raw_text;
static KeyTextFn key_text;
/* createRaw is cdecl. add/screen.add take an owning SSP by VALUE: their
 * native bodies read the object at [ebp+8], release it, and return with RET 4. */
typedef void *(__cdecl *CreateTextFn)(const WCHAR *,float,float,unsigned,int,int,float,BOOL,float,BOOL,BOOL);
typedef void (__thiscall *AddFn)(void *,void *);
typedef void (__thiscall *FocusFn)(void *,int);
static CreateTextFn create_text;
static AddFn add_item, add_screen, remove_screen;
static EnableFn menu_item;
static FocusFn set_focus;
typedef void *(__thiscall *SpriteFn)(void *,int,const float *,int,int);
typedef void (__thiscall *SpriteDefFn)(void *,unsigned);
typedef void (__thiscall *PositionFn)(void *,const float *);
static SpriteFn create_sprite;
static SpriteDefFn sprite_def, sprite_origin;
static PositionFn sprite_position;
static void **ui_instance;
static ColourFn original_colour;
static void *toggles[3][2];
static const int toggle_actions[3]={1,4,5};
static void *server_rows[11]; /* borrowed; the original screen owns every row */
static BYTE settings_visible, audio_visible;
static float server_row_width, server_row_height;
static WCHAR config_path[MAX_PATH], log_path[MAX_PATH];
static Settings pending;
static void *page_owner;
static BOOL server_page;
static unsigned input_trace_count;

static WCHAR heading[96]=L"Servers — applies after restart";
static volatile LONG installed;
static BOOL restarting;
static DWORD restart_started;
#include "../NativeLauncher/restart_protocol.h"
#define SERVERS_ID 1500
#define SERVER_ACTION_BASE 1500

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
    GetPrivateProfileStringW(L"launcher",L"skip_launcher",L"false",value,32,config_path);
    pending.skip_launcher=!_wcsicmp(value,L"true") || !_wcsicmp(value,L"yes") || !wcscmp(value,L"1");
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
        && WritePrivateProfileStringW(L"server",L"networking_enabled",pending.enabled?L"true":L"false",temp)
        && WritePrivateProfileStringW(L"launcher",L"skip_launcher",pending.skip_launcher?L"true":L"false",temp);
    if (ok) {
        WritePrivateProfileStringW(NULL,NULL,NULL,temp);
        ok=MoveFileExW(temp,config_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
        WritePrivateProfileStringW(NULL,NULL,NULL,config_path);
    }
    if (!ok) DeleteFileW(temp);
    return ok;
}

static BOOL restart_game(void) {
    Toast target;ZeroMemory(&target,sizeof(target));
    EnumWindows(find_game,(LPARAM)&target);
    if(!target.game)return FALSE;
    WCHAR name[96];DWORD pid=GetCurrentProcessId();
    restart_event_name(name,96,pid,FALSE);
    HANDLE request=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
    if(!request){
        request=CreateEventW(NULL,TRUE,FALSE,name);
        restart_event_name(name,96,pid,TRUE);
        HANDLE ready=CreateEventW(NULL,TRUE,FALSE,name);
        WCHAR folder[MAX_PATH],launcher[MAX_PATH];
        DWORD count=GetModuleFileNameW(NULL,folder,MAX_PATH);
        WCHAR *slash=count&&count<MAX_PATH?wcsrchr(folder,L'\\'):NULL;
        if(!request||!ready||!slash||((size_t)(slash-folder)+24>=MAX_PATH)){
            if(request)CloseHandle(request);if(ready)CloseHandle(ready);return FALSE;
        }
        slash[1]=0;wcscpy(launcher,folder);wcscat(launcher,L"ReRiff-Launcher.exe");
        /* Keep the original argument quoting and Steam environment. */
        static WCHAR command[32768];
        int used=_snwprintf(command,32768,L"\"%ls\" --restart-game %lu --skip-launcher -- %ls",
            launcher,(unsigned long)pid,GetCommandLineW());
        STARTUPINFOW start;PROCESS_INFORMATION child;ZeroMemory(&start,sizeof(start));start.cb=sizeof(start);
        start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;
        BOOL started=used>0&&used<32768&&CreateProcessW(launcher,command,NULL,NULL,FALSE,0,NULL,folder,&start,&child);
        if(started){CloseHandle(child.hThread);CloseHandle(child.hProcess);}
        BOOL armed=started&&WaitForSingleObject(ready,5000)==WAIT_OBJECT_0;
        CloseHandle(ready);
        if(!armed){CloseHandle(request);return FALSE;}
    }
    BOOL ok=SetEvent(request)&&PostMessageW(target.game,WM_CLOSE,0,0);
    if(!ok)ResetEvent(request);
    CloseHandle(request);
    return ok;
}
static BOOL (*request_restart)(void)=restart_game;

static BOOL readable(const void *p,size_t count) {
    MEMORY_BASIC_INFORMATION info;
    if (!p || !VirtualQuery(p,&info,sizeof(info)) || info.State!=MEM_COMMIT
        || (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return FALSE;
    uintptr_t start=(uintptr_t)p, end=(uintptr_t)info.BaseAddress+info.RegionSize;
    return start<=end && count<=end-start;
}

static void show_item(void *item,BYTE visible) {
    if(readable(item,404))((EnableFn)(*(void ***)item)[3])(item,visible);
}
static void focus_row(void *menu,int id) {
    void *screen=((void **)menu)[46];
    if(readable(screen,16))set_focus(screen,id);
}
static void *item_text(void *menu,int id) {
    return page_owner==menu&&id>=0&&id<=10?server_rows[id]:NULL;
}
static void keep_servers_available(void *menu) {
    if(!server_page||page_owner!=menu)return;
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    if(!mask)return;
    for(int id=1;id<=10;id++){
        void *row=server_rows[id];
        /* Never clear a live mouse/controller interaction latch each frame. */
        if(readable(row,404)&&(*((BYTE *)row+372)&1)&&*(DWORD *)((BYTE *)row+76)!=mask){enable_input(row,TRUE);*(DWORD *)((BYTE *)row+76)=mask;}
    }
}

static void toggle_visibility(BYTE visible) {
    for(int i=0;i<3;i++)for(int part=0;part<2;part++)show_item(toggles[i][part],visible);
}
static void update_toggles(void *menu) {
    const BOOL values[3]={pending.skip_launcher,pending.only,!pending.enabled};
    for(int i=0;i<3;i++){
        void *bar=toggles[i][0],*knob=toggles[i][1];
        if(!bar||!knob)continue;
        BOOL on=values[i];
        /* Original gaGraphicsOptions.setTogglebarVars: assets 74/75,
         * right/left origin, bar-width displacement and 25*scale inset. */
        sprite_def(knob,on?74:75);sprite_origin(knob,on?5:3);
        float pos[2]={((float *)bar)[8],((float *)bar)[9]},scale=((float *)menu)[15];
        if(!on)pos[0]-=((float *)bar)[12]*scale;
        pos[0]+=(on?-25.0f:25.0f)*scale;
        sprite_position(knob,pos);
    }
}
#include "settings_scroll.h"

static void __thiscall hook_colour(void *menu,int id,const float *colour) {
    original_colour(menu,id,colour);
    if(!server_page||page_owner!=menu)return;
    for(int i=0;i<3;i++)if(id==SERVER_ACTION_BASE+toggle_actions[i])
        for(int part=0;part<2;part++){
            void *item=toggles[i][part];
            if(item)((void (__thiscall *)(void *,const float *))(*(void ***)item)[4])(item,colour);
        }
}

static void render(void *menu);
static void __thiscall hook_tick(void *menu) {
    if(restarting&&GetTickCount()-restart_started>15000){
        WCHAR name[96];restart_event_name(name,96,GetCurrentProcessId(),FALSE);
        HANDLE request=OpenEventW(EVENT_MODIFY_STATE,FALSE,name);
        if(request){ResetEvent(request);CloseHandle(request);}
        restarting=FALSE;wcscpy(heading,L"Saved — restart canceled; game still running");
        log_line("Save and restart: game did not close; request canceled.");render(menu);
    }
    prepare_scroll(menu);
    BOOL navigating=navigate_direction(menu,direction_input(),GetTickCount());
    dispatching_menu=TRUE;
    if(!navigating){
        if(active_list(menu)){
            /* The original options tick runs audio-slider hit tests and drag
             * latches even on the root page. A scrolling root uses the same
             * base menu dispatch as Servers; Audio retains the original tick. */
            if(!server_page){*((BYTE *)menu+392)=0;*((BYTE *)menu+393)=0;}
            keep_servers_available(menu);base_tick(menu);
        }
        else original_tick(menu);
    }
    dispatching_menu=FALSE;
    sync_scroll(menu);attach_scroll_window();keep_servers_available(menu);
}

static void leave_server_page(void *menu) {
    for(int id=1;id<=10;id++)show_item(server_rows[id],FALSE);
    toggle_visibility(FALSE);
    show_item(((void **)menu)[67],settings_visible);
    show_item(((void **)menu)[83],audio_visible);
    server_page=FALSE;
    InterlockedExchange(&scroll_delta,0);InterlockedExchange(&scroll_requested,-1);
    root_first=0;logical_selection=0;pending_focus=SERVERS_ID;layout_scroll(menu);
    if(!dispatching_menu){prepare_scroll(menu);apply_pending_focus(menu);}
}

static void render(void *menu) {
    static const WCHAR *labels[11]={L"", L"Skip launcher",
        L"Use local server", L"Use configured external server", L"External only",
        L"Force Offline", L"Save settings", L"Reload saved settings",
        L"Save and restart", L"Back without saving", L"Save and return"};
    for(int id=1;id<=10;id++) {
        void *text=item_text(menu,id);
        if(!text)continue;
        raw_text(text,labels[id]);
        /* replaceTextRaw recomputes glyph bounds. Keep the original Settings
         * hit area so clicking the switch also activates its labelled row. */
        ((float *)text)[12]=server_row_width;((float *)text)[13]=server_row_height;
    }
    void *title=((void **)menu)[36];
    if(readable(title,484)) {
        if(server_page)raw_text(title,heading);
        else {
            TitleFn get_title=(TitleFn)(*(void ***)menu)[13];
            key_text(title,get_title(menu));
        }
    }
    layout_scroll(menu);
    keep_servers_available(menu);
}

static void *make_row(void *parent,BOOL screen,int id,float x,float y,float width,float height,BOOL visible) {
    BOOL toggle=id==1501||id==1504||id==1505;
    void *row=create_text(id==SERVERS_ID?L"Servers":L"",toggle?x-width*0.5f:x,y,3,toggle?0:1,0,-1.0f,FALSE,-1.0f,FALSE,TRUE);
    if(!row)return NULL;
    ((int *)row)[2]=id;((int *)row)[15]=100;
    menu_item(row,TRUE);
    static const float normal[4]={1,1,1,0.9f};
    ((void (__thiscall *)(void *,const float *))(*(void ***)row)[4])(row,normal);
    ((float *)row)[12]=width;((float *)row)[13]=height;
    show_item(row,visible);
    /* Match ILSpy toItem and native add's consuming SSP argument. */
    ((int *)row)[1]++;
    (screen?add_screen:add_item)(parent,row);
    return row;
}
static BOOL make_toggles(void *menu,void *screen,float x,float width,float height) {
    if(!readable(ui_instance,sizeof(void *))||!*ui_instance)return FALSE;
    for(int i=0;i<3;i++){
        void *text=server_rows[toggle_actions[i]];
        float pos[2]={x+width*0.5f,((float *)text)[9]};
        for(int part=0;part<2;part++){
            void *item=create_sprite(*ui_instance,part?74:73,pos,part?0:5,-1);
            if(!item)return FALSE;
            toggles[i][part]=item;
            if(!part){
                /* Fullscreen bar uses frame 1 for every interaction state. */
                ((int *)item)[18]=0;((int *)item)[38]=1;
                ((int *)item)[39]=1;((int *)item)[40]=1;
            }
            ((void (__thiscall *)(void *,const float *))(*(void ***)item)[9])(item,(const float *)menu+14);
            ((int *)item)[15]=100;
            static const float normal[4]={1,1,1,0.9f};
            ((void (__thiscall *)(void *,const float *))(*(void ***)item)[4])(item,normal);
            menu_item(item,FALSE);enable_input(item,FALSE);show_item(item,FALSE);
            ((int *)item)[1]++;add_screen(screen,item);
        }
        /* The whole labelled row accepts mouse/controller activation. The
         * original toggle graphics cannot steal focus or dispatch a second click. */
        ((float *)text)[12]=width;((float *)text)[13]=height;
    }
    return TRUE;
}
static void __thiscall hook_init(void *menu,void *screen,void *lower) {
    original_init(menu,screen,lower);
    page_owner=menu;server_page=FALSE;input_trace_count=0;ZeroMemory(server_rows,sizeof(server_rows));ZeroMemory(toggles,sizeof(toggles));
    void *group=((void **)menu)[67],*ui=((void **)menu)[46],*sample=NULL;
    for(int id=0;id<5&&!sample;id++)sample=((void **)menu)[68+id];
    if(!readable(group,404)||!readable(ui,16)||!readable(sample,484)){
        log_line("Settings menu has no supported row layout; Servers entry not added.");return;
    }
    float x=((float *)sample)[8];
    float gap=((float *)menu)[66],width=((float *)sample)[12],row_height=((float *)sample)[13];
    server_row_width=width;server_row_height=row_height;
    /* Submenu positions are laid out at the original Settings row interval. */
    for(int id=1;id<=10;id++){
        server_rows[id]=make_row(ui,TRUE,SERVER_ACTION_BASE+id,x,((float *)sample)[9]+(id-1)*gap,width,row_height,FALSE);
        if(!server_rows[id]){log_line("Could not create complete Servers page; original Settings retained.");return;}
    }
    if(!make_toggles(menu,ui,x,width,row_height)){
        log_line("Could not create native fullscreen toggles; original Settings retained.");return;
    }
    server_rows[0]=make_row(group,FALSE,SERVERS_ID,x,((float *)sample)[9],width,row_height,FALSE);
    if(server_rows[0]&&!init_scroll(menu,sample)){
        show_item(server_rows[0],FALSE);log_line("Could not initialize Settings scrolling.");return;
    }
    log_line(server_rows[0]?"Native Settings initialized; Servers added; Friends unchanged.":"Could not add Servers row.");
}
static void __thiscall hook_cleanup(void *menu,void *screen,void *lower) {
    if(page_owner==menu){
        /* gaMenu can retain its screen across visits. Remove screen-owned
         * controls explicitly so re-entry cannot leave duplicate IDs behind. */
        cleanup_scroll(menu);
        void *ui=((void **)menu)[46];
        for(int id=1;id<=10;id++)if(server_rows[id]){
            ((int *)server_rows[id])[1]++;
            remove_screen(ui,server_rows[id]);
        }
        for(int i=0;i<3;i++)for(int part=0;part<2;part++)if(toggles[i][part]){
            ((int *)toggles[i][part])[1]++;remove_screen(ui,toggles[i][part]);
        }
        server_page=FALSE;page_owner=NULL;ZeroMemory(server_rows,sizeof(server_rows));ZeroMemory(toggles,sizeof(toggles));
    }
    original_cleanup(menu,screen,lower);
}

static void __thiscall hook_input(void *menu,int selection) {
    if(restarting)return;
    if(page_owner==menu&&!server_page&&((int *)menu)[62]==0&&selection>=0&&selection<5){
        /* These latches belong only to Audio. The original handler rejects
         * every root selection if either stale latch remains set. */
        *((BYTE *)menu+392)=0;*((BYTE *)menu+393)=0;
        if(input_trace_count++<64){
            char event[192];snprintf(event,sizeof(event),"Settings activate id=%d focus=%d viewport=%d transition=%d",selection,((int *)menu)[11],root_first,((int *)menu)[102]);
            log_line(event);
        }
    }
    if (!server_page || page_owner!=menu) {
        if(selection!=SERVERS_ID||page_owner!=menu||!server_rows[0]||((int *)menu)[62]!=0||((int *)menu)[102]!=0||!(*((BYTE *)server_rows[0]+372)&1)){
            original_input(menu,selection);return;
        }
        void *group=((void **)menu)[67],*audio=((void **)menu)[83];
        settings_visible=readable(group,404)?*((BYTE *)group+372)&1:0;
        audio_visible=readable(audio,404)?*((BYTE *)audio+372)&1:0;
        show_item(group,FALSE);show_item(audio,FALSE);
        load_settings();server_page=TRUE;page_first=0;logical_selection=0;pending_focus=SERVER_ACTION_BASE+1;
        InterlockedExchange(&scroll_delta,0);InterlockedExchange(&scroll_requested,-1);
        for(int id=1;id<=10;id++)show_item(server_rows[id],TRUE);
        toggle_visibility(TRUE);
        wcscpy(heading,L"Servers — applies after restart");render(menu);if(!dispatching_menu){prepare_scroll(menu);apply_pending_focus(menu);}
        log_line("Native Settings > Servers opened.");return;
    }
    if(selection!=2000){
        selection-=SERVER_ACTION_BASE;
        if(selection<1||selection>10)return;
        if(scroll_ready&&(selection-1<page_first||selection-1>=page_first+visible_rows))return;
    }
    switch (selection) {
    case 1:
        pending.skip_launcher=!pending.skip_launcher;
        wcscpy(heading,pending.skip_launcher?L"Skip launcher enabled — Save to apply":L"Show launcher enabled — Save to apply"); break;
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
        wcscpy(heading,pending.enabled?L"Force Offline off — networking enabled after restart":L"Force Offline on — addresses retained; Save to apply"); break;
    case 6: case 10:
        if (save_settings()) {
            wcscpy(heading,L"Saved — restart game to apply");
            if (selection==10) leave_server_page(menu);
            log_line("Server settings saved; active endpoint unchanged.");
        } else { wcscpy(heading,L"Save failed — original settings retained"); log_line("Server settings save failed."); }
        break;
    case 7: load_settings(); wcscpy(heading,L"Reloaded — applies after restart"); break;
    case 8:
        if(!save_settings()){
            wcscpy(heading,L"Save failed — game not restarted");log_line("Save and restart: save failed.");
        }else if(request_restart()){
            restarting=TRUE;restart_started=GetTickCount();
            wcscpy(heading,L"Saved — restarting game...");log_line("Save and restart requested.");
        }else{
            wcscpy(heading,L"Saved — could not restart game");log_line("Save and restart: handoff failed; game left running.");
        }
        break;
    case 9: case 2000: leave_server_page(menu); break;
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

#include "antialiasing.h"
#include "graphics_aa_menu.h"
#include "main_branding.h"

/* All addresses are RVAs from original bytes, not Ghidra's inferred prototypes. */
static BOOL validate_image_at(BYTE *base,uintptr_t address_base) {
    if(!validate_main_branding(base,address_base))return FALSE;
    static const BYTE base_tick_start[]={0xff,0x25};
    if(!readable(base+0x7a56d0,4)||*(DWORD *)(base+0x7a56d0)!=(DWORD)(address_base+0x2e28f0)
        ||!readable(base+0x2e28f0,6)||memcmp(base+0x2e28f0,base_tick_start,2)
        ||*(DWORD *)(base+0x2e28f2)!=(DWORD)(address_base+0x7a5734))return FALSE;
    if(!readable(base+0x7a5740,37*sizeof(void *))||*(DWORD *)(base+0x7a5740)!=(DWORD)(address_base+0x2e50e0))return FALSE;
    if(!readable(base+0x2e50e0,6)||memcmp(base+0x2e50e0,base_tick_start,2)||*(DWORD *)(base+0x2e50e2)!=(DWORD)(address_base+0x7a57d4))return FALSE;
    static const DWORD entries[27]={0x2f10f4,0x2eea7a,0x2f0730,0x2f10a4,0x1f5891,
        0x2d96a2,0x2eea74,0x2eea80,0x1f58a5,0x2da614,0x2dd174,0x1f58b9,0x1f58cd,
        0x2f10fa,0x2da506,0x2eff50,0x2f0408,0x2d970f,0x2d979e,0x2d97f8,0x2d97fe,
        0x2d9e24,0x2d9e84,0x2d9dc4,0x2f08e3,0x2eea86,0x2ef1e0};
    if (!readable(base+0x7a5a7c,sizeof(entries))) return FALSE;
    void **table=(void **)(base+0x7a5a7c);
    for (int i=0;i<27;i++) if ((uintptr_t)table[i]!=address_base+entries[i]) return FALSE;
    const DWORD slots[5]={3,15,16,24,26}, targets[5]={0x7a5afc,0x7a5aec,0x7a5af0,0x7a5af8,0x7a5ae8};
    for (int i=0;i<5;i++) {
        BYTE *thunk=base+entries[slots[i]];
        if (!readable(thunk,6) || thunk[0]!=0xff || thunk[1]!=0x25
            || *(DWORD *)(thunk+2)!=(DWORD)(address_base+targets[i])) return FALSE;
    }
    /* Original native entry bytes for newly used Settings UI routines. */
    static const BYTE entry_0[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0xfc,0xfb,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x50af0,sizeof(entry_0))||memcmp(base+0x50af0,entry_0,sizeof(entry_0)))return FALSE;
    static const BYTE entry_1[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x08,0xf4,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x43800,sizeof(entry_1))||memcmp(base+0x43800,entry_1,sizeof(entry_1)))return FALSE;
    static const BYTE entry_2[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x18,0x19,0x90,0x00,0x64,0xa1};
    if(!readable(base+0x8e8d0,sizeof(entry_2))||memcmp(base+0x8e8d0,entry_2,sizeof(entry_2)))return FALSE;
    static const BYTE entry_3[]={0x55,0x8b,0xec,0x83,0xe4,0xf8,0x6a,0xff,0x68,0x46,0xf7,0x8f};
    if(!readable(base+0x4b530,sizeof(entry_3))||memcmp(base+0x4b530,entry_3,sizeof(entry_3)))return FALSE;
    static const BYTE entry_4[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x08,0xf4,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x8f580,sizeof(entry_4))||memcmp(base+0x8f580,entry_4,sizeof(entry_4)))return FALSE;
    static const BYTE remove_start[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x38,0xf4,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x8e9d0,sizeof(remove_start))||memcmp(base+0x8e9d0,remove_start,sizeof(remove_start)))return FALSE;
    static const BYTE toggle_0[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0x74,0xdf,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x22030,sizeof(toggle_0))||memcmp(base+0x22030,toggle_0,sizeof(toggle_0)))return FALSE;
    static const BYTE toggle_1[]={0x55,0x8b,0xec,0xa1,0xa4,0xba,0xbf,0x00,0x8b,0x55,0x08,0x56};
    if(!readable(base+0x48830,sizeof(toggle_1))||memcmp(base+0x48830,toggle_1,sizeof(toggle_1)))return FALSE;
    static const BYTE toggle_2[]={0x55,0x8b,0xec,0x6a,0xff,0x68,0xa8,0xf5,0x8f,0x00,0x64,0xa1};
    if(!readable(base+0x482b0,sizeof(toggle_2))||memcmp(base+0x482b0,toggle_2,sizeof(toggle_2)))return FALSE;
    static const BYTE toggle_3[]={0x55,0x8b,0xec,0x8b,0x55,0x08,0x8b,0x02,0x89,0x41,0x20,0x8b};
    if(!readable(base+0x48280,sizeof(toggle_3))||memcmp(base+0x48280,toggle_3,sizeof(toggle_3)))return FALSE;
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

__declspec(dllexport) BOOL __cdecl ReRiffConfigureNativeReel(void **methods,int count) {
    WCHAR path[MAX_PATH];DWORD length=GetModuleFileNameW(NULL,path,MAX_PATH);
    if(!length||length>=MAX_PATH||!supported_file(path)||count!=7||!readable(methods,7*sizeof(void *)))return FALSE;
    for(int i=0;i<7;i++)if(!methods[i])return FALSE;
    reel_ctor=(ReelCtorFn)methods[0];reel_add=(ReelAddFn)methods[1];reel_limit=(ReelLimitFn)methods[2];
    reel_tick=(ReelTickFn)methods[3];reel_move=(ReelMoveFn)methods[4];reel_selection=(ReelSelectionFn)methods[5];reel_bar=(ReelBarFn)methods[6];
    bridge_ready=TRUE;return TRUE;
}

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
    if(!bridge_ready){log_line("Native file-picker bridge unavailable; update ReRiff.ClientConfig.dll with this menu DLL.");return FALSE;}
    HMODULE pinned;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        (const WCHAR *)(uintptr_t)&ReRiffStartEngineMenu,&pinned)) return FALSE;
    void **table=(void **)(base+0x7a5a7c); DWORD protection;
    if (!VirtualProtect(table,27*sizeof(void *),PAGE_READWRITE,&protection)) return FALSE;
    original_init=(ScreenFn)table[15]; original_cleanup=(ScreenFn)table[16]; original_input=(InputFn)table[24];
    original_colour=(ColourFn)table[26];
    original_tick=(TickFn)table[3]; enable_input=(EnableFn)(base+0x4b4a0);
    base_tick=(TickFn)(base+0x2e28f0);
    raw_text=(RawTextFn)(base+0x50930); key_text=(KeyTextFn)(base+0x508a0);
    create_text=(CreateTextFn)(base+0x50af0);
    game_allocate=*(void *(__cdecl **)(size_t))(base+0x52d4b4);
    reel_vtable=(void **)(base+0x7a5740);input_manager=(void **)(base+0x80d410);
    create_sprite=(SpriteFn)(base+0x22030);ui_instance=(void **)(base+0x7fbaa4);
    sprite_def=(SpriteDefFn)(base+0x48830);sprite_origin=(SpriteDefFn)(base+0x482b0);
    sprite_position=(PositionFn)(base+0x48280);
    add_item=(AddFn)(base+0x43800);add_screen=(AddFn)(base+0x8e8d0);remove_screen=(AddFn)(base+0x8e9d0);
    menu_item=(EnableFn)(base+0x4b530);set_focus=(FocusFn)(base+0x8f580);
    InterlockedExchangePointer(&table[3],(void *)&hook_tick);
    InterlockedExchangePointer(&table[15],(void *)&hook_init);
    InterlockedExchangePointer(&table[16],(void *)&hook_cleanup);
    InterlockedExchangePointer(&table[24],(void *)&hook_input);
    InterlockedExchangePointer(&table[26],(void *)&hook_colour);
    DWORD ignored; VirtualProtect(table,27*sizeof(void *),protection,&ignored);
    if(!install_graphics_aa_menu(base))log_line("Native antialiasing control could not be installed.");
    if(!install_main_branding(base))log_line("Main-menu branding could not be installed.");
    InterlockedExchange(&installed,2); log_line("Engine menu hooks installed."); return TRUE;
}

/* Menu startup and the optional existing server toast have separate jobs. */
__declspec(dllexport) void __cdecl ReRiffShowServerToastW(const WCHAR *endpoint) {
    ReRiffStartEngineMenu(); NativeServerToastW(endpoint);
}

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    return NotificationDllMain(instance,reason,reserved);
}
