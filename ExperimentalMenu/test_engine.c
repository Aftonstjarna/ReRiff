#include "engine_menu.c"

#define CHECK(c) do { if (!(c)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); return 1; } } while (0)
static BYTE menu[512],items[11][512],texts[12][1024];
static void *children[11][1];
static void *vtable[27];
static int delegated, initialized, cleaned;
static int ticked, enable_calls;
static BOOL friends_enabled;
static void __thiscall mock_tick(void *self) { (void)self; ticked++; friends_enabled=FALSE; }
static void __thiscall mock_enable(void *self,BYTE enabled) {
    if (self==items[SERVERS_ID]) { friends_enabled=enabled; enable_calls++; }
}
static void __thiscall mock_colour(void *self,int id,const float *colour) {
    (void)self; (void)id; (void)colour;
}

static void __thiscall mock_raw(void *text,const WCHAR *value) {
    wcsncpy((WCHAR *)((BYTE *)text+512),value,250);
    ((float *)text)[12]=(float)wcslen(value)*8;
}
static void __thiscall mock_key(void *text,const char *value) {
    MultiByteToWideChar(CP_UTF8,0,value,-1,(WCHAR *)((BYTE *)text+512),250);
}
static const char *__thiscall mock_title(void *self) {
    (void)self; return "original-title";
}
static void __thiscall mock_input(void *self,int selection) {
    (void)self; delegated=selection;
}
static void __thiscall mock_init(void *self,void *screen,void *lower) {
    (void)self; (void)screen; (void)lower; initialized++;
}
static void __thiscall mock_cleanup(void *self,void *screen,void *lower) {
    (void)self; (void)screen; (void)lower; cleaned++;
}
static const WCHAR *label(int id) { return (const WCHAR *)(texts[id]+512); }
/* Model the game's runtime vtable dispatch with matching x86 thiscall types. */
static InputFn volatile input_entry=&hook_input;
static ScreenFn volatile init_entry=&hook_init;
static ScreenFn volatile cleanup_entry=&hook_cleanup;
static TickFn volatile tick_entry=&hook_tick;
static void select_item(int id) { input_entry(menu,id); }

int main(int argc,char **argv) {
    CHECK(argc==2);
    WCHAR game_path[MAX_PATH]; MultiByteToWideChar(CP_UTF8,0,argv[1],-1,game_path,MAX_PATH);
    CHECK(supported_file(game_path));
    WCHAR own[MAX_PATH]; GetModuleFileNameW(NULL,own,MAX_PATH);
    CHECK(!supported_file(own));
    /* The installed original is read-only: no original methods execute here. */
    CHECK(!ReRiffStartEngineMenu());
    CHECK(InterlockedExchange(&installed,0)==1);
    HMODULE image=LoadLibraryExW(game_path,NULL,DONT_RESOLVE_DLL_REFERENCES);
    CHECK(image);
    /* LoadLibraryEx does not relocate EXE references. Validate this read-only
     * view against its recorded preferred addresses, without executing code.
     * The production guard always requires the actual process base instead. */
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)image;
    IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)((BYTE *)image+dos->e_lfanew);
    CHECK(validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));
    puts("PASS: hash and original image view"); fflush(stdout);
    void **actual=(void **)((BYTE *)image+0x7a7470); DWORD protection,ignored;
    CHECK(VirtualProtect(actual,108,PAGE_READWRITE,&protection));
    void *saved=actual[24]; actual[24]=NULL;
    CHECK(!validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase)); actual[24]=saved;
    VirtualProtect(actual,108,protection,&ignored); FreeLibrary(image);
    puts("PASS: altered-vtable refusal"); fflush(stdout);
    CHECK(GetTempPathW(MAX_PATH,config_path));
    wcscat(config_path,L"reriff-engine-settings-test.ini"); DeleteFileW(config_path);
    wcscpy(log_path,config_path); wcscat(log_path,L".log");
    CHECK(WritePrivateProfileStringW(L"server",L"external_url",L"https://example.com",config_path));
    CHECK(WritePrivateProfileStringW(L"keep",L"key",L"value",config_path));
    static const char *labels[11]={"zero","one","two","three","four","five","six","seven","eight","nine","ten"};
    original_labels=labels;
    raw_text=&mock_raw; key_text=&mock_key;
    original_init=&mock_init; original_cleanup=&mock_cleanup;
    original_input=&mock_input;
    original_tick=&mock_tick; enable_input=&mock_enable;
    *(void ***)menu=vtable; vtable[13]=(void *)&mock_title;
    vtable[26]=(void *)&mock_colour;
    ((void **)menu)[36]=texts[11];
    for (int i=1;i<=10;i++) {
        ((void **)menu)[62+i]=items[i]; children[i][0]=texts[i];
        *(void ***)(items[i]+376)=children[i]; *(void ***)(items[i]+380)=children[i]+1;
        ((float *)items[i])[13]=30;
    }
    init_entry(menu,NULL,NULL);
    puts("PASS: original init callback returned"); fflush(stdout);
    CHECK(initialized==1 && !wcscmp(label(8),L"Servers") && !wcscmp(label(4),L"four"));
    CHECK(friends_enabled);
    tick_entry(menu); CHECK(ticked==1 && friends_enabled);
    ((void **)menu)[70]=NULL;
    int before=enable_calls; tick_entry(menu); CHECK(enable_calls==before);
    ((void **)menu)[70]=items[8];
    select_item(4); CHECK(delegated==4 && !server_page);
    select_item(9); CHECK(delegated==9);
    select_item(8); CHECK(server_page && !wcscmp(label(2),L"Use local server"));
    select_item(2); CHECK(!pending.only && !wcscmp(pending.fallback,L"http://127.0.0.1:5144"));
    select_item(3); CHECK(pending.only && !wcscmp(pending.external,L"https://example.com"));
    select_item(4); CHECK(!pending.only);
    select_item(6);
    WCHAR value[2048]; GetPrivateProfileStringW(L"keep",L"key",L"",value,2048,config_path);
    CHECK(!wcscmp(value,L"value"));
    select_item(5); CHECK(!pending.enabled && !wcscmp(pending.external,L"https://example.com")
        && !wcscmp(pending.fallback,L"http://127.0.0.1:5144"));
    select_item(7); CHECK(!wcscmp(pending.external,L"https://example.com"));
    select_item(2000); CHECK(!server_page && !wcscmp(label(2),L"two")
        && !wcscmp(label(8),L"Servers") && !wcscmp(label(4),L"four"));
    CHECK(delegated==9);
    select_item(8); select_item(5); select_item(10); CHECK(!server_page);
    load_settings(); CHECK(!pending.enabled && !wcscmp(pending.external,L"https://example.com")
        && !wcscmp(pending.fallback,L"http://127.0.0.1:5144"));
    select_item(8); select_item(5); select_item(10); load_settings(); CHECK(pending.enabled);
    cleanup_entry(menu,NULL,NULL); CHECK(cleaned==1 && !page_owner);
    DeleteFileW(config_path); DeleteFileW(log_path);
    puts("PASS: original hash/image guards, foreign-host refusal, corrupted-vtable refusal, x86 callback ABI, engine text routing, navigation, presets, save/reload, cleanup");
    return 0;
}
