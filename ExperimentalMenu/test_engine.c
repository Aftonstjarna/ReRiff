#include "engine_menu.c"
#define CHECK(c) do { if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;} }while(0)
static BYTE menu[512],group[512],audio[512],screen[700],sample[1024],title[1024],rows[16][1024],sprites[40][512],vanilla_rows[3][1024],vanilla_arrows[4][512],vanilla_lines[3][512];
static void *vtable[27],*item_vtable[32];
static int delegated,initialized,cleaned,ticked,base_ticked,focused,created,added;
static int restart_calls,removed,sprites_created,colours;
static void *fake_ui=screen;
static BOOL restart_ok;
static EnableFn native_enable;
static void __thiscall mock_tick(void *self){(void)self;ticked++;}
static void __thiscall mock_base_tick(void *self){(void)self;base_ticked++;}
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
static void __thiscall mock_input(void *self,int selection){(void)self;delegated=selection;}
static void __thiscall mock_init(void *self,void *up,void *low){(void)self;(void)up;(void)low;initialized++;}
static void __thiscall mock_cleanup(void *self,void *up,void *low){(void)self;(void)up;(void)low;cleaned++;}
static void __thiscall mock_focus(void *self,int id){(void)self;focused=id;((int *)menu)[11]=id;}
static void * __cdecl mock_create(const WCHAR *text,float x,float y,unsigned font,int h,int v,float size,BOOL wrap,float spacing,BOOL a,BOOL b){
    (void)font;(void)h;(void)v;(void)size;(void)wrap;(void)spacing;(void)a;(void)b;
    if(created>=16)return NULL;
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
static void select_action(int action){scroll_to(menu,clamp_index(action-1,0,10-visible_rows),FALSE);input_entry(menu,SERVER_ACTION_BASE+action);}
static void open_servers(void){if(((int *)menu)[62]==0&&!server_page)scroll_to(menu,root_count-visible_rows,FALSE);input_entry(menu,SERVERS_ID);}
int main(int argc,char **argv){
    CHECK(argc==2);WCHAR game[MAX_PATH],own[MAX_PATH];
    MultiByteToWideChar(CP_UTF8,0,argv[1],-1,game,MAX_PATH);
    CHECK(supported_file(game));GetModuleFileNameW(NULL,own,MAX_PATH);CHECK(!supported_file(own));
    CHECK(!ReRiffStartEngineMenu());CHECK(InterlockedExchange(&installed,0)==1);
    HMODULE image=LoadLibraryExW(game,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(image);
    IMAGE_NT_HEADERS32 *nt=(IMAGE_NT_HEADERS32 *)((BYTE *)image+((IMAGE_DOS_HEADER *)image)->e_lfanew);
    CHECK(validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));
    void **actual=(void **)((BYTE *)image+0x7a5a7c);DWORD protection,ignored;
    CHECK(VirtualProtect(actual,108,PAGE_READWRITE,&protection));
    void *saved=actual[24];actual[24]=NULL;CHECK(!validate_image_at((BYTE *)image,nt->OptionalHeader.ImageBase));
    actual[24]=saved;VirtualProtect(actual,108,protection,&ignored);
    native_enable=(EnableFn)((BYTE *)image+0x4b4a0);
    CHECK(GetTempPathW(MAX_PATH,config_path));wcscat(config_path,L"reriff-settings-test.ini");DeleteFileW(config_path);
    wcscpy(log_path,config_path);wcscat(log_path,L".log");
    CHECK(WritePrivateProfileStringW(L"server",L"external_url",L"https://example.com",config_path));
    CHECK(WritePrivateProfileStringW(L"keep",L"key",L"value",config_path));
    original_init=mock_init;original_cleanup=mock_cleanup;original_input=mock_input;
    original_tick=mock_tick;base_tick=mock_base_tick;enable_input=mock_enable;
    raw_text=mock_raw;key_text=mock_key;create_text=mock_create;menu_item=mock_visible;
    add_item=mock_add;add_screen=mock_add;remove_screen=mock_remove;set_focus=mock_focus;
    create_sprite=mock_sprite;ui_instance=&fake_ui;sprite_def=mock_def;sprite_origin=mock_origin;sprite_position=mock_position;original_colour=mock_menu_colour;
    item_vtable[9]=(void *)mock_scale;
    item_vtable[3]=(void *)mock_visible;item_vtable[4]=(void *)mock_colour;
    *(void ***)menu=vtable;vtable[13]=(void *)mock_title;
    *(void ***)group=item_vtable;*(void ***)audio=item_vtable;
    ((void **)menu)[36]=title;((void **)menu)[46]=screen;
    ((void **)menu)[67]=group;((void **)menu)[68]=sample;((void **)menu)[83]=audio;
    ((float *)menu)[14]=0.5f;((float *)menu)[15]=0.5f;((float *)menu)[66]=211;
    for(int i=0;i<4;i++){
        BYTE *row=i?vanilla_rows[i-1]:sample;*(void ***)row=item_vtable;((void **)menu)[68+i]=row;
        ((float *)row)[8]=960;((float *)row)[9]=324+i*211;((float *)row)[12]=1344;((float *)row)[13]=106;
        row[372]=1;*(DWORD *)(row+76)=0xffffffffu;
        *(void ***)vanilla_arrows[i]=item_vtable;((float *)vanilla_arrows[i])[8]=1372.5f;((float *)vanilla_arrows[i])[9]=354+i*211;
        ((void **)menu)[73+i]=vanilla_arrows[i];vanilla_arrows[i][372]=1;
        if(i<3){*(void ***)vanilla_lines[i]=item_vtable;((void **)menu)[78+i]=vanilla_lines[i];vanilla_lines[i][372]=1;}
    }
    group[372]=1;
    init_entry(menu,NULL,NULL);CHECK(initialized==1&&created==13&&added==34&&sprites_created==21&&visible_rows==4&&root_count==5);
    CHECK(!wcscmp(label(0),L"Servers")&&group[372]==1&&!audio[372]);
    CHECK(root_first==0&&!(*((BYTE *)server_rows[0]+372)&1));
    CHECK(((float *)root_rows[1].text)[9]-((float *)root_rows[0].text)[9]==row_gap);
    CHECK((*((BYTE *)root_rows[0].line+372)&1)&&!(*((BYTE *)root_rows[3].line+372)&1));
    scroll_to(menu,1,FALSE);CHECK(root_first==1&&(*((BYTE *)server_rows[0]+372)&1)&&!sample[372]);
    ((int *)menu)[11]=root_rows[1].id;scrollbar_shortcuts(menu);CHECK(((DWORD *)scroll_buttons[0])[16]==64);
    input_entry(menu,SCROLL_UP_ID);CHECK(root_first==0&&sample[372]&&focused==0);
    /* Original options, Friends ID 8 and Settings ID 9 are forwarded intact. */
    for(int id=0;id<=10;id++){input_entry(menu,id);CHECK(delegated==id&&!server_page);}
    ((int *)menu)[62]=1;open_servers();CHECK(!server_page&&delegated==SERVERS_ID);
    ((int *)menu)[62]=0;tick_entry(menu);CHECK(ticked==1);
    open_servers();CHECK(server_page&&focused==1501&&!group[372]);
    CHECK(!wcscmp(label(8),L"Save and restart"));CHECK(((float *)server_rows[1])[12]==line_width&&((float *)server_rows[1])[13]==106);
    CHECK(!wcscmp(label(1),L"Skip launcher")&&!wcscmp(label(4),L"External only")&&!wcscmp(label(5),L"Networking"));
    CHECK(((unsigned *)toggles[0][1])[100]==75&&((unsigned *)toggles[0][1])[101]==3);
    CHECK(((unsigned *)toggles[2][1])[100]==74&&((unsigned *)toggles[2][1])[101]==5);
    CHECK(*(DWORD *)((BYTE *)toggles[0][0]+76)==0);CHECK(((int *)toggles[0][0])[38]==1&&((int *)toggles[0][0])[39]==1&&((int *)toggles[0][0])[40]==1);
    int before_colour=colours;float colour[4]={0,1,1,0.8f};hook_colour(menu,1501,colour);CHECK(colours==before_colour+2);
    CHECK(*(DWORD *)((BYTE *)server_rows[1]+76)==0xffffffffu);
    *(DWORD *)((BYTE *)server_rows[1]+356)=0x12345678;
    tick_entry(menu);tick_entry(menu);CHECK(ticked==1&&base_ticked==2);
    CHECK(*(DWORD *)((BYTE *)server_rows[1]+356)==0x12345678);
    /* Every visible row retains the original interval and separators. Hidden
     * rows have no input mask, including the switches on those rows. */
    CHECK(((float *)server_rows[2])[9]-((float *)server_rows[1])[9]==row_gap);
    CHECK(((float *)toggles[0][0])[9]==((float *)server_rows[1])[9]+arrow_dy);
    CHECK(!(*((BYTE *)server_rows[5]+372)&1)&&*(DWORD *)((BYTE *)server_rows[5]+76)==0);
    BOOL network_before=pending.enabled;input_entry(menu,1505);CHECK(pending.enabled==network_before);
    ((int *)menu)[11]=1504;scrollbar_shortcuts(menu);CHECK(((DWORD *)scroll_buttons[1])[16]==128);
    input_entry(menu,SCROLL_DOWN_ID);CHECK(page_first==1&&focused==1505);
    input_entry(menu,SCROLL_UP_ID);CHECK(page_first==0&&focused==1501);
    HWND window=CreateWindowExW(0,L"STATIC",L"Scroll input fixture",WS_OVERLAPPEDWINDOW,0,0,800,600,NULL,NULL,GetModuleHandleW(NULL),NULL);
    CHECK(window);scroll_previous=DefWindowProcW;
    scroll_wndproc(window,WM_MOUSEWHEEL,MAKEWPARAM(0,(WORD)-60),0);sync_scroll(menu);CHECK(page_first==0);
    scroll_wndproc(window,WM_MOUSEWHEEL,MAKEWPARAM(0,(WORD)-60),0);CHECK(page_first==0);sync_scroll(menu);CHECK(page_first==1);
    scroll_wndproc(window,WM_KEYDOWN,VK_END,0);sync_scroll(menu);CHECK(page_first==6);
    CHECK(thumb_y+thumb_height-bar_bottom>-0.01f&&thumb_y+thumb_height-bar_bottom<0.01f);
    scroll_wndproc(window,WM_KEYDOWN,VK_HOME,0);sync_scroll(menu);CHECK(page_first==0);
    RECT client;CHECK(GetClientRect(window,&client));
    int mx=(int)(bar_x/ui_width*client.right),my=(int)((bar_bottom-1)/ui_height*client.bottom);
    scroll_wndproc(window,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(mx,my));CHECK(bar_dragging&&page_first==0);
    sync_scroll(menu);CHECK(page_first==6);
    scroll_wndproc(window,WM_LBUTTONUP,0,MAKELPARAM(mx,my));CHECK(!bar_dragging);
    scroll_to(menu,0,FALSE);DestroyWindow(window);scroll_previous=NULL;
    ((int *)menu)[11]=1501;
    int before=delegated;input_entry(menu,1);input_entry(menu,2003);CHECK(delegated==before&&server_page);
    select_action(1);CHECK(pending.skip_launcher&&((unsigned *)toggles[0][1])[100]==74&&((unsigned *)toggles[0][1])[101]==5);select_action(9);CHECK(!(*((BYTE *)toggles[0][0]+372)&1));
    CHECK(!server_page&&group[372]==1&&!audio[372]&&focused==SERVERS_ID);
    ((int *)menu)[62]=1;group[372]=0;audio[372]=1;sync_scroll(menu);CHECK(!scroll_active);
    ((int *)menu)[62]=0;group[372]=1;audio[372]=0;((int *)menu)[11]=0;sync_scroll(menu);
    CHECK(root_first==0&&sample[372]);
    ((int *)menu)[102]=1;sync_scroll(menu);CHECK(!scroll_active);
    ((int *)menu)[102]=0;sync_scroll(menu);CHECK(scroll_active);
    open_servers();CHECK(!pending.skip_launcher);
    select_action(1);select_action(2);CHECK(!pending.only&&!wcscmp(pending.fallback,L"http://127.0.0.1:5144"));
    select_action(3);CHECK(pending.only&&!wcscmp(pending.external,L"https://example.com"));
    select_action(4);CHECK(!pending.only);select_action(6);select_action(1);select_action(7);CHECK(pending.skip_launcher);
    WCHAR value[2048];GetPrivateProfileStringW(L"keep",L"key",L"",value,2048,config_path);CHECK(!wcscmp(value,L"value"));
    select_action(5);CHECK(!pending.enabled&&*pending.external&&*pending.fallback);select_action(10);CHECK(!server_page);
    open_servers();load_settings();CHECK(!pending.enabled);
    select_action(5);request_restart=mock_restart;restart_ok=FALSE;select_action(8);CHECK(restart_calls==1&&!restarting);
    WCHAR valid[MAX_PATH];wcscpy(valid,config_path);wcscat(config_path,L"\\missing\\server.ini");select_action(8);
    CHECK(restart_calls==1&&!restarting);wcscpy(config_path,valid);
    restart_ok=TRUE;select_action(8);CHECK(restart_calls==2&&restarting);
    select_action(1);CHECK(pending.skip_launcher);restart_started=GetTickCount()-15001;tick_entry(menu);CHECK(!restarting);
    input_entry(menu,2000);CHECK(!server_page&&group[372]==1);
    input_entry(menu,2000);CHECK(delegated==2000);
    cleanup_entry(menu,NULL,NULL);CHECK(cleaned==1&&!page_owner&&!server_rows[0]&&removed==30);
    FreeLibrary(image);DeleteFileW(config_path);DeleteFileW(log_path);
    puts("PASS: original guards, vanilla row gaps/dividers, bounded hidden input, controller shortcuts, wheel/page/home/end/drag queues, focus/back/audio isolation, save/restart and cleanup");return 0;
}
