#define AA_CONTROL_ID 1601
#define RESOLUTION_FOCUS_ID 1604
#define BORDERLESS_CONTROL_ID 1602
static ScreenFn graphics_init_original,graphics_cleanup_original;
static TickFn graphics_tick_original;
static InputFn graphics_input_original;
static ColourFn graphics_colour_original;
static void *aa_menu_owner,*aa_label,*aa_bar,*aa_knob,*resolution_hover;
static void *borderless_label,*borderless_bar,*borderless_knob;
static const float *graphics_focus_colour;
static BOOL aa_pending,borderless_pending;
static float aa_hit_width,aa_hit_height;
static struct {void *item;int links[5];} aa_navigation[4];
static BOOL (*attach_antialiasing)(void)=aa_attach;
#include "graphics_scroll.h"
static void aa_connect_navigation(void *menu){
    void *full=((void **)menu)[62],*apply=((void **)menu)[66];
    void *left=((void **)menu)[64],*right=((void **)menu)[65];
    void *items[4]={full,apply,left,right};
    for(int i=0;i<4;i++){
        aa_navigation[i].item=items[i];
        memcpy(aa_navigation[i].links,(BYTE *)items[i]+0x98,sizeof(aa_navigation[i].links));
    }
    /* Native direction handler 0x44AA60: mode 1 uses IDs at +9C/+A0 for
     * Up/Down. Extend vanilla's closed row loop in both directions. */
    for(int i=2;i<4;i++){
        ((int *)items[i])[38]=1;((int *)items[i])[39]=UI_SCALE_CONTROL_ID;
        ((int *)items[i])[40]=((int *)full)[2];
    }
    ((int *)full)[38]=1;((int *)full)[40]=BORDERLESS_CONTROL_ID;
    ((int *)borderless_label)[38]=1;((int *)borderless_label)[39]=((int *)full)[2];
    ((int *)borderless_label)[40]=AA_CONTROL_ID;((int *)borderless_label)[41]=((int *)borderless_label)[42]=-1;
    ((int *)aa_label)[38]=1;((int *)aa_label)[39]=BORDERLESS_CONTROL_ID;
    ((int *)aa_label)[40]=UI_SCALE_CONTROL_ID;
    if(ui_slider_label){((int *)ui_slider_label)[38]=1;((int *)ui_slider_label)[39]=AA_CONTROL_ID;((int *)ui_slider_label)[40]=((int *)left)[2];((int *)ui_slider_label)[41]=((int *)ui_slider_label)[42]=-1;}
    ((int *)aa_label)[41]=((int *)aa_label)[42]=-1;
    ((int *)apply)[38]=1;((int *)apply)[39]=UI_SCALE_CONTROL_ID;
    ((int *)apply)[40]=((int *)left)[2];
}
static void aa_load_preference(void){
    WCHAR value[16];GetPrivateProfileStringW(L"graphics",L"antialiasing",L"false",value,16,config_path);
    InterlockedExchange(&aa_requested,!_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes"));
}
static void aa_sync_art(void *menu){
    if(aa_menu_owner!=menu||!aa_label)return;
    raw_text(aa_label,InterlockedCompareExchange(&aa_fault,0,0)?L"ANTIALIASING UNAVAILABLE":L"ANTIALIASING");
    ((float *)aa_label)[12]=aa_hit_width;((float *)aa_label)[13]=aa_hit_height;
    move_item(aa_label,((float *)aa_label)[8],((float *)aa_label)[9]);
    if(!aa_bar||!aa_knob)return;
    sprite_def(aa_knob,aa_pending?74:75);sprite_origin(aa_knob,aa_pending?5:3);
    float scale=((float *)menu)[15],pos[2]={((float *)aa_bar)[8],((float *)aa_bar)[9]};
    if(!aa_pending)pos[0]-=((float *)aa_bar)[12]*scale;
    pos[0]+=(aa_pending?-25:25)*scale;sprite_position(aa_knob,pos);
    if(borderless_bar&&borderless_knob){
        sprite_def(borderless_knob,borderless_pending?74:75);sprite_origin(borderless_knob,borderless_pending?5:3);
        pos[0]=((float *)borderless_bar)[8];pos[1]=((float *)borderless_bar)[9];
        if(!borderless_pending)pos[0]-=((float *)borderless_bar)[12]*scale;
        pos[0]+=(borderless_pending?-25:25)*scale;sprite_position(borderless_knob,pos);
    }
}
static void aa_graphics_input_state(void *menu){
    if(aa_menu_owner!=menu||!aa_label)return;
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    row_visible(aa_label,TRUE,mask);row_visible(aa_bar,TRUE,0);row_visible(aa_knob,TRUE,0);
    row_visible(resolution_hover,TRUE,mask);
    row_visible(borderless_label,TRUE,mask);row_visible(borderless_bar,TRUE,0);row_visible(borderless_knob,TRUE,0);
    row_visible(ui_slider_label,TRUE,mask);row_visible(ui_slider_rail,TRUE,0);row_visible(ui_slider_knob,TRUE,0);
    if(ui_slider_label)*(DWORD *)((BYTE *)ui_slider_label+64)=((int *)menu)[11]==UI_SCALE_CONTROL_ID?768:0;
    BOOL focused=((int *)menu)[11]==AA_CONTROL_ID;
    BOOL borderless=((int *)menu)[11]==BORDERLESS_CONTROL_ID;
    BOOL resolution=((int *)menu)[11]==RESOLUTION_FOCUS_ID;
    BOOL scaling=((int *)menu)[11]==UI_SCALE_CONTROL_ID;
    if(resolution)((int *)menu)[77]=0;
    *(DWORD *)((BYTE *)aa_label+64)=focused?768:0;
    if(borderless_label)*(DWORD *)((BYTE *)borderless_label+64)=borderless?768:0;
    if(focused||borderless||scaling)for(int i=62;i<=65;i++)if(((void **)menu)[i])*(DWORD *)((BYTE *)((void **)menu)[i]+64)=0;
    if(resolution){
        *(DWORD *)((BYTE *)((void **)menu)[62]+64)=0;
        *(DWORD *)((BYTE *)((void **)menu)[64]+64)=256;
        *(DWORD *)((BYTE *)((void **)menu)[65]+64)=512;
    }
}
static void make_graphics_toggle(void *menu,int id,const WCHAR *caption,float x,float y,float right,float gap,void **label,void **bar,void **knob){
    *label=create_text(caption,x,y,3,0,1,-1,FALSE,-1,FALSE,TRUE);
    if(!*label)return;
    ((int *)*label)[2]=id;((int *)*label)[15]=100;menu_item(*label,TRUE);
    ((float *)*label)[12]=right-x;((float *)*label)[13]=gap;
    static const float normal[4]={1,1,1,0.9f};colour_item(*label,normal);
    ((int *)*label)[1]++;add_screen(((void **)menu)[46],*label);
    float pos[2]={right,y};*bar=create_sprite(*ui_instance,73,pos,5,-1);*knob=create_sprite(*ui_instance,75,pos,3,-1);
    void *parts[2]={*bar,*knob};
    for(int i=0;i<2;i++)if(parts[i]){
        void *item=parts[i];menu_item(item,FALSE);enable_input(item,FALSE);((int *)item)[15]=100;
        scale_item(item,((float *)menu)[14],((float *)menu)[15]);colour_item(item,normal);
        if(!i){((int *)item)[18]=0;((int *)item)[38]=((int *)item)[39]=((int *)item)[40]=1;}
        ((int *)item)[1]++;add_screen(((void **)menu)[46],item);
    }
}
static void __thiscall aa_graphics_init(void *menu,void *screen,void *lower){
    if(display_installed&&graphics_bridge_ready){void *pc=display_pc();if(pc){prepare_extra_modes(pc);display_catalog_scope=TRUE;}}
    graphics_init_original(menu,screen,lower);display_catalog_scope=FALSE;
    aa_menu_owner=menu;aa_label=aa_bar=aa_knob=resolution_hover=borderless_label=borderless_bar=borderless_knob=NULL;
    ZeroMemory(aa_navigation,sizeof(aa_navigation));
    void *full=((void **)menu)[62],*left=((void **)menu)[64],*label=((void **)menu)[73];
    if(!readable(full,404)||!readable(left,404)||!readable(((void **)menu)[65],404)||!readable(label,484)||!readable(((void **)menu)[66],404))return;
    float original_gap=((float *)full)[9]-((float *)left)[9];if(original_gap<=0)return;
    float gap=original_gap,resolution_y=((float *)left)[9]-original_gap;
    float border_y=resolution_y+gap*2,y=resolution_y+gap*3,x=((float *)label)[8],right=((float *)full)[8];
    for(int i=62;i<=65;i++)if(((void **)menu)[i]){void *item=((void **)menu)[i];move_item(item,((float *)item)[8],i<=63?resolution_y+gap:resolution_y);}
    move_item(label,x,resolution_y+gap);
    void **begin=((void ***)menu)[70],**end=((void ***)menu)[71];
    if(begin&&end&&end>=begin&&end-begin<256&&readable(begin,(size_t)(end-begin)*sizeof(void *)))
        for(void **it=begin;it!=end;it++)if(readable(*it,484))move_item(*it,((float *)*it)[8],resolution_y);
    aa_pending=InterlockedCompareExchange(&aa_requested,0,0)!=0;
    borderless_pending=borderless_active;
    aa_hit_width=right-x;aa_hit_height=gap;
    make_graphics_toggle(menu,AA_CONTROL_ID,L"ANTIALIASING",x,y,right,gap,&aa_label,&aa_bar,&aa_knob);
    make_graphics_toggle(menu,BORDERLESS_CONTROL_ID,L"BORDERLESS FULLSCREEN",x,border_y,right,gap,&borderless_label,&borderless_bar,&borderless_knob);
    if(!aa_label||!borderless_label)return;
    ui_slider_create(menu,x,resolution_y+gap*4,right,gap);
    aa_connect_navigation(menu);
    float hit_pos[2]={(((float *)left)[8]+((float *)((void **)menu)[65])[8])*0.5f,((float *)left)[9]};
    float hit_size[2]={((float *)((void **)menu)[65])[8]-((float *)left)[8]-((float *)left)[12]*((float *)menu)[14]*2,gap*0.7f};
    if(hit_size[0]>0)resolution_hover=create_blank_button(*ui_instance,RESOLUTION_FOCUS_ID,0,hit_pos,hit_size);
    if(resolution_hover){
        sprite_origin(resolution_hover,4);
        ((int *)resolution_hover)[38]=1;((int *)resolution_hover)[39]=UI_SCALE_CONTROL_ID;
        ((int *)resolution_hover)[40]=((int *)full)[2];
        ((int *)resolution_hover)[1]++;add_screen(((void **)menu)[46],resolution_hover);
    }
    aa_sync_art(menu);graphics_scroll_init(menu,gap);
}
static void __thiscall aa_graphics_cleanup(void *menu,void *screen,void *lower){
    if(aa_menu_owner==menu){
        graphics_scroll_cleanup();
        for(int i=0;i<4;i++)if(aa_navigation[i].item){
            memcpy((BYTE *)aa_navigation[i].item+0x98,aa_navigation[i].links,sizeof(aa_navigation[i].links));
            aa_navigation[i].item=NULL;
        }
        void *items[12]={aa_label,aa_bar,aa_knob,resolution_hover,borderless_label,borderless_bar,borderless_knob,ui_slider_label,ui_slider_rail,ui_slider_knob,ui_slider_min,ui_slider_max};
        for(int i=0;i<12;i++)if(items[i]){((int *)items[i])[1]++;remove_screen(((void **)menu)[46],items[i]);}
        aa_menu_owner=aa_label=aa_bar=aa_knob=resolution_hover=borderless_label=borderless_bar=borderless_knob=NULL;
        ui_slider_label=ui_slider_rail=ui_slider_knob=ui_slider_min=ui_slider_max=NULL;ui_slider_dragging=FALSE;
    }
    graphics_cleanup_original(menu,screen,lower);
}
static void __thiscall aa_graphics_input_scoped(void *menu,int selection){
    if(aa_menu_owner==menu&&aa_label){
        if(selection==RESOLUTION_FOCUS_ID||selection==UI_SCALE_CONTROL_ID)return;
        if(selection==BORDERLESS_CONTROL_ID){
            if(!display_installed||!graphics_bridge_ready){log_line("Borderless requires the matching client helper.");return;}
            borderless_pending=!borderless_pending;
            if(borderless_pending&&*((BYTE *)menu+305))graphics_input_original(menu,4);
            aa_sync_art(menu);return;
        }
        if(selection==4){graphics_input_original(menu,selection);if(*((BYTE *)menu+305))borderless_pending=FALSE;aa_sync_art(menu);return;}
        if(selection==AA_CONTROL_ID){aa_pending=!aa_pending;InterlockedExchange(&aa_fault,0);aa_sync_art(menu);return;}
        if(selection==3){
            ui_slider_save();
            if(aa_pending&&!attach_antialiasing()){aa_pending=FALSE;aa_fail("Antialiasing unavailable: original graphics device could not be attached.");}
            if(WritePrivateProfileStringW(L"graphics",L"antialiasing",aa_pending?L"true":L"false",config_path))InterlockedExchange(&aa_requested,aa_pending);
            else log_line("Could not save antialiasing preference.");
            aa_sync_art(menu);
            if(apply_display(menu,borderless_pending))return;
        }
    }
    graphics_input_original(menu,selection);
}
static void __thiscall aa_graphics_input(void *menu,int selection){
    /* Renderer resets and camera/viewport callbacks always use real dimensions. */
    UiCanvas *previous=ui_scope();if(selection==3)ui_scope_set(NULL);
    aa_graphics_input_scoped(menu,selection);if(selection==3)ui_scope_set(previous);
}
static void __thiscall aa_graphics_colour(void *menu,int id,const float *colour){
    graphics_colour_original(menu,id,colour);
    if(menu==aa_menu_owner&&id==UI_SCALE_CONTROL_ID){if(ui_slider_label)colour_item(ui_slider_label,colour);if(ui_slider_rail)colour_item(ui_slider_rail,colour);if(ui_slider_knob)colour_item(ui_slider_knob,colour);if(ui_slider_min)colour_item(ui_slider_min,colour);if(ui_slider_max)colour_item(ui_slider_max,colour);}
    if(menu==aa_menu_owner&&id==AA_CONTROL_ID){if(aa_label)colour_item(aa_label,colour);if(aa_bar)colour_item(aa_bar,colour);if(aa_knob)colour_item(aa_knob,colour);}
    if(menu==aa_menu_owner&&id==BORDERLESS_CONTROL_ID){if(borderless_label)colour_item(borderless_label,colour);if(borderless_bar)colour_item(borderless_bar,colour);if(borderless_knob)colour_item(borderless_knob,colour);}
}
static void __thiscall aa_graphics_tick(void *menu){
    if(menu==aa_menu_owner)ui_slider_tick(menu);
    aa_graphics_input_state(menu);graphics_scroll_tick(menu);graphics_tick_original(menu);aa_graphics_input_state(menu);graphics_layout(menu);
    display_style();
    if(aa_menu_owner==menu&&InterlockedCompareExchange(&aa_fault,0,0)){aa_pending=FALSE;aa_sync_art(menu);}
    if(aa_menu_owner==menu&&((int *)menu)[11]==RESOLUTION_FOCUS_ID){
        void **begin=((void ***)menu)[70],**end=((void ***)menu)[71];
        if(begin&&end&&end>=begin&&end-begin<256&&readable(begin,(size_t)(end-begin)*sizeof(void *)))
            for(void **it=begin;it!=end;it++)if(readable(*it,484))colour_item(*it,graphics_focus_colour);
    }
    if(aa_menu_owner==menu&&(((int *)menu)[11]==AA_CONTROL_ID||((int *)menu)[11]==BORDERLESS_CONTROL_ID||((int *)menu)[11]==UI_SCALE_CONTROL_ID)){
        static const float normal[4]={1,1,1,0.9f};
        colour_item(((void **)menu)[73],normal);
        void **begin=((void ***)menu)[70],**end=((void ***)menu)[71];
        if(begin&&end&&end>=begin&&end-begin<256&&readable(begin,(size_t)(end-begin)*sizeof(void *)))
            for(void **it=begin;it!=end;it++)if(readable(*it,484))colour_item(*it,normal);
    }
}
static BOOL validate_graphics_aa_menu(BYTE *base,uintptr_t address){
    const DWORD slots[5]={3,15,16,24,26},entries[5]={0x2eca10,0x2eea04,0x2eda8c,0x2eccd1,0x2ec6e9},fixups[5]={0x7a59b4,0x7a5a30,0x7a5a2c,0x7a59b8,0x7a59a8};
    void **table=(void **)(base+0x7a59c0);
    if(!readable(table,27*sizeof(void *)))return FALSE;
    for(int i=0;i<5;i++){
        BYTE *code=base+entries[i];if((uintptr_t)table[slots[i]]!=address+entries[i]||!readable(code,6)||code[0]!=0xff||code[1]!=0x25||*(DWORD *)(code+2)!=(DWORD)(address+fixups[i]))return FALSE;
    }
    return TRUE;
}
static BOOL install_graphics_aa_menu(BYTE *base){
    if(!validate_graphics_aa_menu(base,(uintptr_t)base))return FALSE;
    void **table=(void **)(base+0x7a59c0);DWORD old;
    graphics_focus_colour=(const float *)(base+0x7a5644);
    if(!VirtualProtect(table,27*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    graphics_tick_original=(TickFn)table[3];graphics_init_original=(ScreenFn)table[15];graphics_cleanup_original=(ScreenFn)table[16];graphics_input_original=(InputFn)table[24];graphics_colour_original=(ColourFn)table[26];
    InterlockedExchangePointer(&table[3],(void *)aa_graphics_tick);InterlockedExchangePointer(&table[15],(void *)aa_graphics_init);InterlockedExchangePointer(&table[16],(void *)aa_graphics_cleanup);InterlockedExchangePointer(&table[24],(void *)aa_graphics_input);InterlockedExchangePointer(&table[26],(void *)aa_graphics_colour);
    DWORD ignored;VirtualProtect(table,27*sizeof(void *),old,&ignored);aa_load_preference();return TRUE;
}
