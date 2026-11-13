/* Audio's slider: native sprites 51/54, native button/repeat masks, drag latch,
 * rounded/clamped discrete positions. No separate input polling thread. */
#define UI_SCALE_CONTROL_ID 1603
static void *ui_slider_label,*ui_slider_rail,*ui_slider_knob,*ui_slider_min,*ui_slider_max;
static int ui_slider_pending;
static BOOL ui_slider_dragging;
static float ui_slider_left,ui_slider_width,ui_slider_y;
static float ui_slider_rail_pixel_scale(float render_height,float client_height){
    float ratio=client_height>0?render_height/client_height:1;
    if(ratio<1)ratio=1;if(ratio>32)ratio=32;
    int pixels=(int)ratio;return (float)(pixels+(ratio>pixels));
}
static float ui_slider_rail_minimum(void){
    RECT client;float size[2];
    if(display_installed&&ui_actual_resolution(size)&&GetClientRect(display_window(),&client)&&client.bottom>0)
        return ui_slider_rail_pixel_scale(size[1],(float)client.bottom);
    return 1;
}
static void ui_slider_art(void){
    if(!ui_slider_label)return;
    WCHAR text[96];if(ui_slider_pending)_snwprintf(text,96,L"UI SCALE: %d",ui_slider_pending);
    else wcscpy(text,L"UI SCALE: 0 (AUTO)");
    float width=((float *)ui_slider_label)[12],height=((float *)ui_slider_label)[13];
    raw_text(ui_slider_label,text);((float *)ui_slider_label)[12]=width;((float *)ui_slider_label)[13]=height;
    if(ui_slider_rail){
        ui_slider_y=(float)(int)(ui_slider_y+0.5f);
        move_item(ui_slider_rail,((float *)ui_slider_rail)[8],ui_slider_y);
    }
    move_item(ui_slider_knob,ui_slider_left+ui_slider_width*ui_scale_step(ui_slider_pending)/16,ui_slider_y);
}
static void ui_slider_create(void *menu,float x,float y,float right,float gap){
    ui_slider_pending=ui_scale_value;ui_slider_dragging=FALSE;
    ui_slider_left=x;ui_slider_width=right-x;ui_slider_y=y+gap*0.18f;
    ui_slider_label=create_text(L"UI SCALE: 0 (AUTO)",x,y-gap*0.2f,3,0,1,-1,FALSE,-1,FALSE,TRUE);
    float pos[2]={(x+right)*0.5f,ui_slider_y};
    ui_slider_rail=create_sprite(*ui_instance,51,pos,4,-1);ui_slider_knob=create_sprite(*ui_instance,54,pos,4,-1);
    /* Original Audio endpoint placement: 20 design pixels outside the rail,
     * 10 below its center, with outward-facing text alignment. */
    float sx=((float *)menu)[14],sy=((float *)menu)[15];
    ui_slider_min=create_text(L"0",x-20*sx,ui_slider_y+10*sy,3,2,1,608,FALSE,-1,FALSE,TRUE);
    ui_slider_max=create_text(L"100",right+20*sx,ui_slider_y+10*sy,3,0,1,608,FALSE,-1,FALSE,TRUE);
    void *items[5]={ui_slider_label,ui_slider_rail,ui_slider_knob,ui_slider_min,ui_slider_max};
    static const float normal[4]={1,1,1,0.9f};
    for(int i=0;i<5;i++)if(items[i]){
        void *item=items[i];((int *)item)[15]=100;menu_item(item,i==0);enable_input(item,i==0);colour_item(item,normal);
        if(!i){((int *)item)[2]=UI_SCALE_CONTROL_ID;((float *)item)[12]=right-x;((float *)item)[13]=gap;}
        else if(i==1){
            float width=((float *)item)[12],thickness=ui_slider_rail_minimum();
            if(thickness<sy)thickness=sy;
            if(width>0)scale_item(item,(right-x)/width,thickness);
        }
        else if(i==2)scale_item(item,((float *)menu)[14],((float *)menu)[15]);
        ((int *)item)[1]++;add_screen(((void **)menu)[46],item);
    }
    ui_slider_art();
}
static void ui_slider_tick(void *menu){
    if(!ui_slider_label||!ui_font_setup||!readable(input_manager,4)||!readable(*input_manager,4))return;
    if(((int *)menu)[102]!=0||!(*((BYTE *)ui_slider_label+372)&1)||!*(DWORD *)((BYTE *)ui_slider_label+76)){ui_slider_dragging=FALSE;return;}
    void *manager=*input_manager,*device=((void *(__thiscall *)(void *,UINT))(*(void ***)manager)[1])(manager,0);
    if(!readable(device,4))return;
    void **table=*(void ***)device;int step=ui_scale_step(ui_slider_pending);
    if(((int *)menu)[11]==UI_SCALE_CONTROL_ID){
        DWORD repeat=((DWORD (__thiscall *)(void *))table[7])(device);
        if(((BYTE (__thiscall *)(void *,DWORD))table[1])(device,256)||(repeat&256))step--;
        if(((BYTE (__thiscall *)(void *,DWORD))table[1])(device,512)||(repeat&512))step++;
    }
    void *pc=((void *(__thiscall *)(void *))table[21])(device);
    if(readable(pc,4)){
        BOOL down=((BYTE (__thiscall *)(void *,DWORD))(*(void ***)pc)[2])(pc,256);
        float point[2];((PointerPositionFn)table[11])(device,point);
        if(down&&!ui_slider_dragging){
            float half=((float *)ui_slider_label)[13]*0.5f;
            if(point[0]>=ui_slider_left&&point[0]<=ui_slider_left+ui_slider_width&&point[1]>=ui_slider_y-half&&point[1]<=ui_slider_y+half){ui_slider_dragging=TRUE;focus_row(menu,UI_SCALE_CONTROL_ID);}
        }
        if(!down)ui_slider_dragging=FALSE;
        if(ui_slider_dragging)step=(int)((point[0]-ui_slider_left)/ui_slider_width*16+0.5f);
    }
    int value=ui_scale_from_step(step);if(value!=ui_slider_pending){ui_slider_pending=value;ui_slider_art();}
}
static void ui_slider_save(void){
    if(!ui_font_setup)return;
    WCHAR value[16];_snwprintf(value,16,L"%d",ui_slider_pending);
    if(WritePrivateProfileStringW(L"graphics",L"ui_scale",value,config_path)){if(ui_scale_value!=ui_slider_pending)ui_scale_refresh_pending=TRUE;ui_scale_value=ui_slider_pending;ui_font_value=-1;}
    else log_line("Could not save UI scale preference.");
}
