/* Main-menu rows keep their native children, hit tests and direction handler.
 * The original reel handles scrolling when manual-size rows exceed the canvas. */
static void *main_scale_owner,*main_scale_reel;
static int main_scale_count,main_scale_first,main_scale_visible;
static float main_scale_top,main_scale_gap;
static volatile LONG main_scale_active,main_scale_wheel;
static struct {void *item;void **table;void *direction[49];float y,sx,sy;} main_scale_rows[11];
static void main_scale_layout(void *menu){
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    for(int i=0;i<main_scale_count;i++){
        void *row=main_scale_rows[i].item;
        move_item(row,((float *)row)[8],main_scale_top+(i-main_scale_first)*main_scale_gap);
        row_visible(row,i>=main_scale_first&&i<main_scale_first+main_scale_visible,mask);
    }
}
static void main_scale_to(void *menu,int first){
    first=clamp_index(first,0,main_scale_count-main_scale_visible);if(first==main_scale_first)return;
    main_scale_first=first;if(main_scale_reel){float instant=0;reel_move(main_scale_reel,first,&instant);}main_scale_layout(menu);
}
static void __thiscall main_scale_direction(void *item,DWORD key,unsigned device){
    for(int i=0;i<main_scale_count;i++)if(main_scale_rows[i].item==item){
        if(main_scale_owner&&(key==64||key==128)){
            int id=((int *)item)[key==64?39:40];
            for(int j=0;j<main_scale_count;j++)if(((int *)main_scale_rows[j].item)[2]==id){
                if(j<main_scale_first)main_scale_to(main_scale_owner,j);
                else if(j>=main_scale_first+main_scale_visible)main_scale_to(main_scale_owner,j-main_scale_visible+1);
                break;
            }
        }
        ((DirectionFn)main_scale_rows[i].table[28])(item,key,device);return;
    }
}
static void main_scale_cleanup(void){
    InterlockedExchange(&main_scale_active,0);InterlockedExchange(&main_scale_wheel,0);
    for(int i=0;i<main_scale_count;i++)if(main_scale_rows[i].table)*(void ***)main_scale_rows[i].item=main_scale_rows[i].table;
    if(main_scale_reel){void *old=main_scale_reel;main_scale_reel=NULL;((int *)old)[1]--;((void *(__thiscall *)(void *,unsigned))(*(void ***)old)[0])(old,1);}
    main_scale_owner=NULL;main_scale_count=0;
}
static void main_scale_size(void *menu){
    main_scale_owner=NULL;main_scale_count=0;main_scale_first=0;
    if(!ui_scale_value||!bridge_ready)return;
    float size[2];if(!ui_actual_resolution(size)||size[1]<=0)return;
    float available=size[1]-220*((float *)menu)[15];if(available<=0)return;
    /* gaMainMenu fits its text to height minus 170+50 native menu units. */
    float factor=ui_design_factor(ui_scale_value)*(1080-220*(1080.f/1536.f))/available,first=0,last=0,gap=0;
    for(int i=0;i<11;i++){
        void *row=((void **)menu)[62+i];if(!readable(row,404))continue;
        float y=((float *)row)[9];if(!main_scale_count)first=y;else if(!gap&&y>last)gap=y-last;last=y;
        main_scale_rows[main_scale_count].item=row;main_scale_rows[main_scale_count].table=NULL;main_scale_rows[main_scale_count].y=y;main_scale_rows[main_scale_count].sx=((float *)row)[43];main_scale_rows[main_scale_count].sy=((float *)row)[44];main_scale_count++;
    }
    if(main_scale_count<2||gap<=0){main_scale_count=0;return;}
    main_scale_gap=gap*factor;
    float top=170*((float *)menu)[15]+main_scale_gap*0.5f,bottom=size[1]-50*((float *)menu)[15];
    main_scale_visible=clamp_index((int)((bottom-top)/main_scale_gap)+1,1,main_scale_count);
    main_scale_top=(first+last-(main_scale_count-1)*main_scale_gap)*0.5f;
    if(main_scale_top<top)main_scale_top=top;
    if(main_scale_top+(main_scale_visible-1)*main_scale_gap>bottom)main_scale_top=top;
    main_scale_owner=menu;
    for(int i=0;i<main_scale_count;i++){
        void *row=main_scale_rows[i].item;scale_item(row,((float *)row)[43]*factor,((float *)row)[44]*factor);
    }
    main_scale_layout(menu);
}
static void main_scale_scroll_init(void *menu){
    if(main_scale_owner!=menu||main_scale_visible==main_scale_count)return;
    void *storage=game_allocate(532);if(storage)main_scale_reel=reel_ctor(storage,menu,main_scale_gap,1);
    if(!main_scale_reel){log_line("Main-menu reel allocation failed; original sizing retained.");for(int i=0;i<main_scale_count;i++){void *row=main_scale_rows[i].item;scale_item(row,main_scale_rows[i].sx,main_scale_rows[i].sy);move_item(row,((float *)row)[8],main_scale_rows[i].y);row_visible(row,TRUE,*(DWORD *)((BYTE *)((void **)menu)[46]+40));}main_scale_count=0;main_scale_owner=NULL;return;}
    ((int *)main_scale_reel)[1]++;
    for(int i=0;i<main_scale_count;i++){
        void *spacer=create_text(L"",0,i*main_scale_gap,3,1,0,-1,FALSE,-1,FALSE,TRUE);
        if(spacer){menu_item(spacer,FALSE);enable_input(spacer,FALSE);show_item(spacer,FALSE);((int *)spacer)[1]++;reel_add(main_scale_reel,spacer,NULL);}
        void *row=main_scale_rows[i].item;void **table=*(void ***)row;
        if(!readable(table-1,sizeof(main_scale_rows[i].direction))||!table[28])continue;
        main_scale_rows[i].table=table;memcpy(main_scale_rows[i].direction,table-1,sizeof(main_scale_rows[i].direction));main_scale_rows[i].direction[29]=(void *)main_scale_direction;*(void ***)row=main_scale_rows[i].direction+1;
    }
    reel_limit(main_scale_reel,main_scale_visible);float size[2];if(ui_actual_resolution(size))reel_bar(main_scale_reel,size[0]*0.92f,main_scale_top-main_scale_gap*0.45f,main_scale_top+(main_scale_visible-0.55f)*main_scale_gap,1,NULL);
    attach_scroll_window();
}
static void main_scale_reveal(void *menu){
    if(main_scale_owner!=menu)return;
    int focused=((int *)menu)[11];
    for(int i=0;i<main_scale_count;i++)if(((int *)main_scale_rows[i].item)[2]==focused){if(i<main_scale_first)main_scale_to(menu,i);else if(i>=main_scale_first+main_scale_visible)main_scale_to(menu,i-main_scale_visible+1);break;}
    main_scale_layout(menu);
}
static void main_scale_tick(void *menu){
    if(main_scale_owner!=menu)return;
    BOOL active=((int *)menu)[102]==0&&*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    InterlockedExchange(&main_scale_active,active&&main_scale_reel!=NULL);
    int before=main_scale_first;
    if(active&&main_scale_reel){reel_tick(main_scale_reel);main_scale_to(menu,reel_selection(main_scale_reel));}
    int delta=(int)InterlockedExchange(&main_scale_wheel,0);if(active&&delta)main_scale_to(menu,main_scale_first+delta);
    if(active&&before!=main_scale_first){
        int focused=((int *)menu)[11];
        for(int i=0;i<main_scale_count;i++)if(((int *)main_scale_rows[i].item)[2]==focused&&(i<main_scale_first||i>=main_scale_first+main_scale_visible)){
            int target=clamp_index(i,main_scale_first,main_scale_first+main_scale_visible-1),id=((int *)main_scale_rows[target].item)[2];
            focus_row(menu,id);((InputFn)(*(void ***)menu)[25])(menu,id);break;
        }
    }
    main_scale_layout(menu);
}
static BOOL main_scroll_wheel(int delta){
    if(!InterlockedCompareExchange(&main_scale_active,0,0))return FALSE;
    InterlockedExchangeAdd(&main_scale_wheel,delta);return TRUE;
}
