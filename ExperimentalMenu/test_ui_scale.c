#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static float actual_width=3840,actual_height=2160;
static BYTE graphics_object[16];
static void *fake_graphics=graphics_object;
static float *__thiscall resolution_mock(void *graphics,float *result,int screen){if(graphics!=fake_graphics||screen)abort();result[0]=actual_width;result[1]=actual_height;return result;}
static void *input_device,*input_pc;static DWORD repeat_mask;static BOOL mouse_down;static float mouse_point[2];
static void *__thiscall get_device_mock(void *self,UINT index){(void)self;(void)index;return input_device;}
static void *__thiscall get_pc_mock(void *self){(void)self;return input_pc;}
static BYTE __thiscall pressed_mock(void *self,DWORD mask){(void)self;(void)mask;return FALSE;}
static DWORD __thiscall repeat_mock(void *self){(void)self;return repeat_mask;}
static BYTE __thiscall mouse_mock(void *self,DWORD mask){(void)self;(void)mask;return mouse_down;}
static float *__thiscall point_mock(void *self,float *value){(void)self;memcpy(value,mouse_point,sizeof(mouse_point));return value;}
static void __thiscall raw_mock(void *self,const WCHAR *value){(void)self;(void)value;}
static void __thiscall position_mock(void *self,const float *point){((float *)self)[8]=point[0];((float *)self)[9]=point[1];}
static void __thiscall focus_mock(void *self,int id){(void)self;(void)id;}
static float observed_factor;
static void __cdecl font_mock(void *self){(void)self;observed_factor=ui_resolution_ratio[0];if(ui_resolution_ratio[1]!=observed_factor)abort();}
static int resize_calls,rescale_calls;
static void __cdecl resize_mock(void *g){(void)g;resize_calls++;}
static void __cdecl rescale_mock(void *g){(void)g;rescale_calls++;}
int main(void){
    const int values[5]={0,25,50,75,100};const float factors[5]={1,.5f,1,1.5f,2};
    float ratios[2]={1,1},reference[2]={2048,1536};void *game_ui=graphics_object;
    ui_resolution_ratio=ratios;ui_menu_reference=reference;ui_game_ui=&game_ui;ui_font_setup=font_mock;
    ui_resolution_original=resolution_mock;ui_graphics_instance=&fake_graphics;
    for(int i=0;i<5;i++){
        ui_scale_value=values[i];float factor=ui_design_factor(values[i]);CHECK(factor==factors[i]);
        ui_layout_reference(factor);ui_setup_fonts(factor);
        CHECK(observed_factor==(values[i]?factor*0.5f:factor)&&ratios[0]==1&&ratios[1]==1);
        CHECK(reference[0]==(values[i]?3840/((1080.f/1536.f)*factor):2048)&&reference[1]==(values[i]?2160/((1080.f/1536.f)*factor):1536));
        float size[2];CHECK(ui_actual_resolution(size)&&size[0]==3840&&size[1]==2160);
    }
    ui_scale_value=0;ui_layout_reference(1);CHECK(reference[0]==2048&&reference[1]==1536);
    graphics_game=graphics_states=&game_ui;graphics_resize=resize_mock;graphics_rescale=rescale_mock;graphics_bridge_ready=TRUE;
    ui_scale_value=75;actual_width=2560;actual_height=1440;ui_scale_refresh_pending=TRUE;ui_refresh_scale();
    CHECK(resize_calls==1&&rescale_calls==1&&!ui_scale_refresh_pending&&observed_factor==1.125f);
    CHECK(actual_width==2560&&actual_height==1440&&ratios[0]==1&&ratios[1]==1);
    ui_refresh_scale();CHECK(resize_calls==1&&rescale_calls==1);
    /* Returning to Auto must undo the manual reference and font override,
     * including after startup changes the render resolution. */
    ui_scale_value=0;ui_scale_refresh_pending=TRUE;ui_refresh_scale();
    CHECK(resize_calls==2&&rescale_calls==2&&observed_factor==1);
    CHECK(reference[0]==2048&&reference[1]==1536&&ratios[0]==1&&ratios[1]==1);
    const float auto_heights[]={720,864,1080,1440,2160};
    for(unsigned i=0;i<sizeof(auto_heights)/sizeof(auto_heights[0]);i++){
        actual_height=auto_heights[i];actual_width=actual_height*16/9;
        ui_layout_reference(1);ui_setup_fonts(1);
        CHECK(observed_factor==1&&reference[0]==2048&&reference[1]==1536);
        CHECK(ratios[0]==1&&ratios[1]==1);
    }
    ui_scale_value=0;
    CHECK(ui_slider_rail_pixel_scale(2160,1080)==2);
    CHECK(ui_slider_rail_pixel_scale(2160,2160)==1);
    CHECK(ui_slider_rail_pixel_scale(1440,1080)==2);
    CHECK(ui_slider_rail_pixel_scale(720,1080)==1);
    CHECK(ui_slider_rail_pixel_scale(2160,0)==1);
    for(int step=0;step<=16;step++)CHECK(ui_scale_step(ui_scale_from_step(step))==step);
    CHECK(ui_scale_from_step(-1)==0&&ui_scale_from_step(100)==100);
    CHECK(ui_scale_clamp(0)==0&&ui_scale_clamp(-1)==25&&ui_scale_clamp(101)==100);
    ui_scope_tls=TlsAlloc();CHECK(ui_scope_tls!=TLS_OUT_OF_INDEXES);
    UiCanvas canvas={3840,2160};CHECK(!ui_scope_set(&canvas));
    float point[2]={123,456};ui_scale_map_pointer(point);CHECK(point[0]==123&&point[1]==456);
    CHECK(ui_scope_set(NULL)==&canvas);
    BYTE menu[512]={0};
    BYTE label[404]={0},knob[404]={0},rail[404]={0};void *manager_table[2]={0},*device_table[24]={0},*pc_table[3]={0};
    void **manager=manager_table,**device=device_table,**pc=pc_table;void *manager_value=&manager;
    manager_table[1]=(void *)get_device_mock;device_table[1]=(void *)pressed_mock;device_table[7]=(void *)repeat_mock;
    device_table[11]=(void *)point_mock;device_table[21]=(void *)get_pc_mock;pc_table[2]=(void *)mouse_mock;
    input_manager=&manager_value;input_device=&device;input_pc=&pc;
    raw_text=raw_mock;sprite_position=position_mock;set_focus=focus_mock;ui_font_setup=font_mock;
    ui_slider_label=label;ui_slider_knob=knob;ui_slider_rail=rail;ui_slider_left=100;ui_slider_width=400;ui_slider_y=300.4f;
    ui_slider_art();CHECK(ui_slider_y==300&&((float *)rail)[9]==300&&((float *)knob)[9]==300);
    ((float *)label)[12]=400;((float *)label)[13]=80;label[372]=1;*(DWORD *)(label+76)=1;
    ((int *)menu)[11]=UI_SCALE_CONTROL_ID;((int *)menu)[102]=0;ui_slider_pending=0;
    repeat_mask=512;ui_slider_tick(menu);CHECK(ui_slider_pending==25&&((float *)knob)[8]==125);
    CHECK(((int *)menu)[11]==UI_SCALE_CONTROL_ID);
    repeat_mask=256;ui_slider_tick(menu);CHECK(ui_slider_pending==0&&((int *)menu)[11]==UI_SCALE_CONTROL_ID);
    repeat_mask=512;ui_slider_tick(menu);CHECK(ui_slider_pending==25&&((int *)menu)[11]==UI_SCALE_CONTROL_ID);
    ui_slider_tick(menu);CHECK(ui_slider_pending==30);
    repeat_mask=0;mouse_down=TRUE;mouse_point[0]=500;mouse_point[1]=300;
    ui_slider_tick(menu);CHECK(ui_slider_pending==100&&ui_slider_dragging&&((float *)knob)[8]==500);
    mouse_point[0]=80;ui_slider_tick(menu);CHECK(ui_slider_pending==0&&((float *)knob)[8]==100);
    mouse_down=FALSE;ui_slider_tick(menu);CHECK(!ui_slider_dragging);
    ((int *)menu)[102]=1;repeat_mask=512;ui_slider_tick(menu);CHECK(ui_slider_pending==0);
    ((int *)menu)[102]=0;label[372]=0;ui_slider_tick(menu);CHECK(ui_slider_pending==0);
    TlsFree(ui_scope_tls);ui_scope_tls=TLS_OUT_OF_INDEXES;
    puts("PASS: native font/menu factors, physical canvas and pointer preservation, Auto reference restore, slider repeat/drag/bounds/transition guards");return 0;
}
