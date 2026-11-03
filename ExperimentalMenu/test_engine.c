#include "engine_menu.c"
#define CHECK(c) do { if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;} }while(0)
static BYTE menu[512],group[512],audio[512],screen[700],sample[1024],title[1024],rows[256][1024],sprites[64][512],vanilla_rows[3][1024],vanilla_arrows[4][512],vanilla_lines[3][512];
static void *vtable[27],*item_methods[49];
static void **item_vtable=item_methods+1;
static int delegated,initialized,cleaned,ticked,base_ticked,focused,created,added;
static int restart_calls,removed,sprites_created,colours;
static void *colour_watch;static float watched_colour[4];
static void *fake_ui=screen;
static BOOL restart_ok;
static BOOL testing_graphics_reel;
static int __thiscall mock_main_hit(void *row,float *point){return point[0]>=((float *)row)[10]&&point[0]<=((float *)row)[10]+((float *)row)[12]?((int *)row)[2]:-1;}
static EnableFn native_enable;
static void __thiscall mock_tick(void *self){(void)self;ticked++;}
static void __thiscall mock_base_tick(void *self);
static int pointer_hover=-1;static BOOL accept_press;
static int native_directions,direction_polls;
static void __thiscall mock_native_direction(void *row,DWORD key,unsigned device){
    if(device!=0||((int *)row)[38]!=1)abort();
    int id=((int *)row)[key==64?39:40];
    if(id<1501||id>1511)abort();
    void *target=server_rows[id-1500];
    if(!(*((BYTE *)target+372)&1)||!(*(DWORD *)((BYTE *)target+76)&1))abort();
    native_directions++;focused=id;((int *)menu)[11]=id;
}
static void __thiscall mock_enable(void *self,BYTE enabled){
    if(enabled)native_enable(self,TRUE);
    else *(DWORD *)((BYTE *)self+76)=0;
}
static void __thiscall mock_visible(void *self,BYTE visible){
    *((BYTE *)self+372)=(*((BYTE *)self+372)&~1)|!!visible;
}
static void __thiscall mock_colour(void *self,const float *value){if(self==colour_watch&&value)memcpy(watched_colour,value,sizeof(watched_colour));colours++;}
static void __thiscall mock_menu_colour(void *self,int id,const float *value){(void)self;(void)id;(void)value;}
static void __thiscall mock_scale(void *self,const float *value){((float *)self)[43]=value[0];((float *)self)[44]=value[1];}
static void __thiscall mock_def(void *self,unsigned id){((unsigned *)self)[100]=id;}
static void __thiscall mock_origin(void *self,unsigned id){((unsigned *)self)[101]=id;}
static void __thiscall mock_position(void *self,const float *pos){((float *)self)[8]=pos[0];((float *)self)[9]=pos[1];}
static void *__thiscall mock_sprite(void *ui,int id,const float *pos,int origin,int item_id){
    (void)ui;if(sprites_created>=64)abort();void *sprite=sprites[sprites_created++];*(void ***)sprite=item_vtable;
    ((int *)sprite)[2]=item_id;((float *)sprite)[12]=300;((float *)sprite)[13]=1;mock_def(sprite,id);mock_origin(sprite,origin);mock_position(sprite,pos);return sprite;
}
static void *__thiscall mock_button_sprite(void *ui,int sprite,int id,DWORD mask,const float *point,int origin){
    void *button=mock_sprite(ui,sprite,point,origin,id);*(DWORD *)((BYTE *)button+64)=mask;return button;
}
static void __thiscall mock_graphics_input(void *menu,int selection){
    delegated=selection;if(selection==4)*((BYTE *)menu+305)=!*((BYTE *)menu+305);
}
static void *__thiscall mock_blank(void *ui,int id,DWORD shortcut,const float *pos,const float *size){
    void *button=mock_sprite(ui,0,pos,4,id);((float *)button)[12]=size[0];((float *)button)[13]=size[1];
    *(DWORD *)((BYTE *)button+64)=shortcut;return button;
}
static void __thiscall mock_raw(void *text,const WCHAR *value){wcsncpy((WCHAR *)((BYTE *)text+512),value,250);((float *)text)[12]=(float)wcslen(value)*8;}
static void __thiscall mock_key(void *text,const char *value){MultiByteToWideChar(CP_UTF8,0,value,-1,(WCHAR *)((BYTE *)text+512),250);}
static const char *__thiscall mock_title(void *self){(void)self;return "settings-title";}
static void __thiscall mock_input(void *self,int selection){
    /* Exact guard in gaOptionsMenu.handleScreenInput, verified original IL. */
    if(((int *)self)[102]||*((BYTE *)self+392)||*((BYTE *)self+393))return;
    delegated=selection;
}
static void __thiscall mock_init(void *self,void *up,void *low){(void)self;(void)up;(void)low;initialized++;}
static void __thiscall mock_cleanup(void *self,void *up,void *low){(void)self;(void)up;(void)low;cleaned++;}
static void __thiscall mock_focus(void *self,int id){(void)self;focused=id;}
static void * __cdecl mock_create(const WCHAR *text,float x,float y,unsigned font,int h,int v,float size,BOOL wrap,float spacing,BOOL a,BOOL b){
    (void)font;(void)h;(void)v;(void)size;(void)wrap;(void)spacing;(void)a;(void)b;
    if(created>=256)return NULL;
    BYTE *row=rows[created++];*(void ***)row=item_vtable;
    ((float *)row)[8]=x;((float *)row)[9]=y;mock_raw(row,text);return row;
}
static void __thiscall mock_add(void *parent,void *row){
    (void)parent;
    /* Native add consumes one temporary reference and retains one in its vector. */
    if(((int *)row)[1]!=1)abort();added++;
}
static void __thiscall mock_remove(void *parent,void *row){
    (void)parent;if(((int *)row)[1]!=2)abort();((int *)row)[1]-=2;removed++;
}
static const WCHAR *label(int action){return (const WCHAR *)((BYTE *)server_rows[action]+512);}
static BOOL mock_restart(void){
    WCHAR value[32];GetPrivateProfileStringW(L"launcher",L"skip_launcher",L"",value,32,config_path);
    if(wcscmp(value,pending.skip_launcher?L"true":L"false"))return FALSE;
    restart_calls++;return restart_ok;
}
static InputFn volatile input_entry=hook_input;
static ScreenFn volatile init_entry=hook_init,cleanup_entry=hook_cleanup;
static TickFn volatile tick_entry=hook_tick;
static void __thiscall mock_base_tick(void *self){
    base_ticked++;
    if(pointer_hover>=0){
        for(int i=0;i<root_count;i++){
            void *row=root_rows[i].text;
            if(root_rows[i].id==pointer_hover&&(*((BYTE *)row+372)&1)&&(*(DWORD *)((BYTE *)row+76)&1)){
                focused=pointer_hover;((int *)self)[11]=focused;
            }
        }
    }
    if(accept_press){accept_press=FALSE;input_entry(self,focused);}
}
static void select_action(int action){logical_selection=server_action_index(action);scroll_to(menu,clamp_index(logical_selection,0,SERVER_ACTION_COUNT-visible_rows),FALSE);apply_pending_focus(menu);input_entry(menu,SERVER_ACTION_BASE+action);}
static void open_servers(void){if(((int *)menu)[62]==0&&!server_page)scroll_to(menu,0,FALSE);input_entry(menu,SERVERS_ID);}
typedef struct FakeReel {BYTE object[532];void *children[16];int count,maximum,selection;} FakeReel;
static void *fake_reel_table[37];static int constructed,destroyed,bars,ticks,forwarded;
static DWORD held_input;
static int aa_attach_calls;
static BOOL mock_aa_attach(void){aa_attach_calls++;return TRUE;}
static BYTE graphics_test_menu[512],graphics_test_items[7][1024];
static void __thiscall mock_graphics_menu_item(void *item,BYTE enabled){
    BYTE *flags=(BYTE *)item+372;*flags=(*flags&~2)|(enabled?2:0);
}
static BOOL graphics_cycle(void **items,int count,int start,int field,const int *expected,int steps){
    int id=start;
    for(int step=0;step<steps;step++){
        void *item=NULL;
        for(int i=0;i<count;i++)if(((int *)items[i])[2]==id)item=items[i];
        if(!item||((int *)item)[38]!=1)return FALSE;
        id=((int *)item)[field];if(id!=expected[step])return FALSE;
    }
    return TRUE;
}
static BOOL notice_accept;static int notices;static WCHAR notice_text[128];
static BOOL mock_notice(const WCHAR *text){notices++;wcsncpy(notice_text,text,127);return notice_accept;}
static float * __cdecl mock_resolution(float *out,int screen_id){if(screen_id)abort();out[0]=1920;out[1]=1080;return out;}
static DWORD mock_direction(void){direction_polls++;return held_input;}
static void *__cdecl mock_allocate(size_t size){if(size!=532)abort();return calloc(1,sizeof(FakeReel));}
static void *__cdecl mock_reel_ctor(void *self,void *parent,float spacing,int direction){
    (void)parent;if(spacing!=(testing_graphics_reel?graphics_gap:row_gap)||direction!=1)abort();*(void ***)self=fake_reel_table;constructed++;return self;
}
static void *__thiscall mock_reel_delete(void *self,unsigned flag){
    FakeReel *r=self;if(flag!=1||((int *)self)[1]!=0)abort();
    for(int i=0;i<r->count;i++){if(((int *)r->children[i])[1]!=1)abort();((int *)r->children[i])[1]--;}
    free(self);destroyed++;return NULL;
}
static void __cdecl mock_reel_add(void *self,void *row,void *screen_arg){
    FakeReel *r=self;if(screen_arg||r->count>=16||((int *)row)[1]!=1)abort();r->children[r->count++]=row;
}
static void __cdecl mock_reel_limit(void *self,int visible){FakeReel *r=self;r->maximum=r->count-visible;}
static void __cdecl mock_reel_tick(void *self){(void)self;ticks++;}
static void __cdecl mock_reel_move(void *self,int first,float *time){
    FakeReel *r=self;if(!time||*time!=0)abort();r->selection=clamp_index(first,0,r->maximum);
}
static int __cdecl mock_reel_selection(void *self){return ((FakeReel *)self)->selection;}
static void __cdecl mock_reel_bar(void *self,float x,float top,float bottom,int pc,float *width){
    (void)self;if(!(x>0&&bottom>top)||pc!=1||width||(!testing_graphics_reel&&(top+bottom<ui_height-0.01f||top+bottom>ui_height+0.01f)))abort();bars++;
}
static void __thiscall mock_menu_focus(void *self,int id){
    ((int *)self)[11]=id;static const float c[4]={0,1,1,0.8f};
    if(server_page&&id>1500&&id<=1511)mock_colour(server_rows[id-1500],c);
    else for(int i=0;i<root_count;i++)if(root_rows[i].id==id)mock_colour(root_rows[i].text,c);
}
static LRESULT CALLBACK mock_window(HWND w,UINT message,WPARAM p,LPARAM l){(void)w;(void)message;(void)p;(void)l;forwarded++;return 0;}
static BOOL exit_ok;static int exit_calls;
static BOOL mock_exit(void){exit_calls++;return exit_ok;}
int main(int argc,char **argv){
    CHECK(argc==2);WCHAR game[MAX_PATH],own[MAX_PATH];MultiByteToWideChar(CP_UTF8,0,argv[1],-1,game,MAX_PATH);
    CHECK(supported_file(game));GetModuleFileNameW(NULL,own,MAX_PATH);CHECK(!supported_file(own));
    CHECK(!ReRiffStartEngineMenu());CHECK(InterlockedExchange(&installed,0)==1);
    HMODULE image=LoadLibraryExW(game,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(image);
    IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)((BYTE *)image+((IMAGE_DOS_HEADER *)image)->e_lfanew);
    CHECK(validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));
    CHECK(validate_display_modes((BYTE *)image,nt->OptionalHeader.ImageBase));
    /* Reproduce Windows loading the supported executable at 0x00C30000. */
    DWORD image_size=nt->OptionalHeader.SizeOfImage;BYTE *rebased=VirtualAlloc(NULL,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(rebased);
    for(DWORD cursor=0;cursor<image_size;){
        MEMORY_BASIC_INFORMATION info;CHECK(VirtualQuery((BYTE *)image+cursor,&info,sizeof(info)));
        size_t count=(BYTE *)info.BaseAddress+info.RegionSize-((BYTE *)image+cursor);if(count>image_size-cursor)count=image_size-cursor;
        if(info.State==MEM_COMMIT&&!(info.Protect&(PAGE_GUARD|PAGE_NOACCESS)))memcpy(rebased+cursor,(BYTE *)image+cursor,count);
        cursor+=(DWORD)count;
    }
    const DWORD relocated_base=0xc30000,delta=relocated_base-nt->OptionalHeader.ImageBase;
    IMAGE_DATA_DIRECTORY reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    for(DWORD cursor=0;cursor<reloc.Size;){
        IMAGE_BASE_RELOCATION *block=(IMAGE_BASE_RELOCATION *)(rebased+reloc.VirtualAddress+cursor);CHECK(block->SizeOfBlock>=8);
        WORD *fixups=(WORD *)(block+1);
        for(DWORD j=0;j<(block->SizeOfBlock-8)/2;j++)if((fixups[j]>>12)==IMAGE_REL_BASED_HIGHLOW)*(DWORD *)(rebased+block->VirtualAddress+(fixups[j]&4095))+=delta;
        cursor+=block->SizeOfBlock;
    }
    CHECK(validate_image_at(rebased,relocated_base));CHECK(validate_display_modes(rebased,relocated_base));CHECK(validate_aa_backend(rebased,relocated_base));
    /* Windows may discard .reloc after fixups. Retain exact guards using the
     * relocation records read from the same SHA-256-verified original file. */
    DWORD reloc_protection,reloc_ignored;
    CHECK(verified_relocations&&verified_relocations_size==reloc.Size);
    CHECK(VirtualProtect(rebased+reloc.VirtualAddress,reloc.Size,PAGE_NOACCESS,&reloc_protection));
    IMAGE_NT_HEADERS32 *live_nt=(IMAGE_NT_HEADERS32 *)(rebased+((IMAGE_DOS_HEADER *)rebased)->e_lfanew);
    DWORD file_base=live_nt->OptionalHeader.ImageBase;
    IMAGE_DATA_DIRECTORY saved_directory=live_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    live_nt->OptionalHeader.ImageBase=relocated_base;
    live_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress=0;
    live_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size=0;
    CHECK(validate_image_at(rebased,relocated_base));
    rebased[0x50af6]^=1;CHECK(!validate_image_at(rebased,relocated_base));rebased[0x50af6]^=1;
    live_nt->OptionalHeader.ImageBase=file_base;
    live_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]=saved_directory;
    CHECK(VirtualProtect(rebased+reloc.VirtualAddress,reloc.Size,reloc_protection,&reloc_ignored));
    rebased[0x4b530]^=1;CHECK(!validate_image_at(rebased,relocated_base));rebased[0x4b530]^=1;
    rebased[0x48834]^=1;CHECK(!validate_image_at(rebased,relocated_base));rebased[0x48834]^=1;
    CHECK(validate_image_at(rebased,relocated_base));CHECK(VirtualFree(rebased,0,MEM_RELEASE));

    CHECK(*(DWORD *)((BYTE *)image+0x5fa2e0)==nt->OptionalHeader.ImageBase+0x48e70);
    void **actual=(void **)((BYTE *)image+0x7a5a7c);DWORD protection,ignored;
    CHECK(VirtualProtect(actual,108,PAGE_READWRITE,&protection));void *saved=actual[24];actual[24]=NULL;
    CHECK(!validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));actual[24]=saved;VirtualProtect(actual,108,protection,&ignored);
    native_enable=(EnableFn)((BYTE *)image+0x4b4a0);
    CHECK(GetTempPathW(MAX_PATH,config_path));wcscat(config_path,L"reriff-native-reel-test.ini");DeleteFileW(config_path);
    wcscpy(log_path,config_path);wcscat(log_path,L".log");
    CHECK(WritePrivateProfileStringW(L"server",L"external_url",L"https://example.com",config_path));
    CHECK(WritePrivateProfileStringW(L"keep",L"key",L"value",config_path));
    CHECK(WritePrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"true",config_path));
    original_init=mock_init;original_cleanup=mock_cleanup;original_input=mock_input;original_tick=mock_tick;base_tick=mock_base_tick;
    enable_input=mock_enable;raw_text=mock_raw;key_text=mock_key;create_text=mock_create;menu_item=mock_visible;
    add_item=mock_add;add_screen=mock_add;remove_screen=mock_remove;set_focus=mock_focus;
    create_sprite=mock_sprite;create_blank_button=mock_blank;ui_instance=&fake_ui;sprite_def=mock_def;sprite_origin=mock_origin;sprite_position=mock_position;original_colour=mock_menu_colour;
    game_allocate=mock_allocate;reel_ctor=mock_reel_ctor;reel_add=mock_reel_add;reel_limit=mock_reel_limit;
    reel_tick=mock_reel_tick;reel_move=mock_reel_move;reel_selection=mock_reel_selection;reel_bar=mock_reel_bar;
    fake_reel_table[0]=(void *)mock_reel_delete;reel_vtable=fake_reel_table;direction_input=mock_direction;
    item_vtable[9]=(void *)mock_scale;item_vtable[3]=(void *)mock_visible;item_vtable[4]=(void *)mock_colour;
    item_vtable[28]=(void *)mock_native_direction;
    *(void ***)menu=vtable;vtable[13]=(void *)mock_title;vtable[25]=(void *)mock_menu_focus;
    *(void ***)group=item_vtable;*(void ***)audio=item_vtable;((void **)menu)[36]=title;((void **)menu)[46]=screen;
    ((void **)menu)[67]=group;((void **)menu)[83]=audio;
    ((float *)menu)[14]=0.5f;((float *)menu)[15]=0.5f;((float *)menu)[66]=211;
    for(int i=0;i<4;i++){
        BYTE *row=i?vanilla_rows[i-1]:sample;*(void ***)row=item_vtable;((void **)menu)[68+i]=row;
        ((float *)row)[8]=960;((float *)row)[9]=324+i*211;((float *)row)[12]=1344;((float *)row)[13]=106;
        row[372]=1;*(DWORD *)(row+76)=0xffffffffu;
        *(void ***)vanilla_arrows[i]=item_vtable;((float *)vanilla_arrows[i])[8]=1372.5f;((float *)vanilla_arrows[i])[9]=354+i*211;
        ((void **)menu)[73+i]=vanilla_arrows[i];vanilla_arrows[i][372]=1;
        if(i<3){*(void ***)vanilla_lines[i]=item_vtable;((void **)menu)[78+i]=vanilla_lines[i];vanilla_lines[i][372]=1;}
    }
    *(DWORD *)(screen+40)=1;group[372]=1;init_entry(menu,NULL,NULL);CHECK(initialized==1&&visible_rows==4&&root_count==5&&added==28);
    CHECK(root_rows[0].id==SERVERS_ID&&(*((BYTE *)server_rows[0]+372)&1));
    CHECK(root_rows[1].id==0&&((float *)root_rows[1].text)[9]-((float *)server_rows[0])[9]==row_gap);
    tick_entry(menu);CHECK(native_reel&&bars==1&&focused==SERVERS_ID&&logical_selection==0);
    /* Reproduce gaMenu.enter's original recursive input-mask reset AFTER
     * initScreens. Exercise the real original native method, not a mask mock. */
    void *children[24];int child_count=0;
    for(int i=0;i<root_count;i++){children[child_count++]=root_rows[i].text;if(root_rows[i].arrow)children[child_count++]=root_rows[i].arrow;if(root_rows[i].line)children[child_count++]=root_rows[i].line;}
    ((void **)group)[94]=children;((void **)group)[95]=children+child_count;((void **)screen)[3]=group;
    ((void (__thiscall *)(void *,DWORD))((BYTE *)image+0x8f4e0))(screen,1);
    CHECK(*(DWORD *)((BYTE *)root_rows[4].text+76)==1); /* hidden native row reenabled */
    prepare_scroll(menu);CHECK(*(DWORD *)((BYTE *)root_rows[4].text+76)==0);
    CHECK(*(DWORD *)((BYTE *)root_rows[0].arrow+76)==0);
    *(DWORD *)((BYTE *)root_rows[1].text+356)=0x24681357;
    tick_entry(menu);CHECK(*(DWORD *)((BYTE *)root_rows[1].text+356)==0x24681357);
    /* Cover all original actions with hover+click and focused controller A,
     * including stale Audio latches and a scroll to reach Credits. */
    for(int i=1;i<root_count;i++){
        logical_selection=i;scroll_to(menu,i>=visible_rows?i-visible_rows+1:0,FALSE);apply_pending_focus(menu);
        pointer_hover=root_rows[i].id;delegated=-1;accept_press=TRUE;menu[392]=menu[393]=1;
        tick_entry(menu);CHECK(focused==root_rows[i].id&&delegated==root_rows[i].id&&!menu[392]&&!menu[393]);
        pointer_hover=-1;delegated=-1;accept_press=TRUE;menu[392]=1;
        tick_entry(menu);CHECK(delegated==root_rows[i].id&&!menu[392]);
    }
    logical_selection=0;scroll_to(menu,0,FALSE);apply_pending_focus(menu);
    CHECK(navigate_direction(menu,128,100));apply_pending_focus(menu);CHECK(logical_selection==1&&focused==0);
    CHECK(navigate_direction(menu,128,101));CHECK(logical_selection==1);
    CHECK(navigate_direction(menu,128,350));apply_pending_focus(menu);CHECK(logical_selection==2&&focused==1);
    CHECK(navigate_direction(menu,192,351));CHECK(logical_selection==2);
    CHECK(!navigate_direction(menu,0,352));
    held_input=128;last_direction=0;int native_before=base_ticked;tick_entry(menu);CHECK(base_ticked==native_before);held_input=0;
    open_servers();CHECK(server_page&&focused==1501&&native_reel&&bars==2&&destroyed==1);
    CHECK(server_native_navigation&&(*(void ***)server_rows[1])[28]==(void *)server_scroll_direction);
    void *saved_server_rows[SERVER_ACTION_COUNT];for(int i=0;i<SERVER_ACTION_COUNT;i++)saved_server_rows[i]=server_rows[server_action_order[i]];
    CHECK(!server_rows[2]&&!server_rows[3]&&server_rows[11]);
    /* Three full cycles each way through the original handler. The scroll
     * adapter must expose even the wrapped target before native focus checks. */
    for(int pass=0;pass<2;pass++)for(int step=0;step<3*SERVER_ACTION_COUNT;step++){
        int old=server_action_index(focused-SERVER_ACTION_BASE),key=pass?64:128,next=(old+(pass?SERVER_ACTION_COUNT-1:1))%SERVER_ACTION_COUNT;
        ((DirectionFn)(*(void ***)server_rows[server_action_order[old]])[28])(server_rows[server_action_order[old]],key,0);
        CHECK(focused==SERVER_ACTION_BASE+server_action_order[next]&&logical_selection==next&&pending_focus==-1);
        CHECK(next>=page_first&&next<page_first+visible_rows);
    }
    CHECK(focused==1501&&native_directions==6*SERVER_ACTION_COUNT&&page_first==0);
    int polls_before=direction_polls;native_before=base_ticked;held_input=128;
    tick_entry(menu);held_input=0;CHECK(direction_polls==polls_before&&base_ticked==native_before+1);
    CHECK(!wcscmp(label(5),L"SINGLEPLAYER")&&pending.enabled&&((unsigned *)toggles[2][1])[100]==75);
    WCHAR saved_primary[4096],saved_secondary[4096];
    wcscpy(saved_primary,pending.external);wcscpy(saved_secondary,pending.fallback);
    select_action(5);CHECK(!pending.enabled&&!wcscmp(saved_primary,pending.external)&&!wcscmp(saved_secondary,pending.fallback));
    select_action(5);CHECK(pending.enabled);
    select_action(5);CHECK(!pending.enabled&&((unsigned *)toggles[2][1])[100]==74);
    select_action(6);select_action(7);CHECK(!pending.enabled&&((unsigned *)toggles[2][1])[100]==74);
    select_action(5);CHECK(pending.enabled&&((unsigned *)toggles[2][1])[100]==75);select_action(6);
    scroll_to(menu,0,FALSE);apply_pending_focus(menu);
    CHECK(!wcscmp(label(8),L"SAVE AND RESTART"));CHECK(((float *)server_rows[4])[9]-((float *)server_rows[1])[9]==row_gap);
    CHECK(!(*((BYTE *)server_rows[7]+372)&1)&&*(DWORD *)((BYTE *)server_rows[7]+76)==0);
    *(DWORD *)((BYTE *)server_rows[1]+356)=0x12345678;tick_entry(menu);CHECK(*(DWORD *)((BYTE *)server_rows[1]+356)==0x12345678);
    /* Native drag/tap movement is consumed from the original reel, with no
     * competing custom scrollbar buttons or native UI directional pass. */
    ((FakeReel *)native_reel)->selection=6;prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==5&&focused==1509);
    HWND window=CreateWindowExW(0,L"STATIC",L"Native reel input fixture",WS_OVERLAPPEDWINDOW,0,0,800,600,NULL,NULL,GetModuleHandleW(NULL),NULL);
    CHECK(window);scroll_previous=mock_window;
    scroll_wndproc(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(700,300));CHECK(forwarded==1);
    scroll_wndproc(window,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(700,400));CHECK(forwarded==2);
    scroll_wndproc(window,WM_LBUTTONUP,0,MAKELPARAM(700,400));CHECK(forwarded==3);
    scroll_wndproc(window,WM_KEYDOWN,VK_HOME,0);prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==0);
    scroll_wndproc(window,WM_MOUSEWHEEL,MAKEWPARAM(0,(WORD)-120),0);CHECK(page_first==0);prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==1);
    scroll_to(menu,0,FALSE);apply_pending_focus(menu);DestroyWindow(window);scroll_previous=NULL;
    dispatching_menu=TRUE;scroll_to(menu,1,TRUE);int previous_focus=focused;apply_pending_focus(menu);CHECK(focused==previous_focus&&pending_focus>=0);
    dispatching_menu=FALSE;apply_pending_focus(menu);CHECK(focused==1507);
    select_action(1);CHECK(pending.skip_launcher);select_action(6);select_action(1);select_action(7);CHECK(pending.skip_launcher);
    WCHAR value[32];GetPrivateProfileStringW(L"keep",L"key",L"",value,32,config_path);CHECK(!wcscmp(value,L"value"));
    GetPrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"",value,32,config_path);CHECK(!wcscmp(value,L"true"));
    select_action(9);CHECK(!server_page&&root_first==0&&focused==SERVERS_ID);
    ((int *)menu)[62]=1;group[372]=0;audio[372]=1;prepare_scroll(menu);CHECK(!native_reel&&!scroll_active);
    int audio_before=ticked;tick_entry(menu);CHECK(ticked==audio_before+1);
    ((int *)menu)[62]=0;group[372]=1;audio[372]=0;prepare_scroll(menu);CHECK(native_reel&&scroll_active);
    open_servers();request_restart=mock_restart;restart_ok=FALSE;select_action(8);CHECK(restart_calls==1&&!restarting);
    restart_ok=TRUE;select_action(8);CHECK(restart_calls==2&&restarting);restart_started=GetTickCount()-15001;tick_entry(menu);CHECK(!restarting);
    request_exit=mock_exit;exit_ok=FALSE;select_action(11);CHECK(exit_calls==1&&!exiting);
    exit_ok=TRUE;select_action(11);CHECK(exit_calls==2&&exiting);
    cleanup_entry(menu,NULL,NULL);CHECK(cleaned==1&&!page_owner&&!native_reel&&destroyed==constructed&&removed==24);
    CHECK(!server_native_navigation);
    for(int i=0;i<SERVER_ACTION_COUNT;i++)CHECK(*(void ***)saved_server_rows[i]==item_vtable);
    main_init_original=mock_init;main_cleanup_original=mock_cleanup;main_tick_original=mock_tick;native_resolution=mock_resolution;queue_update_notice=mock_notice;
    BYTE centered_menu[512]={0},centered_row[404]={0},centered_text[404]={0},centered_icon[404]={0};
    void *centered_children[2]={centered_text,centered_icon};
    *(void ***)centered_icon=item_vtable;
    *(void ***)centered_row=item_vtable;
    ((void **)centered_menu)[62]=centered_row;
    *(void ***)(centered_row+376)=centered_children;*(void ***)(centered_row+380)=centered_children+2;
    ((float *)centered_row)[9]=320;((float *)centered_row)[12]=330;((int *)centered_row)[2]=7;((float *)centered_menu)[73]=30;
    ((float *)centered_text)[12]=300;((float *)centered_text)[43]=1;
    ((float *)centered_icon)[8]=-16;((float *)centered_icon)[12]=48;((float *)centered_icon)[43]=1;((int *)centered_icon)[14]=5;
    item_vtable[25]=(void *)mock_main_hit;((float *)centered_row)[43]=1;((float *)centered_row)[10]=842;
    center_main_rows(centered_menu);
    float icon_point[2]={780,320};CHECK(((MainHitFn)(*(void ***)centered_row)[25])(centered_row,icon_point)==7);
    CHECK(((float *)centered_row)[10]==842&&((float *)centered_row)[12]==330&&((float *)centered_icon)[8]==-16);
    CHECK(((float *)centered_row)[8]==842&&((float *)centered_row)[9]==320&&((int *)centered_row)[2]==7);
    CHECK(((float *)centered_row)[12]==330&&((float *)centered_text)[8]==0);
    CHECK(((float *)centered_icon)[8]==-16&&((int *)centered_icon)[2]==0);
    center_main_rows(centered_menu);CHECK(((float *)centered_row)[8]==842&&((float *)centered_row)[12]==330);
    detach_main_hits();CHECK(*(void ***)centered_row==item_vtable);
    wcscpy(update_notice_path,config_path);wcscat(update_notice_path,L".update.ini");
    CHECK(WritePrivateProfileStringW(L"update",L"version",MENU_WIDE(RERIFF_VERSION),update_notice_path));
    notice_accept=FALSE;deliver_update_notice();CHECK(notices==1&&GetFileAttributesW(update_notice_path)!=INVALID_FILE_ATTRIBUTES);
    notice_accept=TRUE;branded_main_init(menu,NULL,NULL);
    CHECK(watermark&&watermark_owner==menu&&((int *)watermark)[2]==-1&&*(DWORD *)((BYTE *)watermark+76)==0);
    CHECK(!wcscmp((const WCHAR *)((BYTE *)watermark+512),L"ReRiff v" MENU_WIDE(RERIFF_VERSION)));
    CHECK(notices==2&&!wcscmp(notice_text,L"ReRiff has updated to " MENU_WIDE(RERIFF_VERSION))&&GetFileAttributesW(update_notice_path)==INVALID_FILE_ATTRIBUTES);
    deliver_update_notice();CHECK(notices==2);
    ((float *)menu)[73]=48;((float *)watermark)[13]=64;branded_main_tick(menu);
    CHECK(((float *)watermark)[43]==0.5f&&((float *)watermark)[44]==0.5f);
    CHECK(((float *)watermark)[8]==1908&&((float *)watermark)[9]==1036);
    ((float *)menu)[73]=24;branded_main_tick(menu);
    CHECK(((float *)watermark)[44]==0.25f&&((float *)watermark)[9]==1052);
    branded_main_cleanup(menu,NULL,NULL);CHECK(!watermark&&!watermark_owner&&removed==25);
    branded_main_tick(menu);CHECK(watermark&&notices==2);branded_main_cleanup(menu,NULL,NULL);CHECK(!watermark&&removed==26);
    CHECK(validate_graphics_aa_menu((BYTE *)image,nt->OptionalHeader.ImageBase));
    CHECK(validate_aa_backend((BYTE *)image,nt->OptionalHeader.ImageBase));
    CHECK(!validate_aa_backend((BYTE *)image,nt->OptionalHeader.ImageBase+0x18));
    static const BYTE backend_devices[]={0xc7,0x46,0x78,0,0,0,0,0xc7,0x46,0x7c,0,0,0,0};
    CHECK(!memcmp((BYTE *)image+0x9fdbc,backend_devices,sizeof(backend_devices)));
    void *gm=graphics_test_menu;*(void ***)gm=vtable;((void **)gm)[46]=screen;
    ((float *)gm)[14]=((float *)gm)[15]=0.5f;
    for(int i=0;i<7;i++){*(void ***)graphics_test_items[i]=item_vtable;((float *)graphics_test_items[i])[12]=300;((float *)graphics_test_items[i])[13]=64;graphics_test_items[i][372]=1;}
    for(int i=0;i<4;i++)((void **)gm)[62+i]=graphics_test_items[i];
    ((void **)gm)[66]=graphics_test_items[6];((void **)gm)[73]=graphics_test_items[4];
    void *resolution_text=graphics_test_items[5];((void **)gm)[70]=&resolution_text;((void **)gm)[71]=&resolution_text+1;
    ((float *)graphics_test_items[0])[8]=1370;((float *)graphics_test_items[0])[9]=580;
    ((float *)graphics_test_items[2])[9]=500;((float *)graphics_test_items[3])[9]=500;
    ((float *)graphics_test_items[2])[8]=550;((float *)graphics_test_items[3])[8]=1370;
    ((float *)graphics_test_items[4])[8]=550;((float *)graphics_test_items[4])[9]=580;((float *)graphics_test_items[5])[9]=500;
    ((int *)graphics_test_items[0])[2]=4;((int *)graphics_test_items[6])[2]=3;
    ((int *)graphics_test_items[2])[2]=1;((int *)graphics_test_items[3])[2]=2;
    for(int i=0;i<7;i++)for(int field=39;field<=42;field++)((int *)graphics_test_items[i])[field]=-1;
    ((int *)graphics_test_items[0])[38]=1;((int *)graphics_test_items[0])[39]=1;((int *)graphics_test_items[0])[40]=1;
    ((int *)graphics_test_items[2])[38]=((int *)graphics_test_items[3])[38]=1;
    ((int *)graphics_test_items[2])[39]=((int *)graphics_test_items[3])[39]=4;
    ((int *)graphics_test_items[2])[40]=((int *)graphics_test_items[3])[40]=4;
    graphics_init_original=mock_init;graphics_cleanup_original=mock_cleanup;graphics_tick_original=mock_tick;menu_item=mock_graphics_menu_item;graphics_input_original=mock_graphics_input;graphics_button_sprite=mock_button_sprite;graphics_colour_original=mock_menu_colour;
    attach_antialiasing=mock_aa_attach;aa_requested=aa_fault=0;aa_graphics_init(gm,NULL,NULL);
    CHECK((*((BYTE *)graphics_test_items[6]+372)&2)!=0);
    CHECK(aa_label&&aa_bar&&aa_knob&&!aa_pending&&((int *)aa_label)[2]==AA_CONTROL_ID);
    CHECK(resolution_hover&&((int *)resolution_hover)[2]==RESOLUTION_FOCUS_ID);
    CHECK(((float *)resolution_hover)[8]==960&&((float *)resolution_hover)[9]==420);
    CHECK(((float *)resolution_hover)[12]==520&&((float *)resolution_hover)[13]==56);
    /* Exercise multiple complete native row loops in both directions, including
     * both resolution arrows and escape from the mouse-focused Apply button. */
    void *nav_items[8]={graphics_test_items[2],graphics_test_items[3],borderless_label,borderless_bar,borderless_knob,aa_label,graphics_test_items[6],ui_slider_label};
    const int down_cycle[]={BORDERLESS_CONTROL_ID,AA_CONTROL_ID,UI_SCALE_CONTROL_ID,3,1,BORDERLESS_CONTROL_ID,AA_CONTROL_ID,UI_SCALE_CONTROL_ID,3,1};
    const int up_cycle[]={3,UI_SCALE_CONTROL_ID,AA_CONTROL_ID,BORDERLESS_CONTROL_ID,1,3,UI_SCALE_CONTROL_ID,AA_CONTROL_ID,BORDERLESS_CONTROL_ID,1};
    CHECK(graphics_cycle(nav_items,8,1,40,down_cycle,10));
    CHECK(graphics_cycle(nav_items,8,2,40,down_cycle,10));
    CHECK(graphics_cycle(nav_items,8,1,39,up_cycle,10));
    CHECK(graphics_cycle(nav_items,8,2,39,up_cycle,10));
    const int from_apply_down[]={1,BORDERLESS_CONTROL_ID,AA_CONTROL_ID,UI_SCALE_CONTROL_ID,3};
    const int from_apply_up[]={UI_SCALE_CONTROL_ID,AA_CONTROL_ID,BORDERLESS_CONTROL_ID,1,3};
    CHECK(graphics_cycle(nav_items,8,3,40,from_apply_down,5));
    CHECK(graphics_cycle(nav_items,8,3,39,from_apply_up,5));
    CHECK(((int *)borderless_bar)[2]==DISPLAY_LEFT_ID&&((int *)borderless_knob)[2]==DISPLAY_RIGHT_ID);
    ((int *)gm)[11]=BORDERLESS_CONTROL_ID;aa_graphics_input_state(gm);
    CHECK(*(DWORD *)((BYTE *)borderless_label+64)==0);
    CHECK(*(DWORD *)((BYTE *)borderless_bar+64)==256&&*(DWORD *)((BYTE *)borderless_knob+64)==512);
    BYTE overlay_fixture[64]={0};void *overlay_value=overlay_fixture;
    overlay_manager=&overlay_value;((void **)overlay_fixture)[11]=gm;
    aa_graphics_input_state(gm);graphics_layout(gm);
    CHECK(!*(DWORD *)((BYTE *)borderless_label+76)&&!*(DWORD *)((BYTE *)aa_label+76));
    CHECK(!*(DWORD *)((BYTE *)graphics_test_items[4]+76));
    ((void **)overlay_fixture)[11]=NULL;aa_graphics_input_state(gm);graphics_layout(gm);
    CHECK(*(DWORD *)((BYTE *)borderless_label+76));overlay_manager=NULL;
    CHECK(((unsigned *)borderless_bar)[100]==59&&((unsigned *)borderless_knob)[100]==60);
    CHECK(((float *)aa_label)[9]==580&&((float *)borderless_label)[9]==500);
    CHECK(!graphics_test_items[0][372]&&!graphics_test_items[4][372]);
    display_installed=graphics_bridge_ready=TRUE;borderless_pending=FALSE;*((BYTE *)gm+305)=FALSE;
    aa_graphics_input(gm,BORDERLESS_CONTROL_ID);CHECK(!borderless_pending&&!*((BYTE *)gm+305));
    CHECK(((int *)borderless_bar)[15]==100&&((int *)borderless_knob)[15]==100);
    aa_graphics_input(gm,DISPLAY_RIGHT_ID);CHECK(borderless_pending&&!*((BYTE *)gm+305));
    aa_graphics_input(gm,DISPLAY_RIGHT_ID);CHECK(!borderless_pending&&*((BYTE *)gm+305));
    aa_graphics_input(gm,DISPLAY_RIGHT_ID);CHECK(!borderless_pending&&!*((BYTE *)gm+305));
    aa_graphics_input(gm,DISPLAY_LEFT_ID);CHECK(!borderless_pending&&*((BYTE *)gm+305));
    aa_graphics_input(gm,DISPLAY_LEFT_ID);CHECK(borderless_pending&&!*((BYTE *)gm+305));
    aa_graphics_input(gm,DISPLAY_LEFT_ID);CHECK(!borderless_pending&&!*((BYTE *)gm+305));
    display_installed=graphics_bridge_ready=FALSE;
    /* Native showResolutionText returns early for an unchanged selection.
     * Scrolling must restore that selected label itself, without exposing all modes. */
    int saved_visible=graphics_visible;graphics_visible=2;graphics_scroll_to(gm,1,FALSE);
    CHECK(!graphics_test_items[5][372]);graphics_scroll_to(gm,0,FALSE);
    CHECK(graphics_test_items[5][372]);graphics_visible=saved_visible;graphics_layout(gm);
    static const float apply_focus_colour[4]={0,1,1,1};graphics_focus_colour=apply_focus_colour;
    colour_watch=graphics_test_items[6];((int *)gm)[11]=3;aa_graphics_tick(gm);
    CHECK(!memcmp(watched_colour,apply_focus_colour,sizeof(watched_colour)));
    CHECK(*(DWORD *)(graphics_test_items[6]+76)==*(DWORD *)(screen+40));
    ((int *)gm)[11]=RESOLUTION_FOCUS_ID;aa_graphics_tick(gm);CHECK(watched_colour[0]==1&&watched_colour[1]==1);
    aa_graphics_input(gm,AA_CONTROL_ID);CHECK(aa_pending&&!aa_requested&&((unsigned *)aa_knob)[100]==74);
    CHECK(((float *)aa_label)[12]==820&&((float *)aa_label)[13]==80);
    float native_thumb_x=((float *)aa_knob)[8];
    graphics_layout(gm);CHECK(((float *)aa_knob)[8]==native_thumb_x);
    const float thumb_scales[]={0.25f,0.5f,1.0f,1.5f,2.0f};
    int thumb_visible=graphics_visible;
    for(unsigned si=0;si<sizeof(thumb_scales)/sizeof(thumb_scales[0]);si++){
        ((float *)gm)[15]=thumb_scales[si];
        for(int flip=0;flip<4;flip++){
            aa_graphics_input(gm,AA_CONTROL_ID);
            aa_sync_art(gm);float expected_x=((float *)aa_knob)[8];
            graphics_visible=2;graphics_scroll_to(gm,1,FALSE);
            CHECK(((float *)aa_knob)[8]==expected_x&&((float *)aa_knob)[9]==((float *)aa_bar)[9]);
            CHECK(((unsigned *)aa_knob)[100]==(aa_pending?74u:75u));
            CHECK(((unsigned *)aa_knob)[101]==(aa_pending?5u:3u));
            graphics_scroll_to(gm,2,FALSE);graphics_scroll_to(gm,0,FALSE);
            CHECK(((float *)aa_knob)[8]==expected_x&&((float *)aa_knob)[9]==((float *)aa_bar)[9]);
        }
    }
    ((float *)gm)[15]=0.5f;graphics_visible=thumb_visible;aa_sync_art(gm);graphics_layout(gm);
    aa_graphics_input(gm,3);CHECK(aa_requested&&aa_attach_calls==1&&delegated==3);
    GetPrivateProfileStringW(L"graphics",L"antialiasing",L"",value,32,config_path);CHECK(!wcscmp(value,L"true"));
    aa_graphics_input(gm,AA_CONTROL_ID);CHECK(!aa_pending&&aa_requested);
    ((int *)gm)[11]=UI_SCALE_CONTROL_ID;aa_graphics_input_state(gm);
    CHECK(*(DWORD *)((BYTE *)ui_slider_label+64)==0);
    CHECK(*(DWORD *)((BYTE *)graphics_test_items[2]+64)==0&&*(DWORD *)((BYTE *)graphics_test_items[3]+64)==0);
    CHECK(((int *)ui_slider_label)[41]==-1&&((int *)ui_slider_label)[42]==-1);
    ((int *)gm)[11]=AA_CONTROL_ID;aa_graphics_tick(gm);CHECK(*(DWORD *)((BYTE *)aa_label+64)==768&&*(DWORD *)((BYTE *)graphics_test_items[2]+64)==0);
    ((int *)gm)[11]=RESOLUTION_FOCUS_ID;aa_graphics_tick(gm);
    CHECK(((int *)gm)[77]==0&&*(DWORD *)((BYTE *)aa_label+64)==0);
    CHECK(*(DWORD *)((BYTE *)graphics_test_items[2]+64)==256&&*(DWORD *)((BYTE *)graphics_test_items[3]+64)==512);
    CHECK(*(DWORD *)((BYTE *)resolution_hover+76)==1);
    delegated=-1;aa_graphics_input(gm,RESOLUTION_FOCUS_ID);CHECK(delegated==-1);
    CHECK(*(DWORD *)((BYTE *)aa_bar+76)==0&&*(DWORD *)((BYTE *)aa_knob+76)==0);
    aa_graphics_input(gm,3);CHECK(!aa_requested);aa_graphics_cleanup(gm,NULL,NULL);CHECK(!aa_label&&!aa_bar&&!aa_knob&&!resolution_hover&&!borderless_label&&removed==38);
    CHECK(!(*((BYTE *)graphics_test_items[6]+372)&2));
    CHECK(((int *)graphics_test_items[0])[38]==1&&((int *)graphics_test_items[0])[40]==1);
    CHECK(((int *)graphics_test_items[6])[38]==0&&((int *)graphics_test_items[6])[39]==-1);
    CHECK(((int *)graphics_test_items[6])[40]==-1);
    for(int i=2;i<4;i++)CHECK(((int *)graphics_test_items[i])[38]==1&&((int *)graphics_test_items[i])[39]==4&&((int *)graphics_test_items[i])[40]==4);
    testing_graphics_reel=TRUE;
    ((float *)graphics_test_items[0])[9]=580;((float *)graphics_test_items[1])[9]=580;
    ((float *)graphics_test_items[2])[9]=((float *)graphics_test_items[3])[9]=500;((float *)graphics_test_items[4])[9]=580;
    UiCanvas short_canvas={1920,720};ui_scope_tls=TlsAlloc();CHECK(ui_scope_tls!=TLS_OUT_OF_INDEXES);ui_scope_set(&short_canvas);
    aa_graphics_init(gm,NULL,NULL);CHECK(graphics_reel&&graphics_visible==3&&graphics_first==0);
    CHECK((*((BYTE *)aa_label+372)&1)&&!(*((BYTE *)ui_slider_label+372)&1));
    graphics_scroll_to(gm,1,FALSE);CHECK(graphics_first==1&&(*((BYTE *)ui_slider_label+372)&1));
    CHECK(!(*((BYTE *)graphics_test_items[2]+372)&1));
    graphics_scroll_to(gm,0,FALSE);CHECK((*((BYTE *)graphics_test_items[2]+372)&1)&&!(*((BYTE *)ui_slider_label+372)&1));
    aa_graphics_cleanup(gm,NULL,NULL);CHECK(!graphics_reel&&constructed==destroyed);
    ui_scope_set(NULL);TlsFree(ui_scope_tls);ui_scope_tls=TLS_OUT_OF_INDEXES;
    FreeLibrary(image);DeleteFileW(config_path);DeleteFileW(log_path);
    puts("PASS: original guards, Servers first, native reel lifecycle, one navigation owner, held-repeat/boundaries, deferred focus, pointer forwarding, original mask reset, vanilla hover/click/controller activation, Audio isolation, press latches, save/restart, version branding, one-time update notice, Graphics AA, Apply native focus/mask/highlight and cleanup");return 0;
}
