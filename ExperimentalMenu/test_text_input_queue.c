#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static LRESULT CALLBACK queue_fixture_proc(HWND w,UINT m,WPARAM k,LPARAM l){if(text_entry_message(w,m,k,l))return 0;return DefWindowProcW(w,m,k,l);}
int main(void){
    WNDCLASSW cls={0};cls.lpfnWndProc=queue_fixture_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ReRiffQueueFixture";CHECK(RegisterClassW(&cls));
    HWND w=CreateWindowW(cls.lpszClassName,L"",WS_POPUP,0,0,800,600,NULL,NULL,cls.hInstance,NULL);CHECK(w);
    text_input_attach_queue(w);CHECK(text_input_queue_hook);CHECK(text_entry_begin_inline(L""));
    CHECK(PostMessageW(w,WM_KEYDOWN,'B',0));CHECK(PostMessageW(w,WM_CHAR,'b',0));MSG message;
    while(PeekMessageW(&message,w,WM_KEYFIRST,WM_KEYLAST,PM_REMOVE)){
        if(message.message==WM_CHAR)CHECK(text_input_queue_chars==1&&text_entry_model.value[0]==0);
        DispatchMessageW(&message);
    }
    CHECK(text_input_queue_keys==1&&text_input_queue_chars==1&&text_input_keys==1&&text_input_chars==1&&!wcscmp(text_entry_model.value,L"b"));
    SendMessageW(w,WM_CHAR,'c',0);CHECK(text_input_queue_chars==1&&text_input_chars==2&&!wcscmp(text_entry_model.value,L"bc"));
    text_entry_end_inline();PostMessageW(w,WM_CHAR,'d',0);CHECK(PeekMessageW(&message,w,WM_CHAR,WM_CHAR,PM_REMOVE));DispatchMessageW(&message);CHECK(text_input_queue_chars==1);
    CHECK(UnhookWindowsHookEx(text_input_queue_hook));text_input_queue_hook=NULL;DestroyWindow(w);
    puts("PASS: thread-local queue observer sees queued input before dispatch, preserves input, distinguishes sent messages, and ignores inactive fields");return 0;
}
