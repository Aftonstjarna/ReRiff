/* SteamUtils010 / CCallbackBase ABI from Valve's published Steamworks headers.
 * Use the game's SteamClient and pipe; never initialize another Steam session.
 * Floating input sends OS keys to the shared editor. Full-screen fallback
 * delivers UTF-8 only inside GamepadTextInputDismissed_t. */
#include <xinput.h>
typedef void (__cdecl *SteamRegisterFn)(void *,int);
typedef struct SteamKeyboardCallback {void **table;BYTE flags,padding[3];int id;} SteamKeyboardCallback;
static void *steam_keyboard_utils;
static SteamRegisterFn steam_keyboard_register;
static volatile LONG steam_keyboard_waiting;
static BOOL steam_keyboard_owned;
static BOOL steam_keyboard_registered;
static BYTE (__thiscall *steam_keyboard_floating)(void *,int,int,int,int,int);
static BYTE (__thiscall *steam_keyboard_full)(void *,int,int,const char *,unsigned,const char *);
static unsigned (__thiscall *steam_keyboard_length)(void *);
static BYTE (__thiscall *steam_keyboard_text)(void *,char *,unsigned);
static BYTE (__thiscall *steam_keyboard_dismiss)(void *);
static BOOL steam_controller_connected_native(void){
    HMODULE module=GetModuleHandleW(L"XINPUT1_3.dll");if(!module)return FALSE;
    typedef DWORD (WINAPI *StateFn)(DWORD,XINPUT_STATE *);StateFn state=(StateFn)GetProcAddress(module,"XInputGetState");if(!state)return FALSE;
    for(DWORD i=0;i<XUSER_MAX_COUNT;i++){XINPUT_STATE value;if(state(i,&value)==ERROR_SUCCESS)return TRUE;}return FALSE;
}
static BOOL (*steam_controller_connected)(void)=steam_controller_connected_native;
static void __thiscall steam_keyboard_run(void *self,void *event){
    SteamKeyboardCallback *callback=self;LONG expected=callback->id==714?1:2;
    if(steam_keyboard_waiting!=expected)return;
    BOOL owned=steam_keyboard_owned&&text_entry_inline&&text_entry_active;
    if(owned&&expected==1&&event){
        BYTE submitted=*(BYTE *)event;
        if(submitted){
            unsigned length=steam_keyboard_length(steam_keyboard_utils);char utf8[1025]={0};WCHAR wide[1025];
            if(length>0&&length<=sizeof(utf8)&&steam_keyboard_text(steam_keyboard_utils,utf8,length)
                &&memchr(utf8,0,length)&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,-1,wide,1025)){
                EnterCriticalSection(&text_entry_lock);
                TextEntryModel before=text_entry_model;text_entry_model.anchor=0;text_entry_model.cursor=(unsigned)wcslen(text_entry_model.value);
                text_model_insert(&text_entry_model,wide,(unsigned)wcslen(wide));text_history_record(&text_entry_history,&before,&text_entry_model,FALSE);
                text_entry_blink_at=GetTickCount();text_entry_typing=FALSE;
                LeaveCriticalSection(&text_entry_lock);InterlockedExchange(&text_entry_action,1);
            }
        }else InterlockedExchange(&text_entry_action,-1);
    }
    /* Floating input has already supplied WM_CHAR/Enter. Closing without
     * Enter leaves the editable field focused so no unrequested search runs. */
    steam_keyboard_owned=FALSE;InterlockedExchange(&steam_keyboard_waiting,0);
}
static void __thiscall steam_keyboard_result(void *self,void *event,BYTE failed,uint64_t call){(void)call;if(!failed)steam_keyboard_run(self,event);}
static int __thiscall steam_keyboard_size(void *self){return ((SteamKeyboardCallback *)self)->id==714?12:1;}
static void *steam_keyboard_table[3]={(void *)steam_keyboard_run,(void *)steam_keyboard_result,(void *)steam_keyboard_size};
static SteamKeyboardCallback steam_keyboard_full_callback={steam_keyboard_table,0,{0},714};
static SteamKeyboardCallback steam_keyboard_float_callback={steam_keyboard_table,0,{0},738};
static BOOL steam_keyboard_bind(void){
    if(steam_keyboard_registered)return TRUE;
    HMODULE module=GetModuleHandleW(L"steam_api.dll");if(!module)return FALSE;
    typedef void *(__cdecl *ClientFn)(void);typedef int (__cdecl *PipeFn)(void);
    ClientFn client_fn=(ClientFn)(void *)GetProcAddress(module,"SteamClient");PipeFn pipe_fn=(PipeFn)(void *)GetProcAddress(module,"SteamAPI_GetHSteamPipe");
    steam_keyboard_register=(SteamRegisterFn)(void *)GetProcAddress(module,"SteamAPI_RegisterCallback");
    if(!client_fn||!pipe_fn||!steam_keyboard_register)return FALSE;
    void *client=client_fn();int pipe=pipe_fn();if(!pipe||!readable(client,4)||!readable(*(void ***)client,40))return FALSE;
    typedef void *(__thiscall *UtilsFn)(void *,int,const char *);
    void *utils=((UtilsFn)(*(void ***)client)[9])(client,pipe,"SteamUtils010");
    if(!readable(utils,4)||!readable(*(void ***)utils,38*4))return FALSE;
    void **table=*(void ***)utils;steam_keyboard_utils=utils;
    steam_keyboard_full=(void *)table[20];steam_keyboard_length=(void *)table[21];steam_keyboard_text=(void *)table[22];
    steam_keyboard_floating=(void *)table[35];steam_keyboard_dismiss=(void *)table[37];
    steam_keyboard_register(&steam_keyboard_full_callback,714);steam_keyboard_register(&steam_keyboard_float_callback,738);steam_keyboard_registered=TRUE;return TRUE;
}
static BOOL (*steam_keyboard_available)(void)=steam_keyboard_bind;
/* Desktop Steam may decline both overlay APIs. Its keyboard protocol uses
 * OS keys too; dispatch once per edit, and do not claim an API callback. */
static BOOL steam_keyboard_desktop_native(void){
    HMODULE shell=LoadLibraryW(L"shell32.dll");if(!shell)return FALSE;
    typedef HINSTANCE (WINAPI *Execute)(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT);
    Execute execute=(Execute)(void *)GetProcAddress(shell,"ShellExecuteW");
    BOOL result=execute&&(INT_PTR)execute(NULL,L"open",L"steam://open/keyboard",NULL,NULL,SW_SHOWNOACTIVATE)>32;
    FreeLibrary(shell);return result;
}
static BOOL (*steam_keyboard_desktop)(void)=steam_keyboard_desktop_native;
/* Retained for protocol regression tests; search uses manual keyboards. */
static BOOL __attribute__((unused)) steam_keyboard_open(const WCHAR *initial,const RECT *field,BOOL controller_pressed){
    if(!text_entry_inline||steam_keyboard_waiting)return FALSE;
    /* A native Y press is itself evidence of a connected controller. Steam
     * Input can expose it to the game even when a separate XInput poll fails. */
    if(!controller_pressed&&!steam_controller_connected()){log_line("Search keyboard: no connected controller.");return FALSE;}
    if(!steam_keyboard_available()){log_line("Search keyboard: SteamUtils010 unavailable.");return FALSE;}
    steam_keyboard_owned=TRUE;InterlockedExchange(&steam_keyboard_waiting,2);
    if(steam_keyboard_floating(steam_keyboard_utils,0,field->left,field->top,field->right-field->left,field->bottom-field->top)){log_line("Search keyboard: floating request accepted.");return TRUE;}
    InterlockedExchange(&steam_keyboard_waiting,1);char utf8[1025];
    if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,initial,-1,utf8,sizeof(utf8),NULL,NULL)
        &&steam_keyboard_full(steam_keyboard_utils,0,0,"Search music",255,utf8)){log_line("Search keyboard: full-screen request accepted.");return TRUE;}
    log_line("Search keyboard: Steam declined floating and full-screen requests.");
    steam_keyboard_owned=FALSE;InterlockedExchange(&steam_keyboard_waiting,0);
    BOOL dispatched=steam_keyboard_desktop();log_line(dispatched?"Search keyboard: desktop protocol dispatched (visibility unconfirmed).":"Search keyboard: desktop protocol unavailable.");return dispatched;
}
static void steam_keyboard_forget(void){
    steam_keyboard_owned=FALSE;
    if(steam_keyboard_waiting==2&&steam_keyboard_dismiss)steam_keyboard_dismiss(steam_keyboard_utils);
    /* Keep a pending request fenced until its dismissal callback arrives. */
}
