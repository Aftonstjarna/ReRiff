#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static int forwarded;
static LRESULT CALLBACK previous(HWND w,UINT m,WPARAM k,LPARAM l){(void)w;(void)m;(void)k;(void)l;forwarded++;return 42;}
int main(int argc,char **argv){
    CHECK(argc==2);WCHAR path[MAX_PATH];CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,path,MAX_PATH));CHECK(supported_file(path));
    BYTE *base=(BYTE *)LoadLibraryExW(path,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(base);CHECK(install_text_entry_native(base));
    WNDPROC retained=(WNDPROC)(base+0x8c6e0);
    CHECK(text_entry_begin_inline(L""));CHECK(retained(NULL,WM_CHAR,L'つ',0)==0);CHECK(retained(NULL,WM_CHAR,L'き',0)==0);
    CHECK(!wcscmp(text_entry_model.value,L"つき"));
    CHECK(retained(NULL,WM_UNICHAR,UNICODE_NOCHAR,0)==TRUE&&!wcscmp(text_entry_model.value,L"つき"));
    CHECK(retained(NULL,WM_UNICHAR,0x1f3b5,0)==0&&!wcscmp(text_entry_model.value,L"つき\xd83c\xdfb5"));
    CHECK(retained(NULL,WM_UNICHAR,0xd800,0)==0&&wcslen(text_entry_model.value)==4);
CHECK(retained(NULL,WM_KEYDOWN,VK_ESCAPE,0)==0&&text_entry_action==-1);
    text_entry_end_inline();text_entry_native_previous=previous;CHECK(retained(NULL,WM_CHAR,L'x',0)==42&&forwarded==1);
    CHECK(!install_text_entry_native(base));
    puts("PASS: original-hash/prologue guard, direct retained WNDPROC Unicode input, cancel and inactive forwarding");return 0;
}
