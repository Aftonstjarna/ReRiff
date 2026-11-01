#include "engine_menu.c"
#define CHECK(c) do { if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;} }while(0)
static BYTE menu[512],group[512],audio[512],screen[700],sample[1024],title[1024],rows[256][1024],sprites[40][512],vanilla_rows[3][1024],vanilla_arrows[4][512],vanilla_lines[3][512];
static void *vtable[27],*item_vtable[32];
static int delegated,initialized,cleaned,ticked,base_ticked,focused,created,added;
static int restart_calls,removed,sprites_created,colours;
static void *fake_ui=screen;
static BOOL restart_ok;
static EnableFn native_enable;
static void __thiscall mock_tick(void *self){(void)self;ticked++;}
static void __thiscall mock_base_tick(void *self);
static int pointer_hover=-1;static BOOL accept_press;
static void __thiscall mock_enable(void *self,BYTE enabled){
    if(enabled)native_enable(self,TRUE);
    else *(DWORD *)((BYTE *)self+76)=0;
}
static void __thiscall mock_visible(void *self,BYTE visible){
    *((BYTE *)self+372)=(*((BYTE *)self+372)&~1)|!!visible;
}
static void __thiscall mock_colour(void *self,const float *value){(void)self;(void)value;colours++;}
static void __thiscall mock_menu_colour(void *self,int id,const float *value){(void)self;(void)id;(void)value;}
static void __thiscall mock_scale(void *self,const float *value){((float *)self)[14]=value[0];((float *)self)[15]=value[1];}
static void __thiscall mock_def(void *self,unsigned id){((unsigned *)self)[100]=id;}
static void __thiscall mock_origin(void *self,unsigned id){((unsigned *)self)[101]=id;}
static void __thiscall mock_position(void *self,const float *pos){((float *)self)[8]=pos[0];((float *)self)[9]=pos[1];}
static void *__thiscall mock_sprite(void *ui,int id,const float *pos,int origin,int item_id){
    (void)ui;if(sprites_created>=40)abort();void *sprite=sprites[sprites_created++];*(void ***)sprite=item_vtable;
    ((int *)sprite)[2]=item_id;((float *)sprite)[12]=300;((float *)sprite)[13]=1;mock_def(sprite,id);mock_origin(sprite,origin);mock_position(sprite,pos);return sprite;
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
static void select_action(int action){logical_selection=action-1;scroll_to(menu,clamp_index(action-1,0,10-visible_rows),FALSE);apply_pending_focus(menu);input_entry(menu,SERVER_ACTION_BASE+action);}
static void open_servers(void){if(((int *)menu)[62]==0&&!server_page)scroll_to(menu,0,FALSE);input_entry(menu,SERVERS_ID);}
typedef struct FakeReel {BYTE object[532];void *children[16];int count,maximum,selection;} FakeReel;
static void *fake_reel_table[37];static int constructed,destroyed,bars,ticks,forwarded;
static DWORD held_input;
static int aa_attach_calls;
static BOOL mock_aa_attach(void){aa_attach_calls++;return TRUE;}
static BYTE graphics_test_menu[512],graphics_test_items[7][1024];
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
static DWORD mock_direction(void){return held_input;}
static void *__cdecl mock_allocate(size_t size){if(size!=532)abort();return calloc(1,sizeof(FakeReel));}
static void *__cdecl mock_reel_ctor(void *self,void *parent,float spacing,int direction){
    (void)parent;if(spacing!=row_gap||direction!=1)abort();*(void ***)self=fake_reel_table;constructed++;return self;
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
    (void)self;if(!(x>0&&bottom>top)||pc!=1||width||top+bottom<ui_height-0.01f||top+bottom>ui_height+0.01f)abort();bars++;
}
static void __thiscall mock_menu_focus(void *self,int id){
    ((int *)self)[11]=id;static const float c[4]={0,1,1,0.8f};
    if(server_page&&id>1500&&id<=1510)mock_colour(server_rows[id-1500],c);
    else for(int i=0;i<root_count;i++)if(root_rows[i].id==id)mock_colour(root_rows[i].text,c);
}
static LRESULT CALLBACK mock_window(HWND w,UINT message,WPARAM p,LPARAM l){(void)w;(void)message;(void)p;(void)l;forwarded++;return 0;}
int main(int argc,char **argv){
    CHECK(argc==2);WCHAR game[MAX_PATH],own[MAX_PATH];MultiByteToWideChar(CP_UTF8,0,argv[1],-1,game,MAX_PATH);
    CHECK(supported_file(game));GetModuleFileNameW(NULL,own,MAX_PATH);CHECK(!supported_file(own));
    CHECK(!ReRiffStartEngineMenu());CHECK(InterlockedExchange(&installed,0)==1);
    HMODULE image=LoadLibraryExW(game,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(image);
    IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)((BYTE *)image+((IMAGE_DOS_HEADER *)image)->e_lfanew);
    CHECK(validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));
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
    create_sprite=mock_sprite;ui_instance=&fake_ui;sprite_def=mock_def;sprite_origin=mock_origin;sprite_position=mock_position;original_colour=mock_menu_colour;
    game_allocate=mock_allocate;reel_ctor=mock_reel_ctor;reel_add=mock_reel_add;reel_limit=mock_reel_limit;
    reel_tick=mock_reel_tick;reel_move=mock_reel_move;reel_selection=mock_reel_selection;reel_bar=mock_reel_bar;
    fake_reel_table[0]=(void *)mock_reel_delete;reel_vtable=fake_reel_table;direction_input=mock_direction;
    item_vtable[9]=(void *)mock_scale;item_vtable[3]=(void *)mock_visible;item_vtable[4]=(void *)mock_colour;
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
    *(DWORD *)(screen+40)=1;group[372]=1;init_entry(menu,NULL,NULL);CHECK(initialized==1&&visible_rows==4&&root_count==5&&added==30);
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
    CHECK(!wcscmp(label(5),L"Force Offline")&&pending.enabled&&((unsigned *)toggles[2][1])[100]==75);
    select_action(5);CHECK(!pending.enabled&&((unsigned *)toggles[2][1])[100]==74);
    select_action(6);select_action(7);CHECK(!pending.enabled&&((unsigned *)toggles[2][1])[100]==74);
    select_action(5);CHECK(pending.enabled&&((unsigned *)toggles[2][1])[100]==75);select_action(6);
    scroll_to(menu,0,FALSE);apply_pending_focus(menu);
    CHECK(!wcscmp(label(8),L"Save and restart"));CHECK(((float *)server_rows[2])[9]-((float *)server_rows[1])[9]==row_gap);
    CHECK(!(*((BYTE *)server_rows[5]+372)&1)&&*(DWORD *)((BYTE *)server_rows[5]+76)==0);
    *(DWORD *)((BYTE *)server_rows[1]+356)=0x12345678;tick_entry(menu);CHECK(*(DWORD *)((BYTE *)server_rows[1]+356)==0x12345678);
    /* Native drag/tap movement is consumed from the original reel, with no
     * competing custom scrollbar buttons or native UI directional pass. */
    ((FakeReel *)native_reel)->selection=6;prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==6&&focused==1507);
    HWND window=CreateWindowExW(0,L"STATIC",L"Native reel input fixture",WS_OVERLAPPEDWINDOW,0,0,800,600,NULL,NULL,GetModuleHandleW(NULL),NULL);
    CHECK(window);scroll_previous=mock_window;
    scroll_wndproc(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(700,300));CHECK(forwarded==1);
    scroll_wndproc(window,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(700,400));CHECK(forwarded==2);
    scroll_wndproc(window,WM_LBUTTONUP,0,MAKELPARAM(700,400));CHECK(forwarded==3);
    scroll_wndproc(window,WM_KEYDOWN,VK_HOME,0);prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==0);
    scroll_wndproc(window,WM_MOUSEWHEEL,MAKEWPARAM(0,(WORD)-120),0);CHECK(page_first==0);prepare_scroll(menu);sync_scroll(menu);CHECK(page_first==1);
    scroll_to(menu,0,FALSE);apply_pending_focus(menu);DestroyWindow(window);scroll_previous=NULL;
    dispatching_menu=TRUE;scroll_to(menu,1,TRUE);int previous_focus=focused;apply_pending_focus(menu);CHECK(focused==previous_focus&&pending_focus>=0);
    dispatching_menu=FALSE;apply_pending_focus(menu);CHECK(focused==1505);
    select_action(1);CHECK(pending.skip_launcher);select_action(6);select_action(1);select_action(7);CHECK(pending.skip_launcher);
    WCHAR value[32];GetPrivateProfileStringW(L"keep",L"key",L"",value,32,config_path);CHECK(!wcscmp(value,L"value"));
    GetPrivateProfileStringW(L"launcher",L"auto_update_on_skip",L"",value,32,config_path);CHECK(!wcscmp(value,L"true"));
    select_action(9);CHECK(!server_page&&root_first==0&&focused==SERVERS_ID);
    ((int *)menu)[62]=1;group[372]=0;audio[372]=1;prepare_scroll(menu);CHECK(!native_reel&&!scroll_active);
    int audio_before=ticked;tick_entry(menu);CHECK(ticked==audio_before+1);
    ((int *)menu)[62]=0;group[372]=1;audio[372]=0;prepare_scroll(menu);CHECK(native_reel&&scroll_active);
    open_servers();request_restart=mock_restart;restart_ok=FALSE;select_action(8);CHECK(restart_calls==1&&!restarting);
    restart_ok=TRUE;select_action(8);CHECK(restart_calls==2&&restarting);restart_started=GetTickCount()-15001;tick_entry(menu);CHECK(!restarting);
    cleanup_entry(menu,NULL,NULL);CHECK(cleaned==1&&!page_owner&&!native_reel&&destroyed==constructed&&removed==26);
    main_init_original=mock_init;main_cleanup_original=mock_cleanup;main_tick_original=mock_tick;native_resolution=mock_resolution;queue_update_notice=mock_notice;
    wcscpy(update_notice_path,config_path);wcscat(update_notice_path,L".update.ini");
    CHECK(WritePrivateProfileStringW(L"update",L"version",MENU_WIDE(RERIFF_VERSION),update_notice_path));
    notice_accept=FALSE;deliver_update_notice();CHECK(notices==1&&GetFileAttributesW(update_notice_path)!=INVALID_FILE_ATTRIBUTES);
    notice_accept=TRUE;branded_main_init(menu,NULL,NULL);
    CHECK(watermark&&watermark_owner==menu&&((int *)watermark)[2]==-1&&*(DWORD *)((BYTE *)watermark+76)==0);
    CHECK(!wcscmp((const WCHAR *)((BYTE *)watermark+512),L"ReRiff v" MENU_WIDE(RERIFF_VERSION)));
    CHECK(notices==2&&!wcscmp(notice_text,L"ReRiff has updated to " MENU_WIDE(RERIFF_VERSION))&&GetFileAttributesW(update_notice_path)==INVALID_FILE_ATTRIBUTES);
    deliver_update_notice();CHECK(notices==2);
    ((float *)menu)[73]=48;((float *)watermark)[13]=64;branded_main_tick(menu);
    CHECK(((float *)watermark)[14]==0.5f&&((float *)watermark)[15]==0.5f);
    CHECK(((float *)watermark)[8]==1908&&((float *)watermark)[9]==1036);
    ((float *)menu)[73]=24;branded_main_tick(menu);
    CHECK(((float *)watermark)[15]==0.25f&&((float *)watermark)[9]==1052);
    branded_main_cleanup(menu,NULL,NULL);CHECK(!watermark&&!watermark_owner&&removed==27);
    branded_main_tick(menu);CHECK(watermark&&notices==2);branded_main_cleanup(menu,NULL,NULL);CHECK(!watermark&&removed==28);
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
    ((float *)graphics_test_items[4])[8]=550;((float *)graphics_test_items[4])[9]=580;((float *)graphics_test_items[5])[9]=500;
    ((int *)graphics_test_items[0])[2]=4;((int *)graphics_test_items[6])[2]=3;
    ((int *)graphics_test_items[2])[2]=1;((int *)graphics_test_items[3])[2]=2;
    for(int i=0;i<7;i++)for(int field=39;field<=42;field++)((int *)graphics_test_items[i])[field]=-1;
    ((int *)graphics_test_items[0])[38]=1;((int *)graphics_test_items[0])[39]=1;((int *)graphics_test_items[0])[40]=1;
    ((int *)graphics_test_items[2])[38]=((int *)graphics_test_items[3])[38]=1;
    ((int *)graphics_test_items[2])[39]=((int *)graphics_test_items[3])[39]=4;
    ((int *)graphics_test_items[2])[40]=((int *)graphics_test_items[3])[40]=4;
    graphics_init_original=mock_init;graphics_cleanup_original=mock_cleanup;graphics_tick_original=mock_tick;graphics_input_original=mock_input;graphics_colour_original=mock_menu_colour;
    attach_antialiasing=mock_aa_attach;aa_requested=aa_fault=0;aa_graphics_init(gm,NULL,NULL);
    CHECK(aa_label&&aa_bar&&aa_knob&&!aa_pending&&((int *)aa_label)[2]==AA_CONTROL_ID);
    /* Exercise multiple complete native row loops in both directions, including
     * both resolution arrows and escape from the mouse-focused Apply button. */
    void *nav_items[5]={graphics_test_items[2],graphics_test_items[3],graphics_test_items[0],aa_label,graphics_test_items[6]};
    const int down_cycle[]={4,AA_CONTROL_ID,1,4,AA_CONTROL_ID,1,4,AA_CONTROL_ID,1};
    const int up_cycle[]={AA_CONTROL_ID,4,1,AA_CONTROL_ID,4,1,AA_CONTROL_ID,4,1};
    CHECK(graphics_cycle(nav_items,5,1,40,down_cycle,9));
    CHECK(graphics_cycle(nav_items,5,2,40,down_cycle,9));
    CHECK(graphics_cycle(nav_items,5,1,39,up_cycle,9));
    CHECK(graphics_cycle(nav_items,5,2,39,up_cycle,9));
    CHECK(((int *)graphics_test_items[2])[40]==((int *)graphics_test_items[0])[2]);
    CHECK(((int *)graphics_test_items[3])[40]==((int *)graphics_test_items[0])[2]);
    CHECK(((int *)graphics_test_items[0])[40]==((int *)aa_label)[2]);
    CHECK(((int *)aa_label)[38]==1&&((int *)aa_label)[40]==1);
    CHECK(((int *)graphics_test_items[6])[38]==1&&((int *)graphics_test_items[6])[39]==AA_CONTROL_ID);
    CHECK(((int *)graphics_test_items[6])[40]==1);
    CHECK(((int *)aa_label)[39]==4&&((int *)graphics_test_items[0])[39]==1);
    CHECK(((float *)aa_label)[9]==620&&((float *)graphics_test_items[0])[9]==540&&((float *)graphics_test_items[5])[9]==460);
    aa_graphics_input(gm,AA_CONTROL_ID);CHECK(aa_pending&&!aa_requested&&((unsigned *)aa_knob)[100]==74);
    CHECK(((float *)aa_label)[12]==820&&((float *)aa_label)[13]==80);
    aa_graphics_input(gm,3);CHECK(aa_requested&&aa_attach_calls==1&&delegated==3);
    GetPrivateProfileStringW(L"graphics",L"antialiasing",L"",value,32,config_path);CHECK(!wcscmp(value,L"true"));
    aa_graphics_input(gm,AA_CONTROL_ID);CHECK(!aa_pending&&aa_requested);
    ((int *)gm)[11]=AA_CONTROL_ID;aa_graphics_tick(gm);CHECK(*(DWORD *)((BYTE *)aa_label+64)==768&&*(DWORD *)((BYTE *)graphics_test_items[2]+64)==0);
    CHECK(*(DWORD *)((BYTE *)aa_bar+76)==0&&*(DWORD *)((BYTE *)aa_knob+76)==0);
    aa_graphics_input(gm,3);CHECK(!aa_requested);aa_graphics_cleanup(gm,NULL,NULL);CHECK(!aa_label&&!aa_bar&&!aa_knob&&removed==31);
    CHECK(((int *)graphics_test_items[0])[38]==1&&((int *)graphics_test_items[0])[40]==1);
    CHECK(((int *)graphics_test_items[6])[38]==0&&((int *)graphics_test_items[6])[39]==-1);
    CHECK(((int *)graphics_test_items[6])[40]==-1);
    for(int i=2;i<4;i++)CHECK(((int *)graphics_test_items[i])[38]==1&&((int *)graphics_test_items[i])[39]==4&&((int *)graphics_test_items[i])[40]==4);
    FreeLibrary(image);DeleteFileW(config_path);DeleteFileW(log_path);
    puts("PASS: original guards, Servers first, native reel lifecycle, one navigation owner, held-repeat/boundaries, deferred focus, pointer forwarding, original mask reset, vanilla hover/click/controller activation, Audio isolation, press latches, save/restart, version branding, one-time update notice, Graphics AA control and cleanup");return 0;
}
