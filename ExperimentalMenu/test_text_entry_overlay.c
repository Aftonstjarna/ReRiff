#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE manager[64],owner[512],dialog[512],message[512],title[512];static int pushes,pops,results;static WCHAR accepted[2048],drawn[256];
static void __cdecl push(void *m,int state){if(m!=manager||state!=9)abort();((void **)manager)[11]=dialog;pushes++;}
static void __cdecl pop(void *m,int yes){if(m!=manager||!yes)abort();((void **)manager)[11]=NULL;pops++;}
static void done(void *o,const WCHAR *text){if(o!=owner)abort();wcscpy(accepted,text);results++;}
static void __thiscall raw(void *item,const WCHAR *text){if(item==message)wcsncpy(drawn,text,255);}
int main(void){
    void *m=manager;overlay_manager=&m;overlay_push=push;overlay_pop=pop;prompt_installed=TRUE;raw_text=raw;
    ((void **)dialog)[65]=title;((void **)dialog)[66]=message;
    CHECK(text_entry_begin(owner,L"PRIMARY SERVER",L"https://example.com",done,server_text_valid));
    CHECK(!text_entry_begin(owner,L"SECONDARY SERVER",L"",done,NULL));CHECK(text_entry_busy());
    text_entry_caption_at(dialog,text_entry_blink_at);CHECK(wcsstr(drawn,L"https://example.com|")!=NULL);
    CHECK(text_entry_message(NULL,WM_CHAR,L'/',0));CHECK(text_entry_message(NULL,WM_UNICHAR,L'つ',0));
    CHECK(text_entry_message(NULL,WM_KEYDOWN,VK_RETURN,0));CHECK(text_entry_overlay_tick(dialog));CHECK(pops==1&&results==0);
    save_prompt_tick(owner);CHECK(results==1&&!wcscmp(accepted,L"https://example.com/つ")&&!text_entry_active);
    CHECK(text_entry_begin(owner,L"SECONDARY SERVER",L"https://cancel.example",done,server_text_valid));
    text_entry_message(NULL,WM_KEYDOWN,VK_ESCAPE,0);CHECK(text_entry_overlay_tick(dialog));save_prompt_tick(owner);CHECK(results==1&&!text_entry_active);
    CHECK(text_entry_begin(owner,L"PRIMARY SERVER",L"invalid",done,server_text_valid));
    text_entry_message(NULL,WM_KEYDOWN,VK_RETURN,0);CHECK(text_entry_overlay_tick(dialog)&&text_entry_active&&text_entry_error&&pops==2);
    text_entry_caption(dialog);CHECK(wcsstr(drawn,L"Use http://")!=NULL);
    text_model_init(&text_entry_model,L"http://example.com",2047);save_prompt_input(dialog,2);CHECK(pops==3);save_prompt_tick(owner);CHECK(results==2);
    CHECK(server_text_valid(L"")&&server_text_valid(L"https://example.com:5144/path")&&!server_text_valid(L"https://")&&!server_text_valid(L"http://bad host")&&!server_text_valid(L"file:///tmp/test"));
    CHECK(text_entry_begin(owner,L"SECONDARY SERVER",L"before",done,NULL));
    CHECK(OpenClipboard(NULL));CHECK(EmptyClipboard());HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,32);CHECK(memory);WCHAR *clip=GlobalLock(memory);wcscpy(clip,L"くちづけ");GlobalUnlock(memory);CHECK(SetClipboardData(CF_UNICODETEXT,memory));CloseClipboard();
    text_entry_model.anchor=0;text_entry_model.cursor=6;text_entry_paste(NULL);CHECK(!wcscmp(text_entry_model.value,L"くちづけ"));
    BYTE keyboard[256]={0};keyboard[VK_CONTROL]=0x80;CHECK(SetKeyboardState(keyboard));
    text_entry_message(NULL,WM_KEYDOWN,'Z',0);CHECK(!wcscmp(text_entry_model.value,L"before"));
    text_entry_message(NULL,WM_KEYDOWN,'Y',0);CHECK(!wcscmp(text_entry_model.value,L"くちづけ"));
    text_entry_message(NULL,WM_KEYDOWN,'Z',0);keyboard[VK_SHIFT]=0x80;CHECK(SetKeyboardState(keyboard));
    text_entry_message(NULL,WM_KEYDOWN,'Z',0);CHECK(!wcscmp(text_entry_model.value,L"くちづけ"));
    keyboard[VK_SHIFT]=0;CHECK(SetKeyboardState(keyboard));text_entry_message(NULL,WM_KEYDOWN,'A',0);
    memset(keyboard,0,sizeof(keyboard));CHECK(SetKeyboardState(keyboard));
    text_entry_message(NULL,WM_KEYDOWN,'K',0);text_entry_message(NULL,WM_CHAR,'k',0);CHECK(!wcscmp(text_entry_model.value,L"k"));
    keyboard[VK_CONTROL]=0x80;CHECK(SetKeyboardState(keyboard));text_entry_message(NULL,WM_KEYDOWN,'Z',0);CHECK(!wcscmp(text_entry_model.value,L"くちづけ"));
    memset(keyboard,0,sizeof(keyboard));CHECK(SetKeyboardState(keyboard));
    text_entry_message(NULL,WM_KEYDOWN,VK_ESCAPE,0);text_entry_overlay_tick(dialog);save_prompt_tick(owner);CHECK(results==2&&pushes==4&&pops==4);
    CHECK(text_entry_begin(owner,L"PRIMARY SERVER",L"https://official-na-east.reriff.online",done,server_text_valid));
    text_entry_blink_ms=500;text_entry_blink_at=1000;
    text_entry_caption_at(dialog,1000);CHECK(!wcscmp(drawn,L"https://official-na-east.reriff.online|"));
    text_entry_caption_at(dialog,1499);CHECK(wcschr(drawn,L'|'));
    text_entry_caption_at(dialog,1500);CHECK(!wcschr(drawn,L'|')&&!wcsstr(drawn,L"…")&&wcsstr(drawn,L"https://official-na-east.reriff.online"));
    text_entry_caption_at(dialog,2000);CHECK(wcschr(drawn,L'|'));
    text_entry_message(NULL,WM_KEYDOWN,VK_HOME,0);text_entry_caption_at(dialog,text_entry_blink_at);CHECK(!wcscmp(drawn,L"|https://official-na-east.reriff.online"));
    text_entry_blink_ms=INFINITE;text_entry_caption_at(dialog,text_entry_blink_at+50000);CHECK(wcschr(drawn,L'|'));
    text_model_init(&text_entry_model,L"https://example.com/abcdefghijklmnopqrstuvwxyz/abcdefghijklmnopqrstuvwxyz",2047);
    text_entry_caption_at(dialog,text_entry_blink_at);CHECK(drawn[0]==0x2026&&drawn[wcslen(drawn)-1]==L'|'&&wcslen(drawn)<=50);
    text_entry_message(NULL,WM_KEYDOWN,VK_ESCAPE,0);text_entry_overlay_tick(dialog);save_prompt_tick(owner);CHECK(results==2);
    puts("PASS: native overlay ownership, Unicode typing and clipboard, deferred accept/cancel, URL validation, duplicate refusal, Ctrl+Z/Y/Shift+Z and existing confirmation lifecycle");return 0;
}
