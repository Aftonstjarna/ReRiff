/* Bounded Settings lists. Engine objects/rendering/input remain native; the
 * window callback only queues wheel/drag requests for the menu's own thread. */
#define SCROLL_UP_ID 1601
#define SCROLL_DOWN_ID 1602
#define ROOT_LIMIT 6
#define SCROLL_EXTRA_LIMIT 14
static struct RootRow { void *text,*arrow,*line;int id;DWORD mask;float x,arrow_x,arrow_dy; } root_rows[ROOT_LIMIT];
static int root_count,root_first,page_first,visible_rows,laid_out=-1,laid_first=-1,laid_active=-1;
static float row_x,row_start,row_gap,line_width,line_dy,arrow_dy,ui_width,ui_height;
static void *page_lines[10],*scroll_track,*scroll_thumb,*scroll_buttons[2];
static void *scroll_extras[SCROLL_EXTRA_LIMIT];static int extra_count;
static volatile LONG scroll_active,scroll_delta,scroll_requested=-1;
static int scroll_view_count,scroll_view_first;
static float bar_x,bar_width,bar_top,bar_bottom,thumb_y,thumb_height;
static HWND scroll_window;static WNDPROC scroll_previous;
static BOOL bar_dragging;static float drag_grab;static int wheel_remainder;
static BOOL scroll_ready;

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
static void scrollbar_shortcuts(void *menu){
    if(!scroll_ready)return;
    int first=view_first(),index=focused_index(menu),count=view_count();
    ((DWORD *)scroll_buttons[0])[16]=scroll_active&&first>0&&index==first?64:0;
    ((DWORD *)scroll_buttons[1])[16]=scroll_active&&first+visible_rows<count&&index==first+visible_rows-1?128:0;
}
static void layout_scroll(void *menu){
    if(!scroll_ready)return;
    BOOL active=server_page||(((int *)menu)[62]==0&&*((BYTE *)((void **)menu)[67]+372)&1);
    if(((int *)menu)[102]!=0)active=FALSE;
    int count=view_count(),first=clamp_index(view_first(),0,count>visible_rows?count-visible_rows:0);
    if(server_page)page_first=first;else root_first=first;
    for(int i=0;i<10;i++){
        BOOL show=active&&server_page&&i>=first&&i<first+visible_rows;
        void *row=server_rows[i+1];int action=i+1;BOOL toggle=action==1||action==4||action==5;
        row_visible(row,show,0xffffffffu);
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
            row_visible(r->text,show,r->mask);row_visible(r->arrow,show,0);
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
    BOOL overflow=active&&count>visible_rows;
    row_visible(scroll_track,overflow,0);row_visible(scroll_thumb,overflow,0);
    row_visible(scroll_buttons[0],overflow&&first>0,0xffffffffu);
    row_visible(scroll_buttons[1],overflow&&first+visible_rows<count,0xffffffffu);
    float span=bar_bottom-bar_top;
    thumb_height=span*(float)visible_rows/(float)(count?count:1);
    thumb_y=bar_top+(count>visible_rows?(span-thumb_height)*(float)first/(float)(count-visible_rows):0);
    if(overflow){set_line_size(scroll_thumb,bar_width,thumb_height);move_item(scroll_thumb,bar_x,thumb_y);}
    scroll_view_count=count;scroll_view_first=first;
    InterlockedExchange(&scroll_active,overflow);
    laid_active=active;
    laid_out=server_page?1:(((int *)menu)[62]==0?0:2);laid_first=first;
    scrollbar_shortcuts(menu);
}
static void scroll_to(void *menu,int first,BOOL select_edge){
    int count=view_count(),old=view_first();first=clamp_index(first,0,count>visible_rows?count-visible_rows:0);
    if(first==old)return;
    int focus=focused_index(menu);
    if(server_page)page_first=first;else root_first=first;
    layout_scroll(menu);
    if(select_edge)focus=first>old?first+visible_rows-1:first;
    if(focus>=0)focus_row(menu,row_id(clamp_index(focus,first,first+visible_rows-1)));
}
static void sync_scroll(void *menu){
    if(!scroll_ready)return;
    int page=server_page?1:(((int *)menu)[62]==0?0:2);
    BOOL active=(server_page||(page==0&&(*((BYTE *)((void **)menu)[67]+372)&1)))&&((int *)menu)[102]==0;
    if(page!=laid_out||view_first()!=laid_first||active!=laid_active)layout_scroll(menu);
    /* Original Audio/Graphics returns can refocus a row outside the previous
     * viewport. Reveal it before accepting further input. */
    int focused=focused_index(menu),first=view_first();
    if(page!=2&&focused>=0&&(focused<first||focused>=first+visible_rows))
        scroll_to(menu,focused<first?focused:focused-visible_rows+1,FALSE);
    int request=(int)InterlockedExchange(&scroll_requested,-1);
    int delta=(int)InterlockedExchange(&scroll_delta,0);
    if(scroll_active&&(request>=0||delta))scroll_to(menu,request>=0?request:view_first()+delta,FALSE);
    scrollbar_shortcuts(menu);
}
static void drag_scroll(float y){
    float travel=bar_bottom-bar_top-thumb_height;
    int maximum=scroll_view_count-visible_rows;
    if(travel>0&&maximum>0){
        int index=(int)(((y-bar_top-drag_grab)/travel)*(float)maximum+0.5f);
        InterlockedExchange(&scroll_requested,clamp_index(index,0,maximum));
    }
}
static LRESULT CALLBACK scroll_wndproc(HWND window,UINT message,WPARAM wparam,LPARAM lparam){
    /* No engine calls or object writes on the Windows message thread. */
    if(bar_dragging&&(message==WM_LBUTTONUP||message==WM_CAPTURECHANGED||message==WM_CANCELMODE||!scroll_active)){
        bar_dragging=FALSE;if(GetCapture()==window)ReleaseCapture();
        if(message==WM_LBUTTONUP)return 0;
    }
    if(scroll_active){
        if(message==WM_MOUSEWHEEL){
            wheel_remainder+=(short)HIWORD(wparam);
            int steps=wheel_remainder/WHEEL_DELTA;wheel_remainder-=steps*WHEEL_DELTA;
            InterlockedExchangeAdd(&scroll_delta,-steps);return 0;
        }
        if(message==WM_KEYDOWN){
            if(wparam==VK_PRIOR||wparam==VK_NEXT){InterlockedExchangeAdd(&scroll_delta,wparam==VK_NEXT?visible_rows:-visible_rows);return 0;}
            if(wparam==VK_HOME||wparam==VK_END){InterlockedExchange(&scroll_requested,wparam==VK_HOME?0:scroll_view_count-visible_rows);return 0;}
        }
        if(message==WM_LBUTTONDOWN||message==WM_MOUSEMOVE){
            RECT client;
            if(GetClientRect(window,&client)&&client.right>0&&client.bottom>0){
                float x=(float)(short)LOWORD(lparam)*ui_width/(float)client.right;
                float y=(float)(short)HIWORD(lparam)*ui_height/(float)client.bottom;
                if(message==WM_LBUTTONDOWN&&x>=bar_x-bar_width*1.5f&&x<=bar_x+bar_width*1.5f&&y>=bar_top&&y<=bar_bottom){
                    bar_dragging=TRUE;drag_grab=y>=thumb_y&&y<=thumb_y+thumb_height?y-thumb_y:thumb_height*0.5f;
                    SetCapture(window);drag_scroll(y);return 0;
                }
                if(message==WM_MOUSEMOVE&&bar_dragging){drag_scroll(y);return 0;}
            }
        }
    }
    return CallWindowProcW(scroll_previous,window,message,wparam,lparam);
}
static void attach_scroll_window(void){
    if(scroll_window)return;
    Toast target;ZeroMemory(&target,sizeof(target));EnumWindows(find_game,(LPARAM)&target);
    if(!target.game)return;
    SetLastError(0);LONG_PTR previous=SetWindowLongPtrW(target.game,GWLP_WNDPROC,(LONG_PTR)&scroll_wndproc);
    if(previous){scroll_previous=(WNDPROC)previous;scroll_window=target.game;}
}
static BOOL init_scroll(void *menu,void *sample){
    root_count=0;root_first=page_first=extra_count=0;laid_out=-1;scroll_ready=FALSE;
    ZeroMemory(root_rows,sizeof(root_rows));ZeroMemory(page_lines,sizeof(page_lines));
    ZeroMemory(scroll_extras,sizeof(scroll_extras));ZeroMemory(scroll_buttons,sizeof(scroll_buttons));
    row_x=((float *)sample)[8];row_start=((float *)sample)[9];row_gap=((float *)menu)[66];
    ui_width=row_x*2;ui_height=row_start/0.3f;
    if(row_gap<=0||ui_height<=0)return FALSE;
    visible_rows=clamp_index((int)((ui_height-row_start-server_row_height)/row_gap)+1,1,10);
    line_width=((float *)menu)[14]*1650.0f;
    arrow_dy=((float *)menu)[15]*60.0f;
    line_dy=row_gap*0.5f+((float *)menu)[15]*50.0f;
    void *group=((void **)menu)[67],*screen=((void **)menu)[46];
    for(int id=0;id<5;id++){
        void *text=((void **)menu)[68+id];if(!readable(text,484))continue;
        struct RootRow *r=&root_rows[root_count++];r->id=id;r->text=text;
        r->x=((float *)text)[8];r->mask=*(DWORD *)((BYTE *)text+76);
        r->arrow=((void **)menu)[73+id];r->line=id<4?((void **)menu)[78+id]:NULL;
        r->arrow_x=r->arrow?((float *)r->arrow)[8]:row_x+line_width*0.5f;
        r->arrow_dy=r->arrow?((float *)r->arrow)[9]-((float *)text)[9]:arrow_dy;
    }
    struct RootRow *added=&root_rows[root_count++];added->id=SERVERS_ID;added->text=server_rows[0];
    added->x=row_x;added->mask=0xffffffffu;added->arrow_x=row_x+line_width*0.5f;added->arrow_dy=arrow_dy;
    added->arrow=scroll_sprite(group,FALSE,52,added->arrow_x,row_start+arrow_dy,5);
    if(!added->arrow)return FALSE;scale_item(added->arrow,((float *)menu)[14],((float *)menu)[15]);
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
    bar_width=((float *)menu)[14]*22.0f;if(bar_width<10)bar_width=10;
    bar_x=ui_width*0.92f;bar_top=row_start+arrow_dy;
    bar_bottom=row_start+(visible_rows-1)*row_gap+arrow_dy;
    /* Include a row-height allowance when only one row fits. */
    if(bar_bottom-bar_top<server_row_height)bar_bottom=bar_top+server_row_height;
    scroll_track=scroll_sprite(screen,TRUE,51,bar_x,bar_top,1);
    scroll_thumb=scroll_sprite(screen,TRUE,51,bar_x,bar_top,1);
    if(!scroll_track||!scroll_thumb)return FALSE;
    set_line_size(scroll_track,bar_width,bar_bottom-bar_top);
    static const float track_colour[4]={1,1,1,0.2f};colour_item(scroll_track,track_colour);
    ((int *)scroll_thumb)[15]=101;
    for(int i=0;i<2;i++){
        void *button=create_text(i?L"v":L"^",bar_x,i?bar_bottom+server_row_height*0.6f:bar_top-server_row_height*0.6f,3,1,1,-1,FALSE,-1,FALSE,TRUE);
        if(!button)return FALSE;
        scroll_buttons[i]=button;scroll_extras[extra_count++]=button;
        ((int *)button)[2]=i?SCROLL_DOWN_ID:SCROLL_UP_ID;((int *)button)[15]=100;
        menu_item(button,TRUE);show_item(button,FALSE);
        ((float *)button)[12]=bar_width*4;((float *)button)[13]=server_row_height*0.6f;
        ((int *)button)[1]++;add_screen(screen,button);
    }
    scroll_ready=TRUE;layout_scroll(menu);return TRUE;
}
static void cleanup_scroll(void *menu){
    InterlockedExchange(&scroll_active,0);InterlockedExchange(&scroll_delta,0);InterlockedExchange(&scroll_requested,-1);
    scroll_ready=FALSE;
    for(int i=0;i<extra_count;i++)if(scroll_extras[i]){
        ((int *)scroll_extras[i])[1]++;remove_screen(((void **)menu)[46],scroll_extras[i]);
    }
    extra_count=0;root_count=0;
}
