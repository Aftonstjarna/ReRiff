/* No editable native game widget was found. Extend the original confirmation
 * overlay with one shared UTF-16 editor; the window thread never calls game UI. */
#include "text_entry_model.h"
typedef void (*TextEntryResult)(void *,const WCHAR *);
static CRITICAL_SECTION text_entry_lock;static INIT_ONCE text_entry_once=INIT_ONCE_STATIC_INIT;
static TextEntryModel text_entry_model;static TextEntryHistory text_entry_history;
static BOOL text_entry_inline;
static BOOL text_entry_typing;static DWORD text_entry_typed_at,text_entry_blink_at,text_entry_blink_ms;static volatile LONG text_entry_active,text_entry_action;
typedef BOOL (*TextEntryValidator)(const WCHAR *);
static TextEntryValidator text_entry_validator;static BOOL text_entry_error;
static TextEntryResult text_entry_result;static DWORD text_entry_release_until;static WCHAR text_entry_surrogate;
static volatile LONG text_input_keys,text_input_chars,text_input_unicode,text_input_ime,text_input_focus,text_input_sent;
static volatile LONG text_input_queue_keys,text_input_queue_chars,text_input_queue_other;
static HHOOK text_input_queue_hook;static HWND text_input_queue_window;
static LRESULT CALLBACK text_input_queue(int code,WPARAM remove,LPARAM parameter){
    if(code>=0&&remove==PM_REMOVE&&text_entry_active){
        const MSG *message=(const MSG *)parameter;
        BOOL key=message->message==WM_KEYDOWN||message->message==WM_SYSKEYDOWN;
        BOOL character=message->message==WM_CHAR||message->message==WM_UNICHAR||message->message==WM_IME_CHAR;
        if(key||character){
            if(message->hwnd!=text_input_queue_window)InterlockedIncrement(&text_input_queue_other);
            else if(key)InterlockedIncrement(&text_input_queue_keys);
            else InterlockedIncrement(&text_input_queue_chars);
        }
    }
    /* Observation only: never change, consume, translate, or replay input. */
    return CallNextHookEx(text_input_queue_hook,code,remove,parameter);
}
static void text_input_attach_queue(HWND window){
    if(text_input_queue_hook)return;
    DWORD process=0,thread=GetWindowThreadProcessId(window,&process);
    if(!thread||process!=GetCurrentProcessId())return;
    HMODULE module=NULL;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)(void *)text_input_queue,&module))return;
    text_input_queue_window=window;
    text_input_queue_hook=SetWindowsHookExW(WH_GETMESSAGE,text_input_queue,module,thread);
    log_line(text_input_queue_hook?"Text input queue observer attached to game window thread.":"Text input queue observer unavailable.");
}
static DWORD text_input_trace_at;static LONG text_input_trace_total;
static void text_input_trace(BOOL reset,BOOL finish){
    if(reset){text_input_keys=text_input_chars=text_input_unicode=text_input_ime=text_input_focus=text_input_sent=0;text_input_queue_keys=text_input_queue_chars=text_input_queue_other=0;text_input_trace_total=0;}
    LONG total=text_input_keys+text_input_chars+text_input_unicode+text_input_ime+text_input_focus+text_input_queue_keys+text_input_queue_chars+text_input_queue_other;
    DWORD now=GetTickCount();
    if(reset||finish||(total!=text_input_trace_total&&now-text_input_trace_at>=1000)){
        char line[320];snprintf(line,sizeof(line),"Text input %s: queue_key=%ld queue_char=%ld queue_other_target=%ld; keydown=%ld char=%ld unichar=%ld ime=%ld focus=%ld sent=%ld; contents not recorded.",reset?"begin":finish?"end":"events",text_input_queue_keys,text_input_queue_chars,text_input_queue_other,text_input_keys,text_input_chars,text_input_unicode,text_input_ime,text_input_focus,text_input_sent);log_line(line);
        text_input_trace_at=now;text_input_trace_total=total;
    }
}
static BOOL CALLBACK text_entry_initialize(PINIT_ONCE once,PVOID argument,PVOID *context){(void)once;(void)argument;(void)context;InitializeCriticalSection(&text_entry_lock);return TRUE;}
static BOOL text_entry_busy(void){return text_entry_active||(text_entry_release_until&&(LONG)(text_entry_release_until-GetTickCount())>0);}
static void text_entry_complete(void *owner,int choice){
    text_input_trace(FALSE,TRUE);
    WCHAR value[TEXT_ENTRY_CAPACITY];EnterCriticalSection(&text_entry_lock);wcscpy(value,text_entry_model.value);LeaveCriticalSection(&text_entry_lock);
    TextEntryResult result=text_entry_result;text_entry_result=NULL;InterlockedExchange(&text_entry_active,0);text_entry_release_until=GetTickCount()+180;
    if(choice==1&&result)result(owner,value);
}
static BOOL text_entry_begin_inline(const WCHAR *initial){
    if(text_entry_active||!initial)return FALSE;
    text_input_trace(TRUE,FALSE);
    InitOnceExecuteOnce(&text_entry_once,text_entry_initialize,NULL,NULL);
    EnterCriticalSection(&text_entry_lock);text_model_init(&text_entry_model,initial,255);text_history_init(&text_entry_history,&text_entry_model);text_entry_typing=FALSE;text_entry_surrogate=0;text_entry_blink_at=GetTickCount();text_entry_blink_ms=GetCaretBlinkTime();if(!text_entry_blink_ms)text_entry_blink_ms=500;LeaveCriticalSection(&text_entry_lock);
    text_entry_inline=TRUE;text_entry_result=NULL;text_entry_validator=NULL;text_entry_error=FALSE;InterlockedExchange(&text_entry_action,0);InterlockedExchange(&text_entry_active,1);return TRUE;
}
static void text_entry_end_inline(void){
    if(text_entry_inline)text_input_trace(FALSE,TRUE);
    if(!text_entry_inline)return;
    InterlockedExchange(&text_entry_active,0);text_entry_inline=FALSE;text_entry_surrogate=0;InterlockedExchange(&text_entry_action,0);
}
static BOOL text_entry_begin(void *owner,const WCHAR *title,const WCHAR *initial,TextEntryResult result,TextEntryValidator validator){
    if(text_entry_active||!initial||!result)return FALSE;
    text_input_trace(TRUE,FALSE);
    InitOnceExecuteOnce(&text_entry_once,text_entry_initialize,NULL,NULL);
    EnterCriticalSection(&text_entry_lock);text_model_init(&text_entry_model,initial,2047);text_history_init(&text_entry_history,&text_entry_model);text_entry_typing=FALSE;text_entry_surrogate=0;text_entry_blink_at=GetTickCount();text_entry_blink_ms=GetCaretBlinkTime();if(!text_entry_blink_ms)text_entry_blink_ms=500;LeaveCriticalSection(&text_entry_lock);
    text_entry_inline=FALSE;text_entry_result=result;text_entry_validator=validator;text_entry_error=FALSE;InterlockedExchange(&text_entry_action,0);InterlockedExchange(&text_entry_active,1);
    if(!save_prompt_begin_caption(owner,text_entry_complete,title,L"")){InterlockedExchange(&text_entry_active,0);text_entry_result=NULL;return FALSE;}
    return TRUE;
}
static void text_entry_line(WCHAR *line,unsigned visible,DWORD now){
    text_input_trace(FALSE,FALSE);
    unsigned out=0;
    EnterCriticalSection(&text_entry_lock);
    /* Keep ordinary addresses whole; only longer values need a viewport.
     * The original message item retains its native panel-width wrapping. */
    unsigned length=(unsigned)wcslen(text_entry_model.value),cursor=text_entry_model.cursor,start=0;
    unsigned lead=visible>12?visible-12:visible/2;
    if(length>visible&&cursor>lead){start=cursor-lead;if(start>length-visible)start=length-visible;}
    if(start&&text_low(text_entry_model.value[start]))start--;
    unsigned end=start+visible;if(end>length)end=length;
    if(end<length&&end&&text_high(text_entry_model.value[end-1]))end--;
    BOOL caret=text_entry_blink_ms==INFINITE||((now-text_entry_blink_at)/text_entry_blink_ms)%2==0;
    if(start)line[out++]=0x2026;
    for(unsigned i=start;i<=end;i++){
        if(i==cursor)line[out++]=caret?L'|':L' ';if(i<end)line[out++]=text_entry_model.value[i];
    }
    if(end<length)line[out++]=0x2026;line[out]=0;
    LeaveCriticalSection(&text_entry_lock);
 }
static void text_entry_caption_at(void *dialog,DWORD now){
    if(!text_entry_active||text_entry_inline)return;WCHAR line[128];text_entry_line(line,48,now);
    WCHAR message[256];_snwprintf(message,256,L"%ls%ls",line,text_entry_error?L"\n\nUse http:// or https:// with a host.":L"");
    void *item=((void **)dialog)[66];if(readable(item,484))raw_text(item,message);
}
static void text_entry_caption(void *dialog){text_entry_caption_at(dialog,GetTickCount());}
static void text_entry_paste(HWND window){
    if(!OpenClipboard(window))return;HANDLE h=GetClipboardData(CF_UNICODETEXT);
    if(h){const WCHAR *p=GlobalLock(h);if(p){SIZE_T size=GlobalSize(h)/2;unsigned count=0;while(count<size&&count<2047&&p[count])count++;
        EnterCriticalSection(&text_entry_lock);TextEntryModel before=text_entry_model;text_model_insert(&text_entry_model,p,count);text_history_record(&text_entry_history,&before,&text_entry_model,FALSE);text_entry_typing=FALSE;text_entry_blink_at=GetTickCount();LeaveCriticalSection(&text_entry_lock);GlobalUnlock(h);}}
    CloseClipboard();
}
static BOOL text_entry_message(HWND window,UINT message,WPARAM key,LPARAM flags){
    (void)flags;if(!text_entry_active)return FALSE;
    BOOL input=message==WM_KEYDOWN||message==WM_CHAR||message==WM_IME_CHAR||message==WM_UNICHAR||message==WM_IME_COMPOSITION;
    if(input&&InSendMessageEx(NULL))InterlockedIncrement(&text_input_sent);
    if(message==WM_KEYDOWN)InterlockedIncrement(&text_input_keys);
    if(message==WM_CHAR||message==WM_IME_CHAR)InterlockedIncrement(&text_input_chars);
    if(message==WM_IME_COMPOSITION)InterlockedIncrement(&text_input_ime);
    if(message==WM_SETFOCUS||message==WM_KILLFOCUS)InterlockedIncrement(&text_input_focus);
    if(message==WM_UNICHAR){
        InterlockedIncrement(&text_input_unicode);
        if(key==UNICODE_NOCHAR)return TRUE;
        if(key>0x10ffff||(key>=0xd800&&key<=0xdfff))return TRUE;
        WCHAR text[3]={(WCHAR)key,0,0};unsigned count=1;
        if(key>0xffff){DWORD scalar=(DWORD)key-0x10000;text[0]=(WCHAR)(0xd800+(scalar>>10));text[1]=(WCHAR)(0xdc00+(scalar&0x3ff));count=2;}
        EnterCriticalSection(&text_entry_lock);TextEntryModel before=text_entry_model;
        text_model_insert(&text_entry_model,text,count);text_history_record(&text_entry_history,&before,&text_entry_model,FALSE);
        text_entry_surrogate=0;text_entry_typing=FALSE;text_entry_blink_at=GetTickCount();LeaveCriticalSection(&text_entry_lock);return TRUE;
    }
    if(message==WM_KEYDOWN){
        BOOL ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        if(key==VK_RETURN){InterlockedExchange(&text_entry_action,1);return TRUE;}
        if(key==VK_ESCAPE){InterlockedExchange(&text_entry_action,-1);return TRUE;}
        if(ctrl&&key=='V'){text_entry_paste(window);return TRUE;}
        EnterCriticalSection(&text_entry_lock);
        text_entry_blink_at=GetTickCount();TextEntryModel before=text_entry_model;
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
            if(wcscmp(before.value,text_entry_model.value)){text_entry_typing=TRUE;text_entry_typed_at=now;text_entry_blink_at=now;}
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
    (void)dialog;if(!text_entry_active||text_entry_inline)return FALSE;
    LONG action=InterlockedExchange(&text_entry_action,0);if(!action)return FALSE;
    if(!text_entry_confirm(action>0)){text_entry_caption(dialog);return TRUE;}
    save_prompt_choice=action>0?1:0;overlay_pop(*overlay_manager,TRUE);return TRUE;
}
