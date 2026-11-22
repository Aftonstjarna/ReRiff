#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE owner[512],manager[64],field[512];static WCHAR queried[256];static int queries,selections,closed,original_calls;static HWND window;
static BOOL unexpected_keyboard(void){abort();return FALSE;}
static BOOL back_held;static BOOL back(void){return back_held;}
static HWND test_window(void){return window;}
static void __thiscall draw(void *item,const WCHAR *value){((float *)item)[12]=(float)wcslen(value)*10;((float *)item)[13]=20;}
static int __cdecl bridge(int op,void *menu,const WCHAR *query,int arg){if(menu!=owner)abort();if(op==1){wcscpy(queried,query);queries++;}if(op==3)closed++;if(op==4){if(arg!=2)abort();selections++;}return op==2?3:1;}
static void __thiscall original(void *menu,int selection){(void)menu;(void)selection;original_calls++;}
int main(void){
    steam_keyboard_available=unexpected_keyboard;picker_search_back=back;
    InitOnceExecuteOnce(&text_entry_once,text_entry_initialize,NULL,NULL);
    window=CreateWindowW(L"STATIC",L"Search fixture",WS_POPUP|WS_VISIBLE,0,0,800,600,NULL,NULL,GetModuleHandleW(NULL),NULL);CHECK(window);SetForegroundWindow(window);display_window=test_window;
    *(void ***)owner=(void **)((BYTE *)GetModuleHandleW(NULL)+0x7a54bc);*(DWORD *)(owner+28)=1;
    void *mgr=manager;overlay_manager=&mgr;((void **)manager)[11]=owner;
    picker_search_bridge=bridge;picker_search_original_input=original;picker_search_owner=owner;picker_search_field=field;picker_search_borrowed=TRUE;
    picker_search_available=1;picker_search_rect=(RECT){0,0,400,40};
    CHECK(!scroll_window);picker_search_before(owner);CHECK(scroll_window==window);
    BYTE keyboard[256]={0};keyboard[VK_CONTROL]=0x80;CHECK(SetKeyboardState(keyboard));
    SendMessageW(window,WM_KEYDOWN,'F',0);CHECK(picker_search_request==1);
    InterlockedExchange(&picker_random_request,0);SendMessageW(window,WM_KEYDOWN,'R',0);CHECK(picker_random_request==1);InterlockedExchange(&picker_random_request,0);
    memset(keyboard,0,sizeof(keyboard));CHECK(SetKeyboardState(keyboard));
    CHECK(picker_search_before(owner)&&text_entry_inline&&text_entry_active);
    SendMessageW(window,WM_KILLFOCUS,0,0);CHECK(text_entry_inline&&!text_entry_action);
    CHECK(!text_entry_overlay_tick(owner));CHECK(!save_prompt_owner);
    SendMessageW(window,WM_CHAR,L'つ',0);SendMessageW(window,WM_CHAR,L'き',0);
    picker_search_before(owner);CHECK(!wcscmp(picker_search_query,L"つき")&&queries==0);
    picker_search_changed=GetTickCount()-200;CHECK(picker_search_before(owner));CHECK(queries==1&&!wcscmp(queried,L"つき")&&picker_search_filtered);
    picker_search_input(owner,6);CHECK(!selections&&!original_calls);
    CHECK(text_entry_message(window,WM_KEYDOWN,VK_RETURN,0));CHECK(picker_search_before(owner));CHECK(!text_entry_inline&&!text_entry_active);
    picker_search_release=0;picker_search_input(owner,6);CHECK(selections==1&&!original_calls);
    back_held=TRUE;CHECK(picker_search_before(owner));CHECK(!picker_search_filtered&&picker_search_wait_neutral);
    picker_search_release=GetTickCount()-1;CHECK(picker_search_before(owner)&&picker_search_wait_neutral);
    back_held=FALSE;CHECK(picker_search_before(owner));CHECK(!picker_search_wait_neutral);
    picker_search_release=0;picker_search_filtered=TRUE;
    picker_search_input(owner,3);CHECK(!original_calls); /* Do not favourite a search. */
    picker_search_input(owner,2);CHECK(!picker_search_filtered&&!picker_search_query[0]&&queries==3&&!queried[0]);
    picker_search_input(owner,6);CHECK(original_calls==1);
    CHECK(picker_search_message(window,WM_LBUTTONDOWN,0,MAKELPARAM(100,20)));CHECK(picker_search_before(owner)&&text_entry_inline);
    text_entry_message(window,WM_CHAR,L'x',0);text_entry_message(window,WM_KEYDOWN,VK_ESCAPE,0);CHECK(picker_search_before(owner));CHECK(!text_entry_inline&&!picker_search_filtered);
    CHECK(text_entry_begin_inline(L"0123456789abcdefghijklmnopqrstuvwxyz"));WCHAR line[128];text_entry_line(line,8,text_entry_blink_at);CHECK(wcslen(line)<=11&&wcschr(line,L'|'));
    ((void **)owner)[75]=field;((float *)owner)[77]=120;raw_text=draw;picker_search_columns=96;picker_search_after(owner);CHECK(((float *)field)[12]<=80);
    text_entry_end_inline();CHECK(text_entry_begin_inline(L""));text_entry_message(window,WM_CHAR,0xd83c,0);text_entry_message(window,WM_CHAR,0xdfb5,0);CHECK(wcslen(text_entry_model.value)==2);
    text_entry_end_inline();picker_search_controller_armed=FALSE;
    picker_search_controller(0x8000,TRUE);CHECK(!picker_search_request);
    picker_search_controller(0,TRUE);picker_search_controller(0x8000,TRUE);CHECK(picker_search_request==2);
    picker_search_controller(0x8000,TRUE);CHECK(picker_search_request==2);
    CHECK(picker_search_before(owner)&&text_entry_inline);
    SendMessageW(window,WM_KEYDOWN,VK_ESCAPE,0);picker_search_before(owner);CHECK(!text_entry_inline&&!picker_search_filtered);
    CHECK(text_entry_begin_inline(L"controller"));picker_search_controller(0,TRUE);picker_search_controller(0x1000,TRUE);CHECK(text_entry_action==1);
    InterlockedExchange(&text_entry_action,0);picker_search_controller(0,TRUE);picker_search_controller(0x2000,TRUE);CHECK(text_entry_action==-1);
    text_entry_end_inline();keyboard[VK_CONTROL]=0x80;CHECK(SetKeyboardState(keyboard));
    SendMessageW(window,WM_KEYDOWN,'F',0);memset(keyboard,0,sizeof(keyboard));CHECK(SetKeyboardState(keyboard));
    CHECK(picker_search_before(owner)&&text_entry_inline);picker_search_controller(0,TRUE);picker_search_controller(0x2000,TRUE);
    picker_search_before(owner);CHECK(!text_entry_inline&&!picker_search_filtered);
    picker_search_controller(0,FALSE);picker_search_controller(0x8000,TRUE);CHECK(!picker_search_request);
    picker_search_wait_neutral=FALSE;picker_search_release=0;
    CHECK(text_entry_begin_inline(L"mouse"));
    CHECK(!picker_search_message(window,WM_RBUTTONDOWN,0,MAKELPARAM(100,100)));
    CHECK(!text_entry_action);
    CHECK(picker_search_message(window,WM_LBUTTONDOWN,0,MAKELPARAM(100,100)));
    CHECK(text_entry_action==1&&!picker_search_request);
    int selected_before=selections;CHECK(picker_search_before(owner));
    CHECK(!text_entry_inline&&picker_search_filtered&&!wcscmp(queried,L"mouse"));
    picker_search_input(owner,6);CHECK(selections==selected_before); /* No click-through. */
    picker_search_release=0;picker_search_input(owner,6);CHECK(selections==selected_before+1);
    CHECK(!picker_search_message(window,WM_LBUTTONDOWN,0,MAKELPARAM(100,100)));
    CHECK(picker_search_message(window,WM_LBUTTONDOWN,0,MAKELPARAM(100,20)));
    CHECK(picker_search_before(owner)&&text_entry_inline);
    CHECK(picker_search_message(window,WM_LBUTTONDOWN,0,MAKELPARAM(100,20)));
    CHECK(picker_search_before(owner)&&!text_entry_inline); /* Clicking the field accepts too. */
    picker_search_forget(owner);CHECK(closed==1&&!picker_search_owner&&!picker_search_available&&!text_entry_active&&!text_entry_inline);
    picker_search_forget(owner);CHECK(closed==1);CHECK(!picker_search_message(window,WM_KEYDOWN,'F',0));
    DestroyWindow(window);puts("PASS: inline field activation, Unicode editing, debounce, native result routing, Enter/Esc isolation, small viewport, overlay separation and cleanup");return 0;
}
