/* No editable native game widget was found. Extend the original confirmation
 * overlay with one shared UTF-16 editor; the window thread never calls game UI. */
#include "text_entry_model.h"
typedef void (*TextEntryResult)(void *,const WCHAR *);
static CRITICAL_SECTION text_entry_lock;static INIT_ONCE text_entry_once=INIT_ONCE_STATIC_INIT;
static TextEntryModel text_entry_model;static TextEntryHistory text_entry_history;
static BOOL text_entry_typing;static DWORD text_entry_typed_at;static volatile LONG text_entry_active,text_entry_action;
typedef BOOL (*TextEntryValidator)(const WCHAR *);
static TextEntryValidator text_entry_validator;static BOOL text_entry_error;
static TextEntryResult text_entry_result;static DWORD text_entry_release_until;static WCHAR text_entry_surrogate;
static BOOL CALLBACK text_entry_initialize(PINIT_ONCE once,PVOID argument,PVOID *context){(void)once;(void)argument;(void)context;InitializeCriticalSection(&text_entry_lock);return TRUE;}
static BOOL text_entry_busy(void){return text_entry_active||(text_entry_release_until&&(LONG)(text_entry_release_until-GetTickCount())>0);}
static void text_entry_complete(void *owner,int choice){
    WCHAR value[TEXT_ENTRY_CAPACITY];EnterCriticalSection(&text_entry_lock);wcscpy(value,text_entry_model.value);LeaveCriticalSection(&text_entry_lock);
    TextEntryResult result=text_entry_result;text_entry_result=NULL;InterlockedExchange(&text_entry_active,0);text_entry_release_until=GetTickCount()+180;
    if(choice==1&&result)result(owner,value);
}
static BOOL text_entry_begin(void *owner,const WCHAR *title,const WCHAR *initial,TextEntryResult result,TextEntryValidator validator){
    if(text_entry_active||!initial||!result)return FALSE;
    InitOnceExecuteOnce(&text_entry_once,text_entry_initialize,NULL,NULL);
    EnterCriticalSection(&text_entry_lock);text_model_init(&text_entry_model,initial,2047);text_history_init(&text_entry_history,&text_entry_model);text_entry_typing=FALSE;text_entry_surrogate=0;LeaveCriticalSection(&text_entry_lock);
    text_entry_result=result;text_entry_validator=validator;text_entry_error=FALSE;InterlockedExchange(&text_entry_action,0);InterlockedExchange(&text_entry_active,1);
    if(!save_prompt_begin_caption(owner,text_entry_complete,title,L"Type or paste an address.\nEnter accepts; Esc cancels.")){InterlockedExchange(&text_entry_active,0);text_entry_result=NULL;return FALSE;}
    return TRUE;
}
static void text_entry_caption(void *dialog){
    if(!text_entry_active)return;WCHAR line[128];unsigned out=0;
    EnterCriticalSection(&text_entry_lock);
    unsigned length=(unsigned)wcslen(text_entry_model.value),cursor=text_entry_model.cursor,start=cursor>22?cursor-22:0;
    if(start&&text_low(text_entry_model.value[start]))start--;
    if(start)line[out++]=0x2026;
    for(unsigned i=start;i<=length&&i<start+34&&out<100;i++){
        if(i==cursor)line[out++]=L'|';if(i<length)line[out++]=text_entry_model.value[i];
    }
    if(start+34<length)line[out++]=0x2026;line[out]=0;
    LeaveCriticalSection(&text_entry_lock);
    WCHAR message[256];_snwprintf(message,256,L"%ls\n\n%ls\nEnter accepts; Esc cancels.",line,text_entry_error?L"Use http:// or https:// with a host.":L"Ctrl+A: select all; Ctrl+Z: undo.");
    void *item=((void **)dialog)[66];if(readable(item,484))raw_text(item,message);
}
static void text_entry_paste(HWND window){
    if(!OpenClipboard(window))return;HANDLE h=GetClipboardData(CF_UNICODETEXT);
    if(h){const WCHAR *p=GlobalLock(h);if(p){SIZE_T size=GlobalSize(h)/2;unsigned count=0;while(count<size&&count<2047&&p[count])count++;
        EnterCriticalSection(&text_entry_lock);TextEntryModel before=text_entry_model;text_model_insert(&text_entry_model,p,count);text_history_record(&text_entry_history,&before,&text_entry_model,FALSE);text_entry_typing=FALSE;LeaveCriticalSection(&text_entry_lock);GlobalUnlock(h);}}
    CloseClipboard();
}
static BOOL text_entry_message(HWND window,UINT message,WPARAM key,LPARAM flags){
    (void)flags;if(!text_entry_active)return FALSE;
    if(message==WM_KEYDOWN){
        BOOL ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        if(key==VK_RETURN){InterlockedExchange(&text_entry_action,1);return TRUE;}
        if(key==VK_ESCAPE){InterlockedExchange(&text_entry_action,-1);return TRUE;}
        if(ctrl&&key=='V'){text_entry_paste(window);return TRUE;}
        EnterCriticalSection(&text_entry_lock);
        TextEntryModel before=text_entry_model;
        if(ctrl&&(key=='Z'||key=='Y'))text_history_step(&text_entry_history,&text_entry_model,key=='Y'||shift);
        else if(ctrl&&key=='A'){text_entry_model.anchor=0;text_entry_model.cursor=(unsigned)wcslen(text_entry_model.value);}
        else text_model_key(&text_entry_model,(unsigned)key,shift);
        if(!(ctrl&&(key=='Z'||key=='Y')))text_history_record(&text_entry_history,&before,&text_entry_model,FALSE);
        if(ctrl||key==VK_LEFT||key==VK_RIGHT||key==VK_HOME||key==VK_END||key==VK_BACK||key==VK_DELETE)text_entry_typing=FALSE;
        text_entry_surrogate=0;LeaveCriticalSection(&text_entry_lock);return TRUE;
    }
    if(message==WM_CHAR||message==WM_IME_CHAR){
        WCHAR c=(WCHAR)key;EnterCriticalSection(&text_entry_lock);
        if(text_high(c))text_entry_surrogate=c;
        else {WCHAR text[3]={c,0,0};unsigned count=1;if(text_low(c)&&text_entry_surrogate){text[0]=text_entry_surrogate;text[1]=c;count=2;}
            TextEntryModel before=text_entry_model;DWORD now=GetTickCount();
            text_model_insert(&text_entry_model,text,count);
            text_history_record(&text_entry_history,&before,&text_entry_model,text_entry_typing&&now-text_entry_typed_at<750&&before.anchor==before.cursor);
            if(wcscmp(before.value,text_entry_model.value)){text_entry_typing=TRUE;text_entry_typed_at=now;}
            text_entry_surrogate=0;}
        LeaveCriticalSection(&text_entry_lock);return TRUE;
    }
    return message==WM_KEYUP||message==WM_MOUSEWHEEL;
}

static BOOL text_entry_confirm(int choice){
    if(!text_entry_active||!choice||!text_entry_validator)return TRUE;
    EnterCriticalSection(&text_entry_lock);BOOL valid=text_entry_validator(text_entry_model.value);LeaveCriticalSection(&text_entry_lock);
    text_entry_error=!valid;return valid;
}
static BOOL text_entry_overlay_tick(void *dialog){
    (void)dialog;if(!text_entry_active)return FALSE;
    LONG action=InterlockedExchange(&text_entry_action,0);if(!action)return FALSE;
    if(!text_entry_confirm(action>0)){text_entry_caption(dialog);return TRUE;}
    save_prompt_choice=action>0?1:0;overlay_pop(*overlay_manager,TRUE);return TRUE;
}
