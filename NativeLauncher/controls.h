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
static HWND controls_trace;
static HWND controls_photo_enable,controls_photo_hud,controls_photo_speed,controls_photo_invert_strafe,controls_save,controls_first;static WCHAR controls_ini[CAP];
#define KEYBOARD_ACTION_COUNT 14
#define KEYBOARD_CAPTURE_TIMER 3
enum { KEYBOARD_VIRTUAL, KEYBOARD_HOTKEY, KEYBOARD_MOUSE };
typedef struct { const WCHAR *key,*label; int virtual_key,kind,default_value; } KeyboardActionRow;
static HWND controls_keyboard_enabled,controls_keyboard_first,controls_keyboard_mouse;
static HWND controls_keyboard_bindings[KEYBOARD_ACTION_COUNT];
static int controls_key_values[KEYBOARD_ACTION_COUNT];
static int controls_key_capture_row=-1;
static BOOL controls_key_wait_release;
static const KeyboardActionRow controls_keyboard_rows[KEYBOARD_ACTION_COUNT]={
    {L"accelerate",L"Accelerate / Menu up",VK_UP,KEYBOARD_VIRTUAL,VK_UP},
    {L"brake",L"Brake / Menu down",VK_DOWN,KEYBOARD_VIRTUAL,VK_DOWN},
    {L"left",L"Steer / Menu left",VK_LEFT,KEYBOARD_VIRTUAL,VK_LEFT},
    {L"right",L"Steer / Menu right",VK_RIGHT,KEYBOARD_VIRTUAL,VK_RIGHT},
    {L"boost",L"Boost",VK_SHIFT,KEYBOARD_VIRTUAL,VK_SHIFT},
    {L"drift",L"Drift",VK_SPACE,KEYBOARD_VIRTUAL,VK_SPACE},
    {L"camera",L"Camera / Menu tab",VK_TAB,KEYBOARD_VIRTUAL,VK_TAB},
    {L"confirm",L"Confirm",VK_RETURN,KEYBOARD_VIRTUAL,VK_RETURN},
    {L"pause",L"Pause / Back",VK_ESCAPE,KEYBOARD_VIRTUAL,VK_ESCAPE},
    {L"restart",L"Restart",'R',KEYBOARD_VIRTUAL,'R'},
    {L"free_look",L"Free look (ReRiff)",0,KEYBOARD_HOTKEY,0},
    {L"hud",L"HUD (ReRiff)",0,KEYBOARD_HOTKEY,0},
    {L"photo_mode",L"Photo mode (ReRiff)",0,KEYBOARD_HOTKEY,VK_F9},
    {L"mouse_look",L"Mouse look (ReRiff)",0,KEYBOARD_MOUSE,VK_RBUTTON}
};
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
    {L"lb",L"Unused (LB)",L"none"},{L"rb",L"Unused (RB)",L"none"},
    {L"back",L"Restart",L"button:back"},{L"start",L"Pause",L"button:start"},
    {L"l3",L"Photo chord",L"button:l3"},{L"r3",L"Free look",L"button:r3"},
    {L"guide",L"Guide",L"button:guide"},
    {L"dup",L"Menu up",L"button:dup"},{L"ddown",L"Menu down",L"button:ddown"},
    {L"dleft",L"Steer/Menu L",L"button:dleft"},{L"dright",L"Steer/Menu R",L"button:dright"},
    {L"lx",L"Steer/Menu",L"axis:lx"},{L"ly",L"Menu Y",L"axis:ly"},
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
static void controls_controller_defaults(void){
    if(!controls_controller_enabled)return;
    SendMessageW(controls_controller_enabled,BM_SETCHECK,BST_CHECKED,0);
    for(int i=0;i<CONTROLLER_BINDING_COUNT;i++){
        int count,kinds[CONTROLS_MAX_CHORD],indexes[CONTROLS_MAX_CHORD],signs[CONTROLS_MAX_CHORD];
        memset(kinds,0,sizeof(kinds));memset(indexes,0,sizeof(indexes));memset(signs,0,sizeof(signs));
        if(!controls_parse_binding(controls_controller_rows[i].def,&count,kinds,indexes,signs))count=0;
        controls_binding_assign(i,count,kinds,indexes,signs);
    }
    SetWindowTextW(status,L"Controller defaults restored. Save settings to apply.");
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
            /* A virtual axis stores the physical axis direction, not an
             * inversion. Y conversion happens once in the proxy. */
            if(controls_capture_row>=15)sign=1;
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
static int controls_key_value(const WCHAR *value){
    if(!value||!*value||!_wcsicmp(value,L"none"))return 0;
    const WCHAR *number=value;
    if(!_wcsnicmp(number,L"key:",4))number+=4;
    WCHAR *end=NULL;unsigned long vk=wcstoul(number,&end,0);
    if(end&&!*end&&vk==0)return 0;
    if(end&&!*end&&vk<256)return (int)vk;
    if(value[0]&&!value[1]){
        if(value[0]>='a'&&value[0]<='z')return (int)(value[0]-32);
        if((value[0]>='A'&&value[0]<='Z')||(value[0]>='0'&&value[0]<='9'))return (int)value[0];
    }
    if((value[0]=='F'||value[0]=='f')&&value[1]){
        int number_value=_wtoi(value+1);
        if(number_value>=1&&number_value<=24)return VK_F1+number_value-1;
    }
    static const struct {const WCHAR *name;int key;} table[]={
        {L"space",VK_SPACE},{L"enter",VK_RETURN},{L"return",VK_RETURN},{L"tab",VK_TAB},
        {L"escape",VK_ESCAPE},{L"esc",VK_ESCAPE},{L"backspace",VK_BACK},{L"insert",VK_INSERT},
        {L"delete",VK_DELETE},{L"del",VK_DELETE},{L"home",VK_HOME},{L"end",VK_END},
        {L"pageup",VK_PRIOR},{L"pgup",VK_PRIOR},{L"pagedown",VK_NEXT},{L"pgdn",VK_NEXT},
        {L"up",VK_UP},{L"down",VK_DOWN},{L"left",VK_LEFT},{L"right",VK_RIGHT},
        {L"shift",VK_SHIFT},{L"lshift",VK_LSHIFT},{L"rshift",VK_RSHIFT},
        {L"ctrl",VK_CONTROL},{L"control",VK_CONTROL},{L"lctrl",VK_LCONTROL},{L"rctrl",VK_RCONTROL},
        {L"alt",VK_MENU},{L"lalt",VK_LMENU},{L"ralt",VK_RMENU},
        {L"numpad0",VK_NUMPAD0},{L"numpad1",VK_NUMPAD1},{L"numpad2",VK_NUMPAD2},{L"numpad3",VK_NUMPAD3},
        {L"numpad4",VK_NUMPAD4},{L"numpad5",VK_NUMPAD5},{L"numpad6",VK_NUMPAD6},{L"numpad7",VK_NUMPAD7},
        {L"numpad8",VK_NUMPAD8},{L"numpad9",VK_NUMPAD9},{L"multiply",VK_MULTIPLY},{L"add",VK_ADD},
        {L"subtract",VK_SUBTRACT},{L"decimal",VK_DECIMAL},{L"divide",VK_DIVIDE}};
    for(unsigned i=0;i<sizeof(table)/sizeof(table[0]);i++)if(!_wcsicmp(value,table[i].name))return table[i].key;
    return 0;
}
static void controls_key_name(int vk,WCHAR *out,int size){
    if(!vk){wcsncpy(out,L"Unbound",size);out[size-1]=0;return;}
    if(vk==VK_LBUTTON){wcsncpy(out,L"Left mouse",size);out[size-1]=0;return;}
    if(vk==VK_RBUTTON){wcsncpy(out,L"Right mouse",size);out[size-1]=0;return;}
    if(vk==VK_MBUTTON){wcsncpy(out,L"Middle mouse",size);out[size-1]=0;return;}
    if(vk==VK_XBUTTON1){wcsncpy(out,L"Mouse 4",size);out[size-1]=0;return;}
    if(vk==VK_XBUTTON2){wcsncpy(out,L"Mouse 5",size);out[size-1]=0;return;}
    if((vk>='A'&&vk<='Z')||(vk>='0'&&vk<='9')){out[0]=(WCHAR)vk;out[1]=0;return;}
    if(vk>=VK_F1&&vk<=VK_F24){_snwprintf(out,size,L"F%d",vk-VK_F1+1);return;}
    static const struct {int key;const WCHAR *name;} table[]={
        {VK_SPACE,L"Space"},{VK_RETURN,L"Enter"},{VK_TAB,L"Tab"},{VK_ESCAPE,L"Esc"},
        {VK_BACK,L"Backspace"},{VK_INSERT,L"Insert"},{VK_DELETE,L"Delete"},
        {VK_HOME,L"Home"},{VK_END,L"End"},{VK_PRIOR,L"Page Up"},{VK_NEXT,L"Page Down"},
        {VK_UP,L"Up"},{VK_DOWN,L"Down"},{VK_LEFT,L"Left"},{VK_RIGHT,L"Right"},
        {VK_SHIFT,L"Shift"},{VK_LSHIFT,L"Left Shift"},{VK_RSHIFT,L"Right Shift"},
        {VK_CONTROL,L"Ctrl"},{VK_LCONTROL,L"Left Ctrl"},{VK_RCONTROL,L"Right Ctrl"},
        {VK_MENU,L"Alt"},{VK_LMENU,L"Left Alt"},{VK_RMENU,L"Right Alt"},
        {VK_NUMPAD0,L"Numpad 0"},{VK_NUMPAD1,L"Numpad 1"},{VK_NUMPAD2,L"Numpad 2"},
        {VK_NUMPAD3,L"Numpad 3"},{VK_NUMPAD4,L"Numpad 4"},{VK_NUMPAD5,L"Numpad 5"},
        {VK_NUMPAD6,L"Numpad 6"},{VK_NUMPAD7,L"Numpad 7"},{VK_NUMPAD8,L"Numpad 8"},
        {VK_NUMPAD9,L"Numpad 9"},{VK_MULTIPLY,L"Numpad *"},{VK_ADD,L"Numpad +"},
        {VK_SUBTRACT,L"Numpad -"},{VK_DECIMAL,L"Numpad ."},{VK_DIVIDE,L"Numpad /"},
        {VK_OEM_1,L";"},{VK_OEM_PLUS,L"="},{VK_OEM_COMMA,L","},{VK_OEM_MINUS,L"-"},
        {VK_OEM_PERIOD,L"."},{VK_OEM_2,L"/"},{VK_OEM_3,L"`"},{VK_OEM_4,L"["},
        {VK_OEM_5,L"\\"},{VK_OEM_6,L"]"},{VK_OEM_7,L"'"}};
    for(unsigned i=0;i<sizeof(table)/sizeof(table[0]);i++){
        if(table[i].key==vk){wcsncpy(out,table[i].name,size);out[size-1]=0;return;}
    }
    UINT scan=MapVirtualKeyW((UINT)vk,MAPVK_VK_TO_VSC);
    LONG param=(LONG)(scan<<16);
    if(scan&&GetKeyNameTextW(param,out,size)>0)return;
    _snwprintf(out,size,L"VK 0x%02X",vk);
}
static void controls_keyboard_update_text(int row){
    if(row<0||row>=KEYBOARD_ACTION_COUNT||controls_keyboard_rows[row].kind==KEYBOARD_MOUSE)return;
    WCHAR text[64];controls_key_name(controls_key_values[row],text,64);
    SetWindowTextW(controls_keyboard_bindings[row],text);
}
static BOOL controls_keyboard_any_down(BOOL include_mouse){
    for(int vk=1;vk<255;vk++){
        if(!include_mouse&&(vk==VK_LBUTTON||vk==VK_RBUTTON||vk==VK_MBUTTON||vk==VK_XBUTTON1||vk==VK_XBUTTON2))continue;
        if(GetAsyncKeyState(vk)&0x8000)return TRUE;
    }
    return FALSE;
}
static void controls_key_capture_end(void){
    if(window)KillTimer(window,KEYBOARD_CAPTURE_TIMER);
    controls_key_capture_row=-1;
    controls_key_wait_release=FALSE;
}
static void controls_key_capture_cancel(void){
    if(controls_key_capture_row>=0)controls_keyboard_update_text(controls_key_capture_row);
    controls_key_capture_end();
}
static void controls_keyboard_assign(int row,int vk){
    if(row<0||row>=KEYBOARD_ACTION_COUNT)return;
    controls_key_values[row]=vk;
    if(controls_keyboard_rows[row].kind==KEYBOARD_VIRTUAL&&vk){
        for(int i=0;i<KEYBOARD_ACTION_COUNT;i++){
            if(i!=row&&controls_keyboard_rows[i].kind==KEYBOARD_VIRTUAL&&controls_key_values[i]==vk){
                controls_key_values[i]=0;controls_keyboard_update_text(i);
            }
        }
    }
    controls_keyboard_update_text(row);
}
static void controls_key_capture_store(int vk){
    int row=controls_key_capture_row;
    if(row<0)return;
    controls_keyboard_assign(row,vk);
    controls_key_capture_end();
    SetWindowTextW(status,vk?L"Key captured. Save settings to apply it to the game.":L"Binding cleared. Save settings to apply it.");
}
static void controls_key_capture_tick(void){
    if(controls_key_capture_row<0)return;
    if(controls_key_wait_release){
        if(controls_keyboard_any_down(TRUE))return;
        controls_key_wait_release=FALSE;
    }
    for(int vk=1;vk<255;vk++){
        if(vk==VK_LBUTTON||vk==VK_RBUTTON||vk==VK_MBUTTON||vk==VK_XBUTTON1||vk==VK_XBUTTON2)continue;
        if(GetAsyncKeyState(vk)&0x8000){controls_key_capture_store(vk);return;}
    }
}
static void controls_key_capture_begin(int row){
    if(busy||row<0||row>=KEYBOARD_ACTION_COUNT||controls_keyboard_rows[row].kind==KEYBOARD_MOUSE)return;
    if(controls_key_capture_row==row){controls_key_capture_cancel();SetWindowTextW(status,L"Key capture cancelled.");return;}
    controls_key_capture_cancel();
    controls_capture_cancel();
    controls_key_capture_row=row;
    controls_key_wait_release=TRUE;
    SetWindowTextW(controls_keyboard_bindings[row],L"Press a key...");
    SetWindowTextW(status,L"Press a keyboard key. Delete clears; Escape cancels.");
    SetFocus(controls_keyboard_bindings[row]);
    if(window)SetTimer(window,KEYBOARD_CAPTURE_TIMER,16,NULL);
}
static BOOL controls_key_capture_key(int key){
    if(controls_key_capture_row<0)return FALSE;
    if(key==VK_ESCAPE){controls_key_capture_cancel();SetWindowTextW(status,L"Key capture cancelled.");return TRUE;}
    if(key==VK_DELETE||key==VK_BACK){controls_key_capture_store(0);return TRUE;}
    return FALSE;
}
static WNDPROC controls_key_button_old_proc;
static LRESULT CALLBACK controls_key_button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_KEYDOWN&&controls_key_capture_row>=0){
        controls_key_capture_key((int)wp);
        return 0;
    }
    return CallWindowProcW(controls_key_button_old_proc,hwnd,msg,wp,lp);
}
static void controls_leave(void){
    if(controls_capture_row>=0)controls_capture_cancel();
    if(controls_key_capture_row>=0)controls_key_capture_cancel();
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
static BOOL controls_ini_value(const WCHAR *section,const WCHAR *key,const WCHAR *fallback,WCHAR *out,int size){
    WCHAR probe[2]={1,0};
    DWORD got=GetPrivateProfileStringW(section,key,probe,out,size,controls_ini);
    if(got==1&&out[0]==1){
        wcsncpy(out,fallback,size);out[size-1]=0;
        return FALSE;
    }
    return TRUE;
}
static const WCHAR *controls_hotkey_ini_key(int row){
    if(row==10)return L"free_look_key";
    if(row==11)return L"hud_key";
    return L"photo_mode_key";
}
static void controls_keyboard_reload(void){
    if(!controls_keyboard_enabled)return;
    path(controls_ini,L"client.ini");
    SendMessageW(controls_keyboard_enabled,BM_SETCHECK,GetPrivateProfileIntW(L"keyboard",L"enabled",1,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    WCHAR value[64];
    for(int i=0;i<KEYBOARD_ACTION_COUNT;i++){
        if(controls_keyboard_rows[i].kind==KEYBOARD_VIRTUAL){
            if(controls_ini_value(L"keyboard",controls_keyboard_rows[i].key,L"",value,64))
                controls_key_values[i]=controls_key_value(value);
            else
                controls_key_values[i]=controls_keyboard_rows[i].default_value;
            controls_keyboard_update_text(i);
        }else if(controls_keyboard_rows[i].kind==KEYBOARD_HOTKEY){
            WCHAR fallback[16];
            if(controls_keyboard_rows[i].default_value)_snwprintf(fallback,16,L"%d",controls_keyboard_rows[i].default_value);
            else wcsncpy(fallback,L"none",16);
            fallback[15]=0;
            controls_ini_value(L"controls",controls_hotkey_ini_key(i),fallback,value,64);
            controls_key_values[i]=controls_key_value(value);
            controls_keyboard_update_text(i);
        }else{
            controls_ini_value(L"controls",L"free_look_mouse",L"right",value,64);
            controls_pick(controls_keyboard_mouse,value);
        }
    }
}
static BOOL controls_keyboard_save(void){
    path(controls_ini,L"client.ini");
    for(int i=0;i<KEYBOARD_ACTION_COUNT;i++){
        if(controls_keyboard_rows[i].kind!=KEYBOARD_VIRTUAL||!controls_key_values[i])continue;
        for(int j=i+1;j<KEYBOARD_ACTION_COUNT;j++){
            if(controls_keyboard_rows[j].kind==KEYBOARD_VIRTUAL&&controls_key_values[j]==controls_key_values[i]){
                SetWindowTextW(status,L"Choose a unique key for each keyboard action.");
                return FALSE;
            }
        }
    }
    BOOL ok=WritePrivateProfileStringW(L"keyboard",L"enabled",SendMessageW(controls_keyboard_enabled,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)!=0;
    for(int i=0;i<KEYBOARD_ACTION_COUNT&&ok;i++){
        if(controls_keyboard_rows[i].kind==KEYBOARD_VIRTUAL){
            WCHAR text[32];
            if(controls_key_values[i])_snwprintf(text,32,L"key:%d",controls_key_values[i]);
            else wcsncpy(text,L"none",32);
            text[31]=0;
            ok=WritePrivateProfileStringW(L"keyboard",controls_keyboard_rows[i].key,text,controls_ini)!=0;
        }else if(controls_keyboard_rows[i].kind==KEYBOARD_HOTKEY){
            WCHAR text[16];
            if(controls_key_values[i])_snwprintf(text,16,L"%d",controls_key_values[i]);
            else text[0]=0;
            text[15]=0;
            ok=WritePrivateProfileStringW(L"controls",controls_hotkey_ini_key(i),text,controls_ini)!=0;
        }
    }
    if(ok){
        WCHAR mouse[32];controls_text(controls_keyboard_mouse,mouse,32);
        ok=WritePrivateProfileStringW(L"controls",L"free_look_mouse",mouse,controls_ini)!=0;
    }
    return ok;
}
static void controls_keyboard_defaults(void){
    if(!controls_keyboard_enabled)return;
    for(int i=0;i<KEYBOARD_ACTION_COUNT;i++){
        controls_key_values[i]=controls_keyboard_rows[i].default_value;
        controls_keyboard_update_text(i);
    }
    controls_pick(controls_keyboard_mouse,L"right");
    SendMessageW(controls_keyboard_enabled,BM_SETCHECK,BST_CHECKED,0);
    SetWindowTextW(status,L"Keyboard defaults restored. Save settings to apply.");
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
    SendMessageW(controls_photo_enable,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_mode",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_photo_hud,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_hide_hud",1,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    SendMessageW(controls_photo_invert_strafe,BM_SETCHECK,GetPrivateProfileIntW(L"camera",L"photo_invert_strafe",0,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    GetPrivateProfileStringW(L"camera",L"photo_speed",L"8.0",value,64,controls_ini);SetWindowTextW(controls_photo_speed,value);
    SendMessageW(controls_controller_enabled,BM_SETCHECK,GetPrivateProfileIntW(L"controller",L"enabled",1,controls_ini)?BST_CHECKED:BST_UNCHECKED,0);
    for(int i=0;i<CONTROLLER_BINDING_COUNT;i++){
        int count,kinds[CONTROLS_MAX_CHORD],indexes[CONTROLS_MAX_CHORD],signs[CONTROLS_MAX_CHORD];
        memset(kinds,0,sizeof(kinds));memset(indexes,0,sizeof(indexes));memset(signs,0,sizeof(signs));
        GetPrivateProfileStringW(L"controller",controls_controller_rows[i].key,controls_controller_rows[i].def,value,128,controls_ini);
        if(!controls_parse_binding(value,&count,kinds,indexes,signs))count=0;
        controls_binding_assign(i,count,kinds,indexes,signs);
    }
    controls_keyboard_reload();
}
static BOOL controls_save_settings(void){
    path(controls_ini,L"client.ini");
    if(!controls_keyboard_save())return FALSE;
    WCHAR fov[32],zoom[32],photo_speed[32];
    GetWindowTextW(controls_fov,fov,32);GetWindowTextW(controls_zoom,zoom,32);
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
        &&WritePrivateProfileStringW(L"camera",L"photo_mode",SendMessageW(controls_photo_enable,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_hide_hud",SendMessageW(controls_photo_hud,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_invert_strafe",SendMessageW(controls_photo_invert_strafe,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini)
        &&WritePrivateProfileStringW(L"camera",L"photo_speed",photo_speed,controls_ini)
        &&WritePrivateProfileStringW(L"controller",L"enabled",SendMessageW(controls_controller_enabled,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0",controls_ini);
    for(int i=0;i<CONTROLLER_BINDING_COUNT&&ok;i++){
        WCHAR binding[64];
        controls_binding_canonical(i,binding,64);
        ok=WritePrivateProfileStringW(L"controller",controls_controller_rows[i].key,binding,controls_ini)!=0;
    }
    SetWindowTextW(status,ok?L"Input and camera settings saved. Restart the game to apply changes.":L"Could not save all input settings. Please try again.");
    return ok;
}
static void controls_create(void){
    /* Hotkeys: ReRiff camera/photo settings. Controller buttons are remapped
     * on the Controller tab, so this page only edits keyboard and camera
     * options. */
    controls_part=CONTROLS_HOTKEYS;
    controls_enable=control(L"BUTTON",L"Enable free-look camera",BS_AUTOCHECKBOX|WS_TABSTOP,22,22,280,28,0);
    control(L"STATIC",L"FOV offset (degrees)",0,22,54,150,24,0);
    controls_fov=control(L"EDIT",L"0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,50,112,28,0);
    control(L"STATIC",L"Distance multiplier",0,22,86,150,24,0);
    controls_zoom=control(L"EDIT",L"1.0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,190,82,112,28,0);
    controls_invert_x=control(L"BUTTON",L"Invert look X",BS_AUTOCHECKBOX|WS_TABSTOP,22,114,130,28,0);
    controls_invert_y=control(L"BUTTON",L"Invert look Y",BS_AUTOCHECKBOX|WS_TABSTOP,164,114,138,28,0);
    controls_photo_enable=control(L"BUTTON",L"Enable photo mode",BS_AUTOCHECKBOX|WS_TABSTOP,22,178,280,28,0);
    controls_photo_hud=control(L"BUTTON",L"Hide HUD in photo mode",BS_AUTOCHECKBOX|WS_TABSTOP,22,210,280,28,0);
    control(L"STATIC",L"Photo speed",0,312,174,130,24,0);
    controls_photo_speed=control(L"EDIT",L"8.0",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,452,170,80,28,0);
    controls_photo_invert_strafe=control(L"BUTTON",L"Invert photo strafe X",BS_AUTOCHECKBOX|WS_TABSTOP,312,206,280,28,0);
    restart_note(L"Controller buttons are on the Controller tab",312,246,300);
    controls_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,22,240,220,32,CONTROLS_SAVE);
    controls_first=controls_enable;
    checkbox_help(controls_enable,L"Keep the normal chase camera and use the right stick/mouse to look around while free look is toggled on. Press R3 (remap it on the Controller tab) or the configured key to toggle. Restart the game after changes.");
    checkbox_help(controls_invert_x,L"Invert horizontal look from the right stick or mouse. 0 in client.ini is the preferred direction; this checkbox stores 1. It does not change photo-mode strafing.");
    checkbox_help(controls_invert_y,L"Invert vertical look from the right stick or mouse. 0 in client.ini is the preferred direction; this checkbox stores 1.");
    checkbox_help(controls_photo_enable,L"Enable photo mode. The default controller chord is L3+R3 (remap those on the Controller tab) and the default keyboard key is F9. During a race it pauses without opening the pause menu and gives you a free camera.");
    checkbox_help(controls_photo_hud,L"Hide the complete race HUD while photo mode is active and restore it when photo mode ends.");
    checkbox_help(controls_photo_invert_strafe,L"Flip only the photo-mode left-stick and A/D strafe direction. This does not affect right-stick look.");
    checkbox_help(controls_photo_speed,L"Free-camera speed in game units per second. Hold the right trigger or Shift for a 3x boost.");

    /* Keyboard: virtual game keys plus ReRiff hotkeys. Each row captures a
     * physical keyboard key; virtual rows send the vanilla key to the game. */
    controls_part=CONTROLS_KEYBOARD;
    controls_keyboard_enabled=control(L"BUTTON",L"Enable virtual keyboard mapping",BS_AUTOCHECKBOX|WS_TABSTOP,14,22,300,26,0);
    for(int i=0;i<KEYBOARD_ACTION_COUNT;i++){
        int column=i/7,row=i%7;
        int x=14+column*322,y=54+row*24;
        control(L"STATIC",controls_keyboard_rows[i].label,0,x,y+2,170,22,0);
        if(controls_keyboard_rows[i].kind==KEYBOARD_MOUSE){
            controls_keyboard_mouse=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,x+176,y,130,160,0);
            graphics_choice(controls_keyboard_mouse,L"right",0);
            graphics_choice(controls_keyboard_mouse,L"left",0);
            graphics_choice(controls_keyboard_mouse,L"middle",0);
            graphics_choice(controls_keyboard_mouse,L"none",0);
        }else{
            controls_keyboard_bindings[i]=control(L"BUTTON",L"",WS_TABSTOP,x+176,y,130,22,KEYBOARD_CAPTURE_BASE+i);
            controls_key_button_old_proc=(WNDPROC)SetWindowLongPtrW(controls_keyboard_bindings[i],GWLP_WNDPROC,(LONG_PTR)controls_key_button_proc);
        }
    }
    controls_keyboard_first=controls_keyboard_bindings[0];
    control(L"BUTTON",L"Save settings",WS_TABSTOP,14,224,150,30,CONTROLS_SAVE);
    checkbox_help(control(L"BUTTON",L"Defaults",WS_TABSTOP,172,224,120,30,CONTROLS_DEFAULTS),L"Restore the vanilla keyboard mapping and ReRiff hotkey defaults. Save settings to apply.");
    checkbox_help(controls_keyboard_enabled,L"Remap the game's keyboard keys. Virtual rows send the vanilla key to the game; the button shows your physical key. Restart the game after saving.");
    checkbox_help(controls_keyboard_mouse,L"Mouse button held while using free look or photo mode to capture mouse movement.");
    restart_note(L"Click a binding, press a key, then release. Delete clears it.",302,226,340);

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
    controls_controller_enabled=control(L"BUTTON",L"Enable controller remapping",BS_AUTOCHECKBOX|WS_TABSTOP,290,210,350,28,0);
    controls_save=control(L"BUTTON",L"Save settings",WS_TABSTOP,14,210,140,30,CONTROLS_SAVE);
    checkbox_help(control(L"BUTTON",L"Defaults",WS_TABSTOP,162,210,120,30,CONTROLS_DEFAULTS),L"Restore the vanilla controller mapping. LB and RB are unused by default. Save settings to apply.");
    checkbox_help(controls_controller_enabled,L"Enable the SDL3 XInput proxy remap table. Restart the game after saving. The game receives the virtual controls selected on this tab. Chords such as L3+R3 require every listed button.");
    restart_note(L"Click a binding, press one button or a chord, then release. Delete clears it.",14,246,630);
    controls_reload();
}
#endif
