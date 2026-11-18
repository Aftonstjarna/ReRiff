#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BOOL connected,available=TRUE,float_ok,full_ok;static int floats,fulls,dismisses;static const char *submitted="\xe3\x81\xa4\xe3\x81\x8d";
static int desktops;static BOOL desktop_ok;
static BOOL desktop(void){desktops++;return desktop_ok;}
static BOOL connection(void){return connected;}
static BOOL binding(void){return available;}
static BYTE __thiscall floating(void *u,int mode,int x,int y,int w,int h){(void)u;if(mode||x!=10||y!=20||w!=300||h!=40)abort();floats++;return float_ok;}
static BYTE __thiscall full(void *u,int mode,int lines,const char *description,unsigned count,const char *initial){(void)u;if(mode||lines||strcmp(description,"Search music")||count!=255||strcmp(initial,"before"))abort();fulls++;return full_ok;}
static BYTE __thiscall dismiss(void *u){(void)u;dismisses++;return TRUE;}
static unsigned __thiscall length(void *u){(void)u;return (unsigned)strlen(submitted)+1;}
static BYTE __thiscall text(void *u,char *buffer,unsigned size){(void)u;if(size!=strlen(submitted)+1)abort();memcpy(buffer,submitted,size);return TRUE;}
int main(void){
    RECT rect={10,20,310,60};steam_keyboard_desktop=desktop;steam_controller_connected=connection;steam_keyboard_available=binding;steam_keyboard_floating=floating;steam_keyboard_full=full;steam_keyboard_dismiss=dismiss;steam_keyboard_length=length;steam_keyboard_text=text;
    CHECK(sizeof(SteamKeyboardCallback)==12&&steam_keyboard_size(&steam_keyboard_full_callback)==12&&steam_keyboard_size(&steam_keyboard_float_callback)==1);
    CHECK(text_entry_begin_inline(L"before"));CHECK(!steam_keyboard_open(L"before",&rect,FALSE)&&!floats&&!fulls&&!steam_keyboard_waiting);
    connected=TRUE;available=FALSE;CHECK(!steam_keyboard_open(L"before",&rect,FALSE)&&!floats&&!fulls);available=TRUE;
    float_ok=TRUE;CHECK(steam_keyboard_open(L"before",&rect,FALSE)&&floats==1&&!fulls&&steam_keyboard_waiting==2);CHECK(!steam_keyboard_open(L"before",&rect,FALSE)&&floats==1);
    picker_search_controller_armed=TRUE;picker_search_controller(0x2000,TRUE);CHECK(text_entry_action==-1);InterlockedExchange(&text_entry_action,0);
    text_entry_message(NULL,WM_CHAR,L'!',0);CHECK(!wcscmp(text_entry_model.value,L"before!"));
    steam_keyboard_run(&steam_keyboard_float_callback,NULL);CHECK(!steam_keyboard_waiting&&text_entry_active&&!text_entry_action);
    float_ok=FALSE;full_ok=TRUE;CHECK(steam_keyboard_open(L"before",&rect,FALSE)&&steam_keyboard_waiting==1&&fulls==1);
    struct {BYTE accepted,pad[3];DWORD bytes,app;} event={1,{0},7,351990};
    steam_keyboard_run(&steam_keyboard_full_callback,&event);CHECK(!steam_keyboard_waiting&&!wcscmp(text_entry_model.value,L"つき")&&text_entry_action==1);
    InterlockedExchange(&text_entry_action,0);CHECK(steam_keyboard_open(L"before",&rect,FALSE));event.accepted=0;steam_keyboard_run(&steam_keyboard_full_callback,&event);CHECK(text_entry_action==-1&&!steam_keyboard_waiting);
    InterlockedExchange(&text_entry_action,0);CHECK(steam_keyboard_open(L"before",&rect,FALSE));steam_keyboard_forget();text_entry_end_inline();CHECK(text_entry_begin_inline(L"new session"));event.accepted=1;
    CHECK(!steam_keyboard_open(L"before",&rect,FALSE));steam_keyboard_run(&steam_keyboard_full_callback,&event);CHECK(!wcscmp(text_entry_model.value,L"new session")&&!text_entry_action&&!steam_keyboard_waiting);
    float_ok=TRUE;CHECK(steam_keyboard_open(L"before",&rect,FALSE));steam_keyboard_forget();CHECK(dismisses==1);steam_keyboard_run(&steam_keyboard_float_callback,NULL);CHECK(!steam_keyboard_waiting);
    float_ok=full_ok=FALSE;CHECK(!steam_keyboard_open(L"before",&rect,FALSE)&&!steam_keyboard_waiting&&text_entry_active);
    CHECK(desktops==1);desktop_ok=TRUE;CHECK(steam_keyboard_open(L"before",&rect,FALSE)&&desktops==2&&!steam_keyboard_waiting);
    connected=FALSE;float_ok=TRUE;CHECK(steam_keyboard_open(L"before",&rect,TRUE));steam_keyboard_run(&steam_keyboard_float_callback,NULL);
    text_entry_end_inline();puts("PASS: connected-controller gate, Steam floating/full fallback, single request, UTF-8 submit, cancellation, cross-device cancellation, stale callback fence and unavailable-keyboard fallback");return 0;
}
