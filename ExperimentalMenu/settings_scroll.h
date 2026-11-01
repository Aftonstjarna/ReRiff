/* Original gaMenuReel owns scrollbar rendering and pointer interaction.
 * The native Settings adapter owns only row layout and one logical selection. */
#define ROOT_LIMIT 6
#define SCROLL_EXTRA_LIMIT 10
static struct RootRow {void *text,*arrow,*line;int id;float x,arrow_x,arrow_dy;} root_rows[ROOT_LIMIT];
static int root_count,root_first,page_first,visible_rows,laid_out=-1,laid_first=-1,laid_active=-1;
static float row_x,row_start,row_gap,line_width,line_dy,arrow_dy,ui_width,ui_height;
static void *page_lines[10],*scroll_extras[SCROLL_EXTRA_LIMIT];static int extra_count;
static volatile LONG scroll_active,scroll_delta,scroll_requested=-1;
static BOOL scroll_ready;
static HWND scroll_window;static WNDPROC scroll_previous;static int wheel_remainder;
static int logical_selection,pending_focus=-1,last_direction;
static DWORD direction_started,next_repeat;
static BOOL dispatching_menu;

typedef void *(__cdecl *ReelCtorFn)(void *,void *,float,int);
typedef void (__cdecl *ReelAddFn)(void *,void *,void *);
typedef void (__cdecl *ReelLimitFn)(void *,int);
typedef void (__cdecl *ReelTickFn)(void *);
typedef void (__cdecl *ReelMoveFn)(void *,int,float *);
typedef int (__cdecl *ReelSelectionFn)(void *);
typedef void (__cdecl *ReelBarFn)(void *,float,float,float,int,float *);
static ReelCtorFn reel_ctor;static ReelAddFn reel_add;static ReelLimitFn reel_limit;
static ReelTickFn reel_tick;static ReelMoveFn reel_move;static ReelSelectionFn reel_selection;static ReelBarFn reel_bar;
static void *(__cdecl *game_allocate)(size_t);
static void **reel_vtable,**input_manager;
static void *native_reel;static int native_reel_page=-1;
static BOOL bridge_ready;

static int clamp_index(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
static void colour_item(void *item,const float *c){
    ((void (__thiscall *)(void *,const float *))(*(void ***)item)[4])(item,c);
}
static void scale_item(void *item,float x,float y){
    float size[2]={x,y};((void (__thiscall *)(void *,const float *))(*(void ***)item)[9])(item,size);
}
static void move_item(void *item,float x,float y){if(item){float pos[2]={x,y};sprite_position(item,pos);}}
static void row_visible(void *item,BOOL visible,DWORD mask){
    if(!item)return;
    if(!!(*((BYTE *)item+372)&1)!=!!visible)show_item(item,(BYTE)visible);
    DWORD wanted=visible?mask:0;
    if(*(DWORD *)((BYTE *)item+76)!=wanted){enable_input(item,wanted!=0);*(DWORD *)((BYTE *)item+76)=wanted;}
}
static void *scroll_sprite(void *parent,BOOL screen,int sprite,float x,float y,int origin){
    float pos[2]={x,y};void *item=create_sprite(*ui_instance,sprite,pos,origin,-1);
    if(!item)return NULL;
    ((int *)item)[15]=100;menu_item(item,FALSE);enable_input(item,FALSE);show_item(item,FALSE);
    static const float normal[4]={1,1,1,0.9f};colour_item(item,normal);
    ((int *)item)[1]++;(screen?add_screen:add_item)(parent,item);
    if(screen)scroll_extras[extra_count++]=item;
    return item;
}
static void set_line_size(void *line,float width,float height){
    float w=((float *)line)[12],h=((float *)line)[13];
    if(w>0&&h>0)scale_item(line,width/w,height/h);
}
static int view_count(void){return server_page?10:root_count;}
static int view_first(void){return server_page?page_first:root_first;}
static int row_id(int index){return server_page?SERVER_ACTION_BASE+index+1:root_rows[index].id;}
static int focused_index(void *menu){
    int id=((int *)menu)[11];
    for(int i=0;i<view_count();i++)if(row_id(i)==id)return i;
    return -1;
}
static void layout_scroll(void *menu){
    if(!scroll_ready)return;
    BOOL active=server_page||(((int *)menu)[62]==0&&*((BYTE *)((void **)menu)[67]+372)&1);
    if(((int *)menu)[102]!=0)active=FALSE;
    DWORD list_mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    int count=view_count(),first=clamp_index(view_first(),0,count>visible_rows?count-visible_rows:0);
    if(server_page)page_first=first;else root_first=first;
    for(int i=0;i<10;i++){
        BOOL show=active&&server_page&&i>=first&&i<first+visible_rows;
        void *row=server_rows[i+1];int action=i+1;BOOL toggle=action==1||action==4||action==5;
        row_visible(row,show,list_mask);
        if(show){
            float y=row_start+(i-first)*row_gap;
            move_item(row,toggle?row_x-line_width*0.5f:row_x,y);
            ((float *)row)[12]=toggle?line_width:server_row_width;((float *)row)[13]=server_row_height;
        }
        BOOL line_show=show&&i<first+visible_rows-1&&i<9;
        row_visible(page_lines[i],line_show,0);
        if(line_show)move_item(page_lines[i],row_x,row_start+(i-first)*row_gap+line_dy);
    }
    if(!server_page&&((int *)menu)[62]==0){
        for(int i=0;i<root_count;i++){
            struct RootRow *r=&root_rows[i];BOOL show=active&&i>=first&&i<first+visible_rows;
            row_visible(r->text,show,list_mask);row_visible(r->arrow,show,0);
            BOOL line_show=show&&i<first+visible_rows-1&&i<root_count-1;
            row_visible(r->line,line_show,0);
            if(show){
                float y=row_start+(i-first)*row_gap;move_item(r->text,r->x,y);
                move_item(r->arrow,r->arrow_x,y+r->arrow_dy);
                if(line_show)move_item(r->line,row_x,y+line_dy);
            }
        }
    }
    for(int i=0;i<3;i++){
        int index=toggle_actions[i]-1;
        BOOL show=active&&server_page&&index>=first&&index<first+visible_rows;
        for(int p=0;p<2;p++)row_visible(toggles[i][p],show,0);
        if(show)move_item(toggles[i][0],row_x+line_width*0.5f,row_start+(index-first)*row_gap+arrow_dy);
    }
    update_toggles(menu);
    scroll_active=active;
    laid_active=active;laid_out=server_page?1:(((int *)menu)[62]==0?0:2);laid_first=first;
}

static void release_native_reel(void){
    if(native_reel){
        void *old=native_reel;native_reel=NULL;native_reel_page=-1;
        /* The helper holds the sole reference; the original deleting dtor
         * unregisters the four scrollbar visuals and releases spacer children. */
        ((int *)old)[1]--;
        ((void *(__thiscall *)(void *,unsigned))(*(void ***)old)[0])(old,1);
    }
}
static BOOL create_native_reel(void *menu){
    if(native_reel||view_count()<=visible_rows)return TRUE;
    void *storage=game_allocate(532);if(!storage)return FALSE;
    native_reel=reel_ctor(storage,menu,row_gap,1);
    if(!native_reel)return FALSE;
    if(*(void ***)native_reel!=reel_vtable){native_reel=NULL;log_line("Unexpected native reel object; menu not enabled.");return FALSE;}
    ((int *)native_reel)[1]++;
    for(int i=0;i<view_count();i++){
        /* Real native options establish the reel's range, as in the file
         * picker. Empty noninteractive rows carry no Settings actions. */
        void *spacer=create_text(L"",0,i*row_gap,3,1,0,-1,FALSE,-1,FALSE,TRUE);
        if(!spacer){release_native_reel();return FALSE;}
        menu_item(spacer,FALSE);enable_input(spacer,FALSE);show_item(spacer,FALSE);
        ((int *)spacer)[1]++;reel_add(native_reel,spacer,NULL);
    }
    reel_limit(native_reel,visible_rows);
    float bar_height=(visible_rows-1)*row_gap+server_row_height;
    float bar_top=(ui_height-bar_height)*0.5f;
    reel_bar(native_reel,ui_width*0.92f,bar_top,bar_top+bar_height,1,NULL);
    float instant=0;reel_move(native_reel,view_first(),&instant);
    native_reel_page=server_page?1:0;return TRUE;
}
static void scroll_to(void *menu,int first,BOOL select_edge){
    int count=view_count(),old=view_first();first=clamp_index(first,0,count>visible_rows?count-visible_rows:0);
    if(server_page)page_first=first;else root_first=first;
    if(native_reel){float instant=0;reel_move(native_reel,first,&instant);}
    if(select_edge)logical_selection=first>old?first+visible_rows-1:first;
    logical_selection=clamp_index(logical_selection,first,first+visible_rows-1);
    if(first!=old||laid_first!=first)layout_scroll(menu);
    pending_focus=row_id(logical_selection);
}
static BOOL active_list(void *menu){return scroll_ready&&((int *)menu)[102]==0&&(server_page||(((int *)menu)[62]==0&&(*((BYTE *)((void **)menu)[67]+372)&1)));}
static DWORD poll_direction(void){
    if(!readable(input_manager,sizeof(void *))||!readable(*input_manager,sizeof(void *)))return 0;
    void *manager=*input_manager;
    void *device=((void *(__thiscall *)(void *,unsigned))(*(void ***)manager)[1])(manager,0);
    if(!readable(device,sizeof(void *)))return 0;
    /* Same held masks/virtual button test as gaFileSelectInterface.tickInput. */
    DWORD held=((DWORD (__thiscall *)(void *))(*(void ***)device)[5])(device);
    for(DWORD key=64;key<=128;key*=2)
        if(((BYTE (__thiscall *)(void *,DWORD))(*(void ***)device)[0])(device,key))held|=key;
    return held&192;
}
static DWORD (*direction_input)(void)=poll_direction;
static BOOL navigate_direction(void *menu,DWORD held,DWORD now){
    if(!active_list(menu)){last_direction=0;return FALSE;}
    int direction=held==64?-1:held==128?1:0;
    if(!held){last_direction=0;return FALSE;}
    if(!direction)return TRUE; /* Opposite directions do not invoke two owners. */
    BOOL step=direction!=last_direction||(LONG)(now-next_repeat)>=0;
    if(direction!=last_direction)direction_started=now;
    last_direction=direction;
    if(step){
        int interval=250-(int)((now-direction_started)*0.07f);if(interval<20)interval=20;
        next_repeat=now+(DWORD)interval;
        logical_selection=clamp_index(logical_selection+direction,0,view_count()-1);
        int first=view_first();
        if(logical_selection<first)first=logical_selection;
        if(logical_selection>=first+visible_rows)first=logical_selection-visible_rows+1;
        scroll_to(menu,first,FALSE);
    }
    return TRUE; /* The file picker also gates gaMenu.tick while navigating. */
}
static void apply_pending_focus(void *menu){
    if(pending_focus<0||dispatching_menu||!active_list(menu))return;
    int id=pending_focus;pending_focus=-1;
    for(int i=0;i<view_count();i++)if(row_id(i)==id)logical_selection=i;
    static const float normal[4]={1,1,1,0.9f};
    for(int i=0;i<root_count;i++)colour_item(root_rows[i].text,normal);
    for(int i=1;i<=10;i++)colour_item(server_rows[i],normal);
    for(int i=0;i<3;i++)for(int p=0;p<2;p++)colour_item(toggles[i][p],normal);
    focus_row(menu,id);
    /* Synchronize the menu's own focus/colour bookkeeping after the screen
     * focus change, rather than competing inside a selection callback. */
    ((InputFn)(*(void ***)menu)[25])(menu,id);
}
static void reconcile_list_input(void *menu);
static void prepare_scroll(void *menu){
    if(!scroll_ready)return;
    int page=server_page?1:(((int *)menu)[62]==0?0:2);BOOL active=active_list(menu);
    if(native_reel&&(!active||native_reel_page!=page))release_native_reel();
    if(page!=laid_out||active!=laid_active){
        layout_scroll(menu);
        if(active&&pending_focus<0){int focused=focused_index(menu);if(focused>=0)logical_selection=focused;}
    }
    if(active)reconcile_list_input(menu);
    if(active&&!create_native_reel(menu))log_line("Native file-picker scrollbar could not be created.");
    if(active&&native_reel){
        reel_tick(native_reel);
        int first=clamp_index(reel_selection(native_reel),0,view_count()-visible_rows);
        if(first!=view_first()){
            if(server_page)page_first=first;else root_first=first;
            layout_scroll(menu);
            logical_selection=clamp_index(logical_selection,first,first+visible_rows-1);
            pending_focus=row_id(logical_selection);
        }
    }
    int request=(int)InterlockedExchange(&scroll_requested,-1),delta=(int)InterlockedExchange(&scroll_delta,0);
    if(active&&(request>=0||delta))scroll_to(menu,request>=0?request:view_first()+delta,FALSE);
}
static void reconcile_list_input(void *menu){
    if(!active_list(menu))return;
    /* gaMenu.enter sets the screen mask recursively after initScreens. It can
     * reenable offscreen children and decorations. Reconcile without resetting
     * live press/release latches on unchanged visible rows. */
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    if(!mask)return; /* Preserve the game's disabled-screen/transition gate. */
    for(int i=0;i<root_count;i++){
        struct RootRow *r=&root_rows[i];BOOL show=!server_page&&i>=root_first&&i<root_first+visible_rows;
        row_visible(r->text,show,mask);row_visible(r->arrow,show,0);
        row_visible(r->line,show&&i<root_first+visible_rows-1&&i<root_count-1,0);
    }
    for(int i=0;i<10;i++){
        BOOL show=server_page&&i>=page_first&&i<page_first+visible_rows;
        row_visible(server_rows[i+1],show,mask);
        row_visible(page_lines[i],show&&i<page_first+visible_rows-1&&i<9,0);
    }
}
static void sync_scroll(void *menu){
    if(!scroll_ready)return;
    int page=server_page?1:(((int *)menu)[62]==0?0:2);
    if(page!=laid_out||active_list(menu)!=laid_active||(native_reel&&native_reel_page!=page))prepare_scroll(menu);
    if(active_list(menu)){
        reconcile_list_input(menu);
        int focused=focused_index(menu);
        if(pending_focus<0&&focused>=0)logical_selection=focused;
        apply_pending_focus(menu);
    }
}
static LRESULT CALLBACK scroll_wndproc(HWND window,UINT message,WPARAM wparam,LPARAM lparam){
    if(scroll_active){
        if(message==WM_MOUSEWHEEL){
            wheel_remainder+=(short)HIWORD(wparam);int steps=wheel_remainder/WHEEL_DELTA;wheel_remainder-=steps*WHEEL_DELTA;
            InterlockedExchangeAdd(&scroll_delta,-steps);return 0;
        }
        if(message==WM_KEYDOWN){
            if(wparam==VK_PRIOR||wparam==VK_NEXT){InterlockedExchangeAdd(&scroll_delta,wparam==VK_NEXT?visible_rows:-visible_rows);return 0;}
            if(wparam==VK_HOME||wparam==VK_END){InterlockedExchange(&scroll_requested,wparam==VK_HOME?0:view_count()-visible_rows);return 0;}
        }
    }
    /* All pointer events, including native scrollbar dragging, are untouched. */
    return scroll_previous?CallWindowProcW(scroll_previous,window,message,wparam,lparam):DefWindowProcW(window,message,wparam,lparam);
}
static void attach_scroll_window(void){
    if(scroll_window)return;Toast target;ZeroMemory(&target,sizeof(target));EnumWindows(find_game,(LPARAM)&target);
    if(!target.game)return;
    scroll_previous=(WNDPROC)GetWindowLongPtrW(target.game,GWLP_WNDPROC);
    LONG_PTR previous=SetWindowLongPtrW(target.game,GWLP_WNDPROC,(LONG_PTR)&scroll_wndproc);
    if(previous){scroll_previous=(WNDPROC)previous;scroll_window=target.game;}
}
static BOOL init_scroll(void *menu,void *sample){
    root_count=0;root_first=page_first=extra_count=0;laid_out=-1;scroll_ready=FALSE;
    ZeroMemory(root_rows,sizeof(root_rows));ZeroMemory(page_lines,sizeof(page_lines));
    ZeroMemory(scroll_extras,sizeof(scroll_extras));
    logical_selection=0;pending_focus=SERVERS_ID;last_direction=0;
    row_x=((float *)sample)[8];row_start=((float *)sample)[9];row_gap=((float *)menu)[66];
    ui_width=row_x*2;ui_height=row_start/0.3f;
    if(row_gap<=0||ui_height<=0)return FALSE;
    visible_rows=clamp_index((int)((ui_height-row_start-server_row_height)/row_gap)+1,1,10);
    line_width=((float *)menu)[14]*1650.0f;
    arrow_dy=((float *)menu)[15]*60.0f;
    line_dy=row_gap*0.5f+((float *)menu)[15]*50.0f;
    void *group=((void **)menu)[67],*screen=((void **)menu)[46];
    root_count=1;
    struct RootRow *added=&root_rows[0];added->id=SERVERS_ID;added->text=server_rows[0];
    added->x=row_x;added->arrow_x=row_x+line_width*0.5f;added->arrow_dy=arrow_dy;
    added->arrow=scroll_sprite(group,FALSE,52,added->arrow_x,row_start+arrow_dy,5);
    if(!added->arrow)return FALSE;scale_item(added->arrow,((float *)menu)[14],((float *)menu)[15]);
    for(int id=0;id<5;id++){
        void *text=((void **)menu)[68+id];if(!readable(text,484))continue;
        struct RootRow *r=&root_rows[root_count++];r->id=id;r->text=text;
        r->x=((float *)text)[8];
        r->arrow=((void **)menu)[73+id];r->line=id<4?((void **)menu)[78+id]:NULL;
        r->arrow_x=r->arrow?((float *)r->arrow)[8]:row_x+line_width*0.5f;
        r->arrow_dy=r->arrow?((float *)r->arrow)[9]-((float *)text)[9]:arrow_dy;
    }
    for(int i=0;i<root_count;i++)if(!root_rows[i].line){
        root_rows[i].line=scroll_sprite(group,FALSE,51,row_x,row_start+line_dy,4);
        if(!root_rows[i].line)return FALSE;
        set_line_size(root_rows[i].line,line_width,((float *)menu)[15]);
    }
    for(int i=0;i<10;i++){
        page_lines[i]=scroll_sprite(screen,TRUE,51,row_x,row_start,4);
        if(!page_lines[i])return FALSE;
        set_line_size(page_lines[i],line_width,((float *)menu)[15]);
    }
    scroll_ready=TRUE;layout_scroll(menu);return TRUE;
}
static void cleanup_scroll(void *menu){
    InterlockedExchange(&scroll_active,0);InterlockedExchange(&scroll_delta,0);InterlockedExchange(&scroll_requested,-1);
    scroll_ready=FALSE;pending_focus=-1;release_native_reel();
    for(int i=0;i<extra_count;i++)if(scroll_extras[i]){
        ((int *)scroll_extras[i])[1]++;remove_screen(((void **)menu)[46],scroll_extras[i]);
    }
    extra_count=0;root_count=0;
}
