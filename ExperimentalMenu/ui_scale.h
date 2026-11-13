/* Extend native font/menu sizing. Screen coordinates, rendering and hit tests
 * stay on the original physical canvas; Auto restores the original constants. */
#include "ui_scale_tables.h"
typedef float *(__thiscall *UiResolutionFn)(void *,float *,int);
static UiResolutionFn ui_resolution_original;
static void **ui_graphics_instance,**ui_game_ui;
static GraphicsBridgeFn ui_font_setup;
static int ui_scale_value,ui_font_value=-1;
static BOOL ui_scale_refresh_pending;
static DWORD ui_scope_tls=TLS_OUT_OF_INDEXES;
typedef struct UiCanvas {float width,height;} UiCanvas;
static UiCanvas *ui_scope(void){return ui_scope_tls==TLS_OUT_OF_INDEXES?NULL:TlsGetValue(ui_scope_tls);}
static UiCanvas *ui_scope_set(UiCanvas *canvas){UiCanvas *old=ui_scope();if(ui_scope_tls!=TLS_OUT_OF_INDEXES)TlsSetValue(ui_scope_tls,canvas);return old;}
static int ui_scale_clamp(int value){return value==0?0:value<25?25:value>100?100:((value+2)/5)*5;}
static int ui_scale_step(int value){return value?1+(ui_scale_clamp(value)-25)/5:0;}
static int ui_scale_from_step(int step){return step<=0?0:step>=16?100:20+step*5;}
static float ui_design_factor(int value){return value?ui_scale_clamp(value)/50.0f:1;}
static BOOL ui_actual_resolution(float *size){
    if(!ui_resolution_original||!readable(ui_graphics_instance,4)||!readable(*ui_graphics_instance,4))return FALSE;
    ui_resolution_original(*ui_graphics_instance,size,0);return size[0]>0&&size[1]>0;
}
/* No extra pointer transform: native UI layout and rendering share this canvas. */
static void ui_scale_map_pointer(float *position){(void)position;}
static PointerPositionFn ui_pc_pointer_original;
static float *__thiscall ui_pc_pointer(void *pointer,float *position){
    float *result=ui_pc_pointer_original(pointer,position);display_map_pointer(pointer,result);return result;
}
typedef struct UiMenuHook {void **table;TickFn tick;ScreenFn init,cleanup;UiCanvas canvas;} UiMenuHook;
static UiMenuHook ui_menus[sizeof(ui_menu_tables)/sizeof(ui_menu_tables[0])];
static UiMenuHook *ui_menu(void *menu){
    void **table=*(void ***)menu;for(unsigned i=0;i<sizeof(ui_menus)/sizeof(ui_menus[0]);i++)if(ui_menus[i].table==table)return &ui_menus[i];return NULL;
}
static float *ui_resolution_ratio,*ui_menu_reference;
static void ui_layout_reference(float factor){
    if(!ui_resolution_ratio||!ui_menu_reference||factor<=0)return;
    /* gaMenu.setupMenuScale already sizes artwork and spacing from the real
     * resolution. Change its design reference, never its layout coordinates. */
    ui_resolution_ratio[0]=ui_resolution_ratio[1]=1;
    DWORD old,ignored;if(VirtualProtect(ui_menu_reference,8,PAGE_READWRITE,&old)){
        float actual[2];
        if(ui_scale_value&&ui_actual_resolution(actual)){
            float scale=(1080.f/1536.f)*factor;
            ui_menu_reference[0]=actual[0]/scale;ui_menu_reference[1]=actual[1]/scale;
        }else{ui_menu_reference[0]=2048;ui_menu_reference[1]=1536;}
        VirtualProtect(ui_menu_reference,8,old,&ignored);
    }
}
static void ui_setup_fonts(float factor){
    if(!ui_font_setup||!ui_resolution_ratio||!readable(ui_game_ui,4)||!*ui_game_ui)return;
    /* Original gaUI.setupFontScale owns atlas selection and font metrics.
     * Only that call sees the size factor. Backgrounds and layout always see
     * native resolution, and the original input functions need no remapping. */
    float actual[2];
    float font_factor=ui_scale_value&&ui_actual_resolution(actual)?factor*1080/actual[1]:factor;
    ui_resolution_ratio[0]=ui_resolution_ratio[1]=font_factor;
    ui_font_setup(*ui_game_ui);
    ui_resolution_ratio[0]=ui_resolution_ratio[1]=1;ui_font_value=ui_scale_value;
}
static void __thiscall ui_menu_init(void *menu,void *screen,void *lower){
    UiMenuHook *hook=ui_menu(menu);if(!hook)return;
    float actual[2];BOOL manual=ui_font_setup&&ui_scale_value&&hook->table!=(void **)((BYTE *)GetModuleHandleW(NULL)+0x7a7af0);
    hook->canvas=manual&&ui_actual_resolution(actual)?(UiCanvas){actual[0],actual[1]}:(UiCanvas){0};
    float factor=hook->canvas.width?ui_design_factor(ui_scale_value):1;
    ui_layout_reference(factor);ui_setup_fonts(factor);
    ((TickFn)hook->table[9])(menu); /* Original gaMenu.setupMenuScale. */
    UiCanvas *previous=ui_scope_set(hook->canvas.width?&hook->canvas:NULL);
    hook->init(menu,screen,lower);ui_scope_set(previous);
    if(hook->canvas.width){char trace[160];snprintf(trace,sizeof(trace),"UI scale: value=%d native font/menu factor=%.2f canvas=%.0fx%.0f.",ui_scale_value,factor,actual[0],actual[1]);log_line(trace);}
}
static void ui_refresh_scale(void){
    if(!ui_scale_refresh_pending)return;ui_scale_refresh_pending=FALSE;
    float factor=ui_design_factor(ui_scale_value);ui_layout_reference(factor);
    /* Use the same original menu/viewport notification as display changes,
     * without resetting the renderer or changing the physical resolution. */
    if(graphics_bridge_ready)graphics_notify_resize();
    ui_layout_reference(factor);ui_setup_fonts(factor);
}
static void __thiscall ui_menu_tick(void *menu){
    UiMenuHook *hook=ui_menu(menu);if(!hook)return;
    UiCanvas *previous=ui_scope_set(hook->canvas.width?&hook->canvas:NULL);
    hook->tick(menu);ui_scope_set(previous);ui_refresh_scale();cjk_direct_refresh();
}
static void __thiscall ui_menu_cleanup(void *menu,void *screen,void *lower){
    UiMenuHook *hook=ui_menu(menu);if(!hook)return;
    UiCanvas *previous=ui_scope_set(hook->canvas.width?&hook->canvas:NULL);
    hook->cleanup(menu,screen,lower);ui_scope_set(previous);hook->canvas=(UiCanvas){0};
}
static BOOL install_ui_scale(BYTE *base){
    if(*(void **)(base+0x5fbf44+16)!=(void *)(base+0x929d0)||*(void **)(base+0x6004b0+32)!=(void *)(base+0x15b470))return FALSE;
    for(unsigned i=0;i<sizeof(ui_menus)/sizeof(ui_menus[0]);i++){
        void **table=(void **)(base+ui_menu_tables[i]);if(!readable(table,17*sizeof(void *)))return FALSE;
        BYTE *method=table[9];if(method!=base+0x2da614||!readable(method,6)||method[0]!=0xff||method[1]!=0x25)return FALSE;
    }
    ui_scope_tls=TlsAlloc();if(ui_scope_tls==TLS_OUT_OF_INDEXES)return FALSE;
    DWORD old,ignored;void **graphics=(void **)(base+0x5fbf44),**pointer=(void **)(base+0x6004b0);
    ui_graphics_instance=(void **)(base+0x80d408);ui_game_ui=(void **)(base+0x7ef84c);
    ui_scale_value=ui_scale_clamp((int)GetPrivateProfileIntW(L"graphics",L"ui_scale",0,config_path));
    ui_resolution_original=(UiResolutionFn)graphics[4];
    ui_resolution_ratio=(float *)(base+0x7aacac);ui_menu_reference=(float *)(base+0x7a55f8);
    if(!VirtualProtect(pointer+8,4,PAGE_READWRITE,&old))return FALSE;
    ui_pc_pointer_original=(PointerPositionFn)pointer[8];InterlockedExchangePointer(pointer+8,(void *)ui_pc_pointer);VirtualProtect(pointer+8,4,old,&ignored);
    for(unsigned i=0;i<sizeof(ui_menus)/sizeof(ui_menus[0]);i++){
        UiMenuHook *hook=&ui_menus[i];hook->table=(void **)(base+ui_menu_tables[i]);
        if(!VirtualProtect(hook->table,17*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
        hook->tick=(TickFn)hook->table[3];hook->init=(ScreenFn)hook->table[15];hook->cleanup=(ScreenFn)hook->table[16];
        InterlockedExchangePointer(hook->table+3,(void *)ui_menu_tick);InterlockedExchangePointer(hook->table+15,(void *)ui_menu_init);InterlockedExchangePointer(hook->table+16,(void *)ui_menu_cleanup);
        VirtualProtect(hook->table,17*sizeof(void *),old,&ignored);
    }
    return TRUE;
}
