/* Launcher Controls page: Keyboard, Controller and Hotkeys subtabs.
 * Values are stored in the same client.ini read by the native game hook and
 * the SDL3 XInput proxy. Controller rows capture physical SDL gamepad
 * coordinates (button index or axis index + direction) instead of using a
 * fixed dropdown list. */
#ifndef RERIFF_LAUNCHER_CONTROLS_H
#define RERIFF_LAUNCHER_CONTROLS_H
#include <stdlib.h>
static void checkbox_help(HWND,const WCHAR *);
static HWND controls_enable,controls_fov,controls_zoom,controls_invert_x,controls_invert_y;
static HWND controls_trace,controls_free_button,controls_free_key,controls_hud_button,controls_hud_key;
static HWND controls_mouse,controls_photo_enable,controls_photo_button,controls_photo_key,controls_photo_hud,controls_photo_speed,controls_photo_invert_strafe,controls_save,controls_first;static WCHAR controls_ini[CAP];
static HWND controls_controller_enabled;
#define CONTROLLER_BINDING_COUNT 21
static HWND controls_controller_bindings[CONTROLLER_BINDING_COUNT];

/* ------------------------------------------------------------------ */
/* Controller capture                                                  */
/* ------------------------------------------------------------------ */
#define CONTROLS_CAPTURE_TIMER 2
#define CONTROLS_BUTTON_COUNT 21
#define CONTROLS_AXIS_COUNT 6
#define BINDING_NONE 0
#define BINDING_BUTTON 1
#define BINDING_AXIS 2

typedef struct SDL_Gamepad SDL_Gamepad;
typedef unsigned int ControlsJoystickID;
typedef struct {
    HMODULE module;
    int (*Init)(unsigned int);
    void (*Quit)(void);
    void (*UpdateGamepads)(void);
    ControlsJoystickID *(*GetGamepads)(int *);
    void (*Free)(void *);
    SDL_Gamepad *(*OpenGamepad)(ControlsJoystickID);
    void (*CloseGamepad)(SDL_Gamepad *);
    int (*GamepadConnected)(SDL_Gamepad *);
    int (*GetGamepadButton)(SDL_Gamepad *,int);
    int (*GetGamepadAxis)(SDL_Gamepad *,int);
    int (*SetHint)(const char *,const char *);
} ControlsSdlApi;

static ControlsSdlApi controls_sdl;
static SDL_Gamepad *controls_pad;
static BOOL controls_sdl_ready,controls_sdl_attempted,controls_capture_wait_release;
static int controls_capture_row=-1;
static unsigned controls_capture_seen;
#define CONTROLS_MAX_CHORD 4
static int controls_binding_count[CONTROLLER_BINDING_COUNT];
static int controls_binding_kind[CONTROLLER_BINDING_COUNT][CONTROLS_MAX_CHORD];
static int controls_binding_index[CONTROLLER_BINDING_COUNT][CONTROLS_MAX_CHORD];
static int controls_binding_sign[CONTROLLER_BINDING_COUNT][CONTROLS_MAX_CHORD];

/* SDL_GamepadButton order. */
static const WCHAR *controls_button_names[CONTROLS_BUTTON_COUNT]={
    L"a",L"b",L"x",L"y",L"back",L"guide",L"start",L"l3",L"r3",L"lb",L"rb",
    L"dup",L"ddown",L"dleft",L"dright",L"misc1",L"paddle1",L"paddle2",L"paddle3",L"paddle4",L"touchpad"
};
static const WCHAR *controls_button_short[CONTROLS_BUTTON_COUNT]={
    L"A",L"B",L"X",L"Y",L"Back",L"Guide",L"Start",L"L3",L"R3",L"LB",L"RB",
    L"D-up",L"D-down",L"D-left",L"D-right",L"Misc",L"P1",L"P2",L"P3",L"P4",L"Touchpad"
};
/* SDL_GamepadAxis order. */
static const WCHAR *controls_axis_names[CONTROLS_AXIS_COUNT]={L"lx",L"ly",L"rx",L"ry",L"lt",L"rt"};
static const WCHAR *controls_axis_short[CONTROLS_AXIS_COUNT]={L"LX",L"LY",L"RX",L"RY",L"LT",L"RT"};

typedef struct { const WCHAR *key,*label,*def; } ControllerBindingRow;
/* Labels describe the vanilla action each virtual XInput control performs.
 * The capture button on the right shows the physical SDL coordinate. */
static const ControllerBindingRow controls_controller_rows[CONTROLLER_BINDING_COUNT]={
    {L"a",L"Boost",L"button:a"},{L"b",L"Cancel",L"button:b"},
    {L"x",L"Drift",L"button:x"},{L"y",L"Camera",L"button:y"},
    {L"lb",L"Steer L",L"button:lb"},{L"rb",L"Steer R",L"button:rb"},
    {L"back",L"Restart",L"button:back"},{L"start",L"Pause",L"button:start"},
    {L"l3",L"Photo chord",L"button:l3"},{L"r3",L"Free look",L"button:r3"},
    {L"guide",L"Guide",L"button:guide"},
    {L"dup",L"Menu up",L"button:dup"},{L"ddown",L"Menu down",L"button:ddown"},
    {L"dleft",L"Menu left",L"button:dleft"},{L"dright",L"Menu right",L"button:dright"},
    {L"lx",L"Steer",L"axis:lx"},{L"ly",L"Unused",L"axis:ly"},
    {L"rx",L"Look X",L"axis:rx"},{L"ry",L"Look Y",L"axis:ry"},
    {L"lt",L"Brake",L"axis:lt"},{L"rt",L"Accelerate",L"axis:rt"}
};

static int controls_name_index(const WCHAR *name,const WCHAR **names,int count){
    for(int i=0;i<count;i++)if(!_wcsicmp(name,names[i]))return i;
    return -1;
}
static BOOL controls_parse_binding_part(const WCHAR *text,int *kind,int *index,int *sign){
    *kind=BINDING_NONE;*index=0;*sign=1;
    const WCHAR *colon=wcschr(text,L':');
    if(!colon)return FALSE;
    WCHAR type[16];
    size_t length=(size_t)(colon-text);
    if(length>=sizeof(type)/sizeof(type[0]))return FALSE;
    memcpy(type,text,length*sizeof(WCHAR));type[length]=0;
    WCHAR name[32];
    wcsncpy(name,colon+1,31);name[31]=0;
    size_t used=wcslen(name);
    if(used&&(name[used-1]==L'+'||name[used-1]==L'-')){
        *sign=name[used-1]==L'-'?-1:1;
        name[--used]=0;
    }
    int found;
    if(!_wcsicmp(type,L"button")){
        found=controls_name_index(name,controls_button_names,CONTROLS_BUTTON_COUNT);
        if(found<0)return FALSE;
        *kind=BINDING_BUTTON;*index=found;return TRUE;
    }
    if(!_wcsicmp(type,L"axis")){
        found=controls_name_index(name,controls_axis_names,CONTROLS_AXIS_COUNT);
        if(found<0)return FALSE;
        *kind=BINDING_AXIS;*index=found;return TRUE;
    }
    return FALSE;
}
/* The next chord separator is a '+' followed by another control, so an axis
 * direction suffix such as "axis:ly-" or "axis:lx+" is not split. */
static const WCHAR *controls_next_separator(const WCHAR *text){
    for(const WCHAR *cursor=text;*cursor;cursor++){
        if(*cursor==L'+'&&(!_wcsnicmp(cursor+1,L"button:",7)||!_wcsnicmp(cursor+1,L"axis:",5)))return cursor;
    }
    return NULL;
}
static BOOL controls_parse_binding(const WCHAR *text,int *count,int *kinds,int *indexes,int *signs){
    *count=0;
    if(!text||!*text||!_wcsicmp(text,L"none"))return TRUE;
    const WCHAR *cursor=text;
    while(*cursor&&*count<CONTROLS_MAX_CHORD){
        const WCHAR *separator=controls_next_separator(cursor);
        size_t length=separator?(size_t)(separator-cursor):wcslen(cursor);
        WCHAR part[64];
        if(!length||length>=64)return FALSE;
        memcpy(part,cursor,length*sizeof(WCHAR));part[length]=0;
        if(!controls_parse_binding_part(part,&kinds[*count],&indexes[*count],&signs[*count]))return FALSE;
        (*count)++;
        if(!separator)break;
        cursor=separator+1;
    }
    return TRUE;
}
static void controls_binding_text(int row,WCHAR *out,size_t size){
    out[0]=0;
    if(controls_binding_count[row]==0){
        wcsncpy(out,L"Unbound",size);out[size-1]=0;return;
    }
    size_t used=0;
    for(int i=0;i<controls_binding_count[row];i++){
        WCHAR label[24];
        if(controls_binding_kind[row][i]==BINDING_BUTTON){
            _snwprintf(label,24,L"%ls",controls_button_short[controls_binding_index[row][i]]);
        }else{
            _snwprintf(label,24,L"%ls%ls",controls_axis_short[controls_binding_index[row][i]],
                controls_binding_sign[row][i]<0?L"-":L"+");
        }
        _snwprintf(out+used,size-used,L"%ls%ls",i?L"+":L"",label);
        used=wcslen(out);
        if(used>=size)return;
    }
    _snwprintf(out+used,size-used,L" (");
    used=wcslen(out);
    for(int i=0;i<controls_binding_count[row];i++){
        WCHAR coord[16];
        _snwprintf(coord,16,L"%ls%d",controls_binding_kind[row][i]==BINDING_BUTTON?L"b":L"a",
            controls_binding_index[row][i]);
        _snwprintf(out+used,size-used,L"%ls%ls",i?L"+":L"",coord);
        used=wcslen(out);
        if(used>=size)return;
    }
    _snwprintf(out+used,size-used,L")");
}
static void controls_binding_update_text(int row){
    WCHAR text[96];
    controls_binding_text(row,text,96);
    SetWindowTextW(controls_controller_bindings[row],text);
}
static void controls_binding_store(int row,int kind,int index,int sign){
    if(row<0||row>=CONTROLLER_BINDING_COUNT)return;
    if(kind==BINDING_NONE){
        controls_binding_count[row]=0;
    }else{
        controls_binding_count[row]=1;
        controls_binding_kind[row][0]=kind;
        controls_binding_index[row][0]=index;
        controls_binding_sign[row][0]=sign;
    }
    controls_binding_update_text(row);
}
static void controls_binding_store_chord(int row,const unsigned mask){
    if(row<0||row>=CONTROLLER_BINDING_COUNT)return;
    int count=0;
    for(int i=0;i<CONTROLS_BUTTON_COUNT&&count<CONTROLS_MAX_CHORD;i++){
        if(mask&(1u<<i)){
            controls_binding_kind[row][count]=BINDING_BUTTON;
            controls_binding_index[row][count]=i;
            controls_binding_sign[row][count]=1;
            count++;
        }
    }
    /* A virtual axis consumes one physical source; chords are for buttons. */
    if(row>=15&&count>1)count=1;
    controls_binding_count[row]=count;
    controls_binding_update_text(row);
}
static void controls_binding_canonical(int row,WCHAR *out,size_t size){
    out[0]=0;
    if(controls_binding_count[row]==0){
        wcsncpy(out,L"none",size);out[size-1]=0;return;
    }
    size_t used=0;
    for(int i=0;i<controls_binding_count[row];i++){
        WCHAR part[32];
        if(controls_binding_kind[row][i]==BINDING_BUTTON){
            _snwprintf(part,32,L"button:%ls",controls_button_names[controls_binding_index[row][i]]);
        }else{
            _snwprintf(part,32,L"axis:%ls%ls",controls_axis_names[controls_binding_index[row][i]],
                controls_binding_sign[row][i]<0?L"-":L"");
        }
        _snwprintf(out+used,size-used,L"%ls%ls",i?L"+":L"",part);
        used=wcslen(out);
        if(used>=size)return;
    }
}
static void controls_binding_assign(int row,int count,const int *kinds,const int *indexes,const int *signs){
    if(row<0||row>=CONTROLLER_BINDING_COUNT)return;
    if(count<0)count=0;
    if(count>CONTROLS_MAX_CHORD)count=CONTROLS_MAX_CHORD;
    if(row>=15&&count>1)count=1;
    controls_binding_count[row]=count;
    for(int i=0;i<count;i++){
        controls_binding_kind[row][i]=kinds[i];
        controls_binding_index[row][i]=indexes[i];
        controls_binding_sign[row][i]=signs[i];
    }
    controls_binding_update_text(row);
}
static void controls_sdl_bind(void *target,size_t size,HMODULE module,const char *name){
    FARPROC symbol=GetProcAddress(module,name);
    memset(target,0,size);
    memcpy(target,&symbol,size<sizeof(symbol)?size:sizeof(symbol));
}
#define CONTROLS_SDL_BIND(field,name) controls_sdl_bind(&controls_sdl.field,sizeof(controls_sdl.field),module,name)
static int controls_sdl_init(void){
    if(controls_sdl_attempted)return controls_sdl_ready;
    controls_sdl_attempted=TRUE;
    WCHAR dll[CAP];path(dll,L"SDL3.dll");
    controls_sdl.module=LoadLibraryW(dll);
    if(!controls_sdl.module)controls_sdl.module=LoadLibraryW(L"SDL3.dll");
    if(!controls_sdl.module){
        SetWindowTextW(status,L"Could not load SDL3.dll. Install it beside the launcher, then try again.");
        return FALSE;
    }
    HMODULE module=controls_sdl.module;
    CONTROLS_SDL_BIND(Init,"SDL_Init");
    CONTROLS_SDL_BIND(Quit,"SDL_Quit");
    CONTROLS_SDL_BIND(UpdateGamepads,"SDL_UpdateGamepads");
    CONTROLS_SDL_BIND(GetGamepads,"SDL_GetGamepads");
    CONTROLS_SDL_BIND(Free,"SDL_free");
    CONTROLS_SDL_BIND(OpenGamepad,"SDL_OpenGamepad");
    CONTROLS_SDL_BIND(CloseGamepad,"SDL_CloseGamepad");
    CONTROLS_SDL_BIND(GamepadConnected,"SDL_GamepadConnected");
    CONTROLS_SDL_BIND(GetGamepadButton,"SDL_GetGamepadButton");
    CONTROLS_SDL_BIND(GetGamepadAxis,"SDL_GetGamepadAxis");
    CONTROLS_SDL_BIND(SetHint,"SDL_SetHint");
    if(!controls_sdl.Init||!controls_sdl.UpdateGamepads||!controls_sdl.GetGamepads||
       !controls_sdl.OpenGamepad||!controls_sdl.GetGamepadButton||!controls_sdl.GetGamepadAxis){
        SetWindowTextW(status,L"SDL3.dll does not provide the gamepad functions the launcher needs.");
        return FALSE;
    }
    if(controls_sdl.SetHint){
        /* Never route SDL back through the XInput proxy in this folder. */
        controls_sdl.SetHint("SDL_JOYSTICK_XINPUT","0");
        controls_sdl.SetHint("SDL_JOYSTICK_HIDAPI","1");
        controls_sdl.SetHint("SDL_JOYSTICK_RAWINPUT","1");
        controls_sdl.SetHint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS","1");
    }
    if(!controls_sdl.Init(0x00002000u)){
        SetWindowTextW(status,L"SDL could not initialize the gamepad backend.");
        return FALSE;
    }
    controls_sdl_ready=TRUE;
    return TRUE;
}
static int controls_sdl_refresh(void){
    if(!controls_sdl_ready)return 0;
    controls_sdl.UpdateGamepads();
    if(controls_pad&&controls_sdl.GamepadConnected&&controls_sdl.GamepadConnected(controls_pad))return 1;
    if(controls_pad){
        if(controls_sdl.CloseGamepad)controls_sdl.CloseGamepad(controls_pad);
        controls_pad=NULL;
    }
    int count=0;
    ControlsJoystickID *ids=controls_sdl.GetGamepads(&count);
    if(ids&&count>0)controls_pad=controls_sdl.OpenGamepad(ids[0]);
    if(ids)controls_sdl.Free(ids);
    return controls_pad?1:0;
}
static int controls_axis_threshold(int axis){return axis>=4?6000:16000;}
static int controls_pad_active(void){
    for(int i=0;i<CONTROLS_BUTTON_COUNT;i++)if(controls_sdl.GetGamepadButton(controls_pad,i))return 1;
    for(int i=0;i<CONTROLS_AXIS_COUNT;i++){
        int value=controls_sdl.GetGamepadAxis(controls_pad,i);
        if((value<0?-value:value)>controls_axis_threshold(i))return 1;
    }
    return 0;
}
static void controls_capture_end(void){
    if(window)KillTimer(window,CONTROLS_CAPTURE_TIMER);
    controls_capture_row=-1;
    controls_capture_wait_release=FALSE;
    controls_capture_seen=0;
}
static void controls_capture_cancel(void){
    if(controls_capture_row>=0)controls_binding_update_text(controls_capture_row);
    controls_capture_end();
}
static void controls_capture_progress(unsigned mask){
    if(controls_capture_row<0)return;
    WCHAR text[96];size_t used=0;text[0]=0;
    for(int i=0;i<CONTROLS_BUTTON_COUNT;i++){
        if(mask&(1u<<i)){
            _snwprintf(text+used,96-used,L"%ls%ls",used?L"+":L"",controls_button_short[i]);
            used=wcslen(text);
        }
    }
    _snwprintf(text+used,96-used,L"...");
    SetWindowTextW(controls_controller_bindings[controls_capture_row],text);
}
static void controls_capture_finish(const WCHAR *message){
    controls_capture_end();
    SetWindowTextW(status,message);
}
static void controls_capture_store(int kind,int index,int sign){
    int row=controls_capture_row;
    if(row<0)return;
    controls_binding_store(row,kind,index,sign);
    controls_capture_finish(L"Binding captured. Save settings to apply it to the game.");
}
static void controls_capture_store_chord(unsigned mask){
    int row=controls_capture_row;
    if(row<0||!mask)return;
    controls_binding_store_chord(row,mask);
    controls_capture_finish(L"Binding captured. Save settings to apply it to the game.");
}
static void controls_capture_tick(void){
    if(controls_capture_row<0)return;
    if(!controls_sdl_init()){controls_capture_cancel();return;}
    if(!controls_sdl_refresh()){
        SetWindowTextW(status,L"Connect a controller, then press a button to bind it.");
        return;
    }
    if(controls_capture_wait_release){
        if(controls_pad_active())return;
        controls_capture_wait_release=FALSE;
    }
    unsigned pressed=0;
    for(int i=0;i<CONTROLS_BUTTON_COUNT;i++){
        if(controls_sdl.GetGamepadButton(controls_pad,i))pressed|=1u<<i;
    }
    if(controls_capture_seen||pressed){
        controls_capture_seen|=pressed;
        if(pressed){controls_capture_progress(controls_capture_seen);return;}
        /* Every chord button was released: commit the whole chord. */
        unsigned chord=controls_capture_seen;
        controls_capture_seen=0;
        controls_capture_store_chord(chord);
        return;
    }
    for(int i=0;i<CONTROLS_AXIS_COUNT;i++){
        int value=controls_sdl.GetGamepadAxis(controls_pad,i);
        int magnitude=value<0?-value:value;
        if(magnitude>controls_axis_threshold(i)){
            int sign=i>=4?1:(value<0?-1:1);
            controls_capture_store(BINDING_AXIS,i,sign);
            return;
        }
    }
}
static void controls_capture_begin(int row){
    if(busy||row<0||row>=CONTROLLER_BINDING_COUNT)return;
    if(controls_capture_row==row){controls_capture_cancel();SetWindowTextW(status,L"Binding capture cancelled.");return;}
    controls_capture_cancel();
    if(!controls_sdl_init())return;
    controls_capture_row=row;
    controls_capture_wait_release=TRUE;
    controls_capture_seen=0;
    SetWindowTextW(controls_controller_bindings[row],L"Press a button...");
    SetWindowTextW(status,L"Press a controller button or a chord, or move a stick/trigger. Release all buttons to save. Delete clears; Escape cancels.");
    SetFocus(controls_controller_bindings[row]);
    if(window)SetTimer(window,CONTROLS_CAPTURE_TIMER,16,NULL);
}
static BOOL controls_capture_key(int key){
    if(controls_capture_row<0)return FALSE;
    if(key==VK_ESCAPE){controls_capture_cancel();SetWindowTextW(status,L"Binding capture cancelled.");return TRUE;}
    if(key==VK_DELETE||key==VK_BACK){controls_capture_store(BINDING_NONE,0,1);return TRUE;}
    return FALSE;
}
static WNDPROC controls_capture_button_old_proc;
static LRESULT CALLBACK controls_capture_button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_KEYDOWN&&controls_capture_key((int)wp))return 0;
    return CallWindowProcW(controls_capture_button_old_proc,hwnd,msg,wp,lp);
}
/* Called when the Controls page or one of its subtabs is left. */
static void controls_leave(void){
    if(controls_capture_row>=0)controls_capture_cancel();
}

/* ------------------------------------------------------------------ */
/* Existing camera/hotkey helpers                                      */
/* ------------------------------------------------------------------ */
static void controls_add(HWND box,const WCHAR *text){graphics_choice(box,text,0);}
static void controls_pick(HWND box,const WCHAR *value){
    int count=(int)SendMessageW(box,CB_GETCOUNT,0,0);
    for(int i=0;i<count;i++){
        WCHAR text[64];
        SendMessageW(box,CB_GETLBTEXT,i,(LPARAM)text);
        if(!_wcsicmp(text,value)){SendMessageW(box,CB_SETCURSEL,i,0);return;}
    }
    controls_add(box,value);SendMessageW(box,CB_SETCURSEL,count,0);
}
static void controls_text(HWND box,WCHAR *out,int size){
    int index=(int)SendMessageW(box,CB_GETCURSEL,0,0);
    if(index==CB_ERR){out[0]=0;return;}
    SendMessageW(box,CB_GETLBTEXT,index,(LPARAM)out);out[size-1]=0;
}
static BOOL controls_trace_toggle(void){
    path(controls_ini,L"client.ini");
    BOOL enabled=SendMessageW(controls_trace,BM_GETCHECK,0,0)==BST_CHECKED;
    return WritePrivateProfileStringW(L"camera",L"race_orbit_trace",enabled?L"1":L"0",controls_ini);
}
static void controls_trace_reload(void){
    if(!controls_trace)return;
    path(controls_ini,L"client.ini");
    SendMessageW(controls_trace,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"race_orbit_trace",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
}
static void controls_reload(void){
    if(!controls_enable)return;
    path(controls_ini,L"client.ini");
    WCHAR value[128];
    SendMessageW(controls_enable,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"race_orbit",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"camera",L"free_fov_offset",L"0",value,64,controls_ini);SetWindowTextW(controls_fov,value);
    GetPrivateProfileStringW(L"camera",L"free_zoom",L"1.0",value,64,controls_ini);SetWindowTextW(controls_zoom,value);
    SendMessageW(controls_invert_x,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"invert_x",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_invert_y,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"invert_y",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"controls",L"free_look_button",L"right_thumb",value,64,controls_ini);controls_pick(controls_free_button,value);
    GetPrivateProfileStringW(L"controls",L"free_look_key",L"",value,64,controls_ini);SetWindowTextW(controls_free_key,value);
    GetPrivateProfileStringW(L"controls",L"hud_button",L"back",value,64,controls_ini);controls_pick(controls_hud_button,value);
    GetPrivateProfileStringW(L"controls",L"hud_key",L"",value,64,controls_ini);SetWindowTextW(controls_hud_key,value);
    GetPrivateProfileStringW(L"controls",L"free_look_mouse",L"right",value,64,controls_ini);controls_pick(controls_mouse,value);
    SendMessageW(controls_photo_enable,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_mode",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_photo_hud,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_hide_hud",1,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_photo_invert_strafe,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_invert_strafe",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"controls",L"photo_mode_button",L"left_thumb+right_thumb",value,64,controls_ini);controls_pick(controls_photo_button,value);
    GetPrivateProfileStringW(L"controls",L"photo_mode_key",L"F9",value,64,controls_ini);SetWindowTextW(controls_photo_key,value);
    GetPrivateProfileStringW(L"camera",L"photo_speed",L"8.0",value,64,controls_ini);SetWindowTextW(controls_photo_speed,value);
    SendMessageW(controls_controller_enabled,BM_SETCHECK,GetPrivateProfileIntW(L"controller",L"enabled",1,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    for(int i=0;i<CONTROLLER_BINDING_COUNT;i++){
        int count,kinds[CONTROLS_MAX_CHORD],indexes[CONTROLS_MAX_CHORD],signs[CONTROLS_MAX_CHORD];
        memset(kinds,0,sizeof(kinds));memset(indexes,0,sizeof(indexes));memset(signs,0,sizeof(signs));
        GetPrivateProfileStringW(L"controller",controls_controller_rows[i].key,controls_controller_rows[i].def,value,128,controls_ini);
        if(!controls_parse_binding(value,&count,kinds,indexes,signs))count=0;
        controls_binding_assign(i,count,kinds,indexes,signs);
    }
}
static BOOL controls_save_settings(void){
    path(controls_ini,L"client.ini");
    WCHAR fov[32],zoom[32],free_button[64],free_key[64],hud_button[64],hud_key[64],mouse[64],photo_button[64],photo_key[64],photo_speed[32];
    GetWindowTextW(controls_fov,fov,32);GetWindowTextW(controls_zoom,zoom,32);
    controls_text(controls_free_button,free_button,64);GetWindowTextW(controls_free_key,free_key,64);
    controls_text(controls_hud_button,hud_button,64);GetWindowTextW(controls_hud_key,hud_key,64);
    controls_text(controls_mouse,mouse,64);
    controls_text(controls_photo_button,photo_button,64);GetWindowTextW(controls_photo_key,photo_key,64);
    GetWindowTextW(controls_photo_speed,photo_speed,32);
    float fov_value=(float)_wtof(fov),zoom_value=(float)_wtof(zoom),speed_value=(float)_wtof(photo_speed);
    if(fov_value<-100)fov_value=-100;if(fov_value>100)fov_value=100;
    if(zoom_value<0.25f)zoom_value=0.25f;if(zoom_value>4.0f)zoom_value=4.0f;
    if(speed_value<1.0f)speed_value=1.0f;if(speed_value>100.0f)speed_value=100.0f;
    _snwprintf(fov,32,L"%.2f",fov_value);_snwprintf(zoom,32,L"%.2f",zoom_value);_snwprintf(photo_speed,32,L"%.1f",speed_value);
    BOOL ok=WritePrivateProfileStringW(L"camera",L"race_orbit",SendMessageW(controls_enable,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"free_fov_offset",fov,controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"free_zoom",zoom,controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"invert_x",SendMessageW(controls_invert_x,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"invert_y",SendMessageW(controls_invert_y,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_button",free_button,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_key",free_key,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"hud_button",hud_button,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"hud_key",hud_key,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"free_look_mouse",mouse,controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_mode",SendMessageW(controls_photo_enable,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_hide_hud",SendMessageW(controls_photo_hud,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_invert_strafe",SendMessageW(controls_photo_invert_strafe,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_speed",photo_speed,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"photo_mode_button",photo_button,controls_ini)
        &&WritePrivateProfileStringW(L"controls",L"photo_mode_key",photo_key,controls_ini)
        &&WritePrivateProfileStringW(L"controller",L"enabled",SendMessageW(controls_controller_enabled,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini);
    for(int i=0;i<CONTROLLER_BINDING_COUNT&&ok;i++){
        WCHAR binding[64];
        controls_binding_canonical(i,binding,64);
        ok=WritePrivateProfileStringW(L"controller",controls_controller_rows[i].key,binding,controls_ini)!=0;
    }
    SetWindowTextW(status,ok?L"Controller and camera settings saved. Restart the game to apply changes.":L"Could not save all controller settings. Please try again.");
    return ok;
}
static void controls_create(void){
    /* Hotkeys: ReRiff camera/photo settings and the existing controller
     * hotkey bindings. */
    controls_part=CONTROLS_HOTKEYS;
    controls_enable=control(L"BUTTON",L"Enable free-look camera",BS_AUTOCHECKBOX|WS_TABSTOP,22,22,280,28,0);
    control(L"STATIC",L"FOV offset (degrees)",0,22,54,150,24,0);
    controls_fov=control(L"EDIT",L"0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,50,112,28,0);
    control(L"STATIC",L"Distance multiplier",0,22,86,150,24,0);
    controls_zoom=control(L"EDIT",L"1.0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,82,112,28,0);
    controls_invert_x=control(L"BUTTON",L"Invert look X",BS_AUTOCHECKBOX|WS_TABSTOP,22,114,130,28,0);
    controls_invert_y=control(L"BUTTON",L"Invert look Y",BS_AUTOCHECKBOX|WS_TABSTOP,164,114,138,28,0);
    control(L"STATIC",L"Mouse look button",0,22,150,150,24,0);
    controls_mouse=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,190,146,112,160,0);
    controls_photo_enable=control(L"BUTTON",L"Enable photo mode",BS_AUTOCHECKBOX|WS_TABSTOP,22,178,280,28,0);
    controls_photo_hud=control(L"BUTTON",L"Hide HUD in photo mode",BS_AUTOCHECKBOX|WS_TABSTOP,22,210,280,28,0);
    controls_photo_invert_strafe=control(L"BUTTON",L"Invert photo strafe X",BS_AUTOCHECKBOX|WS_TABSTOP,312,246,280,28,0);
    control(L"STATIC",L"Free-look button",0,312,22,130,24,0);
    controls_free_button=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,452,18,160,200,0);
    control(L"STATIC",L"HUD button",0,312,86,130,24,0);
    controls_hud_button=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,452,82,160,200,0);
    control(L"STATIC",L"Photo mode button",0,312,150,130,24,0);
    controls_photo_button=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,452,146,160,200,0);
    SendMessageW(controls_photo_button,CB_SETDROPPEDWIDTH,220,0);
    control(L"STATIC",L"Photo speed",0,312,214,130,24,0);
    controls_photo_speed=control(L"EDIT",L"8.0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,452,210,80,28,0);
    controls_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,240,220,32,CONTROLS_SAVE);
    controls_first=controls_enable;
    checkbox_help(controls_enable,L"Keep the normal chase camera and use the right stick/mouse to look around while free look is toggled on. Press R3 or the configured button/key to toggle. Restart the game after changes.");
    checkbox_help(controls_invert_x,L"Invert horizontal look from the right stick or mouse. 0 in client.ini is the preferred direction; this checkbox stores 1. It does not change photo-mode strafing.");
    checkbox_help(controls_invert_y,L"Invert vertical look from the right stick or mouse. 0 in client.ini is the preferred direction; this checkbox stores 1.");
    checkbox_help(controls_photo_enable,L"Enable photo mode. The default controller chord is L3+R3 and the default keyboard key is F9. During a race it pauses without opening the pause menu and gives you a free camera.");
    checkbox_help(controls_photo_hud,L"Hide the complete race HUD while photo mode is active and restore it when photo mode ends.");
    checkbox_help(controls_photo_invert_strafe,L"Flip only the photo-mode left-stick and A/D strafe direction. This does not affect right-stick look.");
    checkbox_help(controls_photo_button,L"Controller chord for photo mode. Separate buttons with +. The default is left_thumb+right_thumb (L3+R3).");
    checkbox_help(controls_photo_speed,L"Free-camera speed in game units per second. Hold the right trigger or Shift for a 3x boost.");
    checkbox_help(controls_mouse,L"Mouse button held while using free look or photo mode to capture mouse movement.");

    /* Keyboard: ReRiff keyboard bindings. */
    controls_part=CONTROLS_KEYBOARD;
    control(L"STATIC",L"Free-look key",0,22,26,150,24,0);
    controls_free_key=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,22,120,28,0);
    control(L"STATIC",L"HUD key",0,22,58,150,24,0);
    controls_hud_key=control(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,54,120,28,0);
    control(L"STATIC",L"Photo mode key",0,22,90,150,24,0);
    controls_photo_key=control(L"EDIT",L"F9",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,86,120,28,0);
    control(L"BUTTON",L"Save settings",WS_TABSTOP,22,130,220,32,CONTROLS_SAVE);
    checkbox_help(controls_free_key,L"Keyboard key that toggles free look. Leave empty to use only the controller button.");
    checkbox_help(controls_hud_key,L"Keyboard key that toggles the HUD. Leave empty to use only the controller button.");
    checkbox_help(controls_photo_key,L"Keyboard key that toggles photo mode. Leave empty to use only the controller chord.");
    restart_note(L"Restart the game after changing these settings",22,174,590);

    /* Controller: click a row, then press the physical SDL control to bind.
     * Three columns keep the table clear of the launcher status area. */
    controls_part=CONTROLS_CONTROLLER;
    for(int i=0;i<CONTROLLER_BINDING_COUNT;i++){
        int column=i/7,row=i%7;
        int x=6+column*216,y=22+row*26;
        control(L"STATIC",controls_controller_rows[i].label,0,x,y+2,94,24,0);
        controls_controller_bindings[i]=control(L"BUTTON",L"",WS_TABSTOP,x+96,y,112,24,CONTROLLER_CAPTURE_BASE+i);
        controls_capture_button_old_proc=(WNDPROC)SetWindowLongPtrW(controls_controller_bindings[i],GWLP_WNDPROC,(LONG_PTR)controls_capture_button_proc);
    }
    controls_controller_enabled=control(L"BUTTON",L"Enable controller remapping",BS_AUTOCHECKBOX|WS_TABSTOP,250,210,390,28,0);
    controls_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,14,210,210,30,CONTROLS_SAVE);
    checkbox_help(controls_controller_enabled,L"Enable the SDL3 XInput proxy remap table. Restart the game after saving. The game receives the virtual controls selected on this tab. Chords such as L3+R3 require every listed button.");
    restart_note(L"Click a binding, press one button or a chord, then release. Delete clears it.",14,246,630);
    graphics_choice(controls_free_button,L"right_thumb",0);graphics_choice(controls_free_button,L"left_thumb",0);
    graphics_choice(controls_free_button,L"back",0);graphics_choice(controls_free_button,L"start",0);
    graphics_choice(controls_free_button,L"left_shoulder",0);graphics_choice(controls_free_button,L"right_shoulder",0);
    graphics_choice(controls_free_button,L"a",0);graphics_choice(controls_free_button,L"b",0);
    graphics_choice(controls_free_button,L"x",0);graphics_choice(controls_free_button,L"y",0);
    graphics_choice(controls_free_button,L"dpad_up",0);graphics_choice(controls_free_button,L"dpad_down",0);
    graphics_choice(controls_free_button,L"dpad_left",0);graphics_choice(controls_free_button,L"dpad_right",0);
    graphics_choice(controls_free_button,L"none",0);
    for(int i=0;i<(int)SendMessageW(controls_free_button,CB_GETCOUNT,0,0);i++){
        WCHAR text[64];SendMessageW(controls_free_button,CB_GETLBTEXT,i,(LPARAM)text);controls_add(controls_hud_button,text);
    }
    graphics_choice(controls_photo_button,L"left_thumb+right_thumb",0);
    for(int i=0;i<(int)SendMessageW(controls_free_button,CB_GETCOUNT,0,0);i++){
        WCHAR text[64];SendMessageW(controls_free_button,CB_GETLBTEXT,i,(LPARAM)text);
        if(wcscmp(text,L"none"))controls_add(controls_photo_button,text);
    }
    graphics_choice(controls_photo_button,L"none",0);
    graphics_choice(controls_mouse,L"right",0);graphics_choice(controls_mouse,L"left",0);
    graphics_choice(controls_mouse,L"middle",0);graphics_choice(controls_mouse,L"none",0);
    controls_reload();
}
#endif
