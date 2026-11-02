/* Same native file-picker reel and reveal-before-navigation adapter as Settings. */
static void *graphics_reel;
static int graphics_first,graphics_visible;
static float graphics_gap;
static volatile LONG graphics_scroll_active,graphics_wheel;
static struct {void *item;float x,y;int row;BOOL input,mode_label;int mode_index;} graphics_parts[280];
static unsigned graphics_part_count;
static struct {void *item;void **original;void *table[49];} graphics_directions[9];
static int graphics_row(int id){
    if(id==1||id==2||id==RESOLUTION_FOCUS_ID)return 0;
    if(id==BORDERLESS_CONTROL_ID||id==DISPLAY_LEFT_ID||id==DISPLAY_RIGHT_ID)return 1;if(id==AA_CONTROL_ID)return 2;if(id==UI_SCALE_CONTROL_ID)return 3;return -1;
}
static int graphics_id(int row){static const int ids[4]={RESOLUTION_FOCUS_ID,BORDERLESS_CONTROL_ID,AA_CONTROL_ID,UI_SCALE_CONTROL_ID};return ids[clamp_index(row,0,3)];}
static void graphics_layout(void *menu){
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    for(unsigned i=0;i<graphics_part_count;i++){
        void *item=graphics_parts[i].item;int row=graphics_parts[i].row;
        BOOL visible=row>=graphics_first&&row<graphics_first+graphics_visible;
        move_item(item,graphics_parts[i].x,graphics_parts[i].y-graphics_first*graphics_gap);
        if(graphics_parts[i].mode_label)visible=visible&&graphics_parts[i].mode_index==((int *)menu)[75];
        row_visible(item,visible,graphics_parts[i].input?mask:0);
    }
    row_visible(((void **)menu)[62],FALSE,0);row_visible(((void **)menu)[63],FALSE,0);row_visible(((void **)menu)[73],FALSE,0);
    if(ui_slider_rail)ui_slider_y=((float *)ui_slider_rail)[9];
    ui_slider_art();
    /* Like the dynamic volume-style slider thumb, AA must follow its bar
     * after static row positions are restored by the reel layout. */
    aa_position_knob(menu);
}
static void graphics_scroll_to(void *menu,int first,BOOL follow){
    first=clamp_index(first,0,4-graphics_visible);
    if(first==graphics_first)return;
    graphics_first=first;if(graphics_reel){float instant=0;reel_move(graphics_reel,first,&instant);}
    graphics_layout(menu);
    if(follow){int row=graphics_row(((int *)menu)[11]);if(row>=0&&(row<first||row>=first+graphics_visible)){
        int id=graphics_id(clamp_index(row,first,first+graphics_visible-1));focus_row(menu,id);((InputFn)(*(void ***)menu)[25])(menu,id);
    }}
}
static void __thiscall graphics_scroll_direction(void *item,DWORD key,unsigned device){
    for(unsigned i=0;i<9;i++)if(graphics_directions[i].item==item){
        if(aa_menu_owner&&(key==64||key==128)){
            int row=graphics_row(((int *)item)[key==64?39:40]),first=graphics_first;
            if(row>=0){if(row<first)first=row;if(row>=first+graphics_visible)first=row-graphics_visible+1;graphics_scroll_to(aa_menu_owner,first,FALSE);}
        }
        ((DirectionFn)graphics_directions[i].original[28])(item,key,device);return;
    }
}
static void graphics_scroll_part(void *item,int row,BOOL input,BOOL mode){
    if(!readable(item,404)||graphics_part_count>=280)return;
    unsigned i=graphics_part_count++;graphics_parts[i].item=item;graphics_parts[i].x=((float *)item)[8];graphics_parts[i].y=((float *)item)[9];graphics_parts[i].row=row;graphics_parts[i].input=input;graphics_parts[i].mode_label=mode;graphics_parts[i].mode_index=-1;
}
static void graphics_scroll_cleanup(void){
    InterlockedExchange(&graphics_scroll_active,0);InterlockedExchange(&graphics_wheel,0);
    for(unsigned i=0;i<9;i++)if(graphics_directions[i].item){*(void ***)graphics_directions[i].item=graphics_directions[i].original;graphics_directions[i].item=NULL;}
    if(graphics_reel){void *old=graphics_reel;graphics_reel=NULL;((int *)old)[1]--;((void *(__thiscall *)(void *,unsigned))(*(void ***)old)[0])(old,1);}
    graphics_part_count=0;
}
static void graphics_scroll_init(void *menu,float gap){
    graphics_first=0;graphics_gap=gap;graphics_part_count=0;graphics_visible=4;
    float size[2];UiCanvas *canvas=ui_scope();
    if(canvas){size[0]=canvas->width;size[1]=canvas->height;}else if(!ui_actual_resolution(size)){size[0]=1920;size[1]=1080;}
    float top=((float *)((void **)menu)[64])[9],bottom=size[1]-100*((float *)menu)[15];
    graphics_visible=clamp_index((int)((bottom-top)/gap+0.5f),1,4);
    graphics_scroll_part(((void **)menu)[64],0,TRUE,FALSE);graphics_scroll_part(((void **)menu)[65],0,TRUE,FALSE);graphics_scroll_part(resolution_hover,0,TRUE,FALSE);
    void *extras[9]={borderless_label,borderless_bar,borderless_knob,aa_label,aa_bar,aa_knob,ui_slider_label,ui_slider_rail,ui_slider_knob};
    for(int i=0;i<9;i++)graphics_scroll_part(extras[i],1+i/3,i<3||i%3==0,FALSE);
    graphics_scroll_part(ui_slider_min,3,FALSE,FALSE);graphics_scroll_part(ui_slider_max,3,FALSE,FALSE);
    void **begin=((void ***)menu)[70],**end=((void ***)menu)[71];
    if(begin&&end&&(uintptr_t)end>=(uintptr_t)begin&&(uintptr_t)end-(uintptr_t)begin<256*sizeof(void *)&&readable(begin,(uintptr_t)end-(uintptr_t)begin))
        for(void **it=begin;it!=end;it++){unsigned index=graphics_part_count;graphics_scroll_part(*it,0,FALSE,TRUE);if(graphics_part_count>index)graphics_parts[index].mode_index=(int)(it-begin);}
    if(graphics_visible==4){graphics_layout(menu);return;}
    void *items[9]={borderless_label,borderless_bar,((void **)menu)[66],((void **)menu)[64],((void **)menu)[65],borderless_knob,aa_label,ui_slider_label,resolution_hover};
    for(int i=0;i<9;i++)if(items[i]){
        void **table=*(void ***)items[i];if(!readable(table-1,sizeof(graphics_directions[i].table))||!table[28])continue;
        graphics_directions[i].item=items[i];graphics_directions[i].original=table;
        memcpy(graphics_directions[i].table,table-1,sizeof(graphics_directions[i].table));graphics_directions[i].table[29]=(void *)graphics_scroll_direction;*(void ***)items[i]=graphics_directions[i].table+1;
    }
    void *storage=game_allocate(532);if(storage)graphics_reel=reel_ctor(storage,menu,gap,1);
    if(graphics_reel){
        ((int *)graphics_reel)[1]++;
        for(int i=0;i<4;i++){void *spacer=create_text(L"",0,i*gap,3,1,0,-1,FALSE,-1,FALSE,TRUE);if(spacer){menu_item(spacer,FALSE);enable_input(spacer,FALSE);show_item(spacer,FALSE);((int *)spacer)[1]++;reel_add(graphics_reel,spacer,NULL);}}
        reel_limit(graphics_reel,graphics_visible);reel_bar(graphics_reel,size[0]*0.92f,top-gap*0.45f,top+(graphics_visible-0.55f)*gap,1,NULL);
    }
    InterlockedExchange(&graphics_scroll_active,1);attach_scroll_window();graphics_layout(menu);
}
static void graphics_scroll_tick(void *menu){
    if(!graphics_part_count)return;
    InterlockedExchange(&graphics_scroll_active,graphics_reel&&((int *)menu)[102]==0&&*(DWORD *)((BYTE *)((void **)menu)[46]+40));
    if(graphics_reel){reel_tick(graphics_reel);graphics_scroll_to(menu,reel_selection(graphics_reel),TRUE);}
    int delta=(int)InterlockedExchange(&graphics_wheel,0);if(delta)graphics_scroll_to(menu,graphics_first+delta,TRUE);
    graphics_layout(menu);
}
static BOOL graphics_scroll_wheel(int delta){
    if(!InterlockedCompareExchange(&graphics_scroll_active,0,0))return FALSE;
    InterlockedExchangeAdd(&graphics_wheel,delta);return TRUE;
}
