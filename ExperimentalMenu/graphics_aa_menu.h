#define AA_CONTROL_ID 1601
static ScreenFn graphics_init_original,graphics_cleanup_original;
static TickFn graphics_tick_original;
static InputFn graphics_input_original;
static ColourFn graphics_colour_original;
static void *aa_menu_owner,*aa_label,*aa_bar,*aa_knob;
static BOOL aa_pending;
static float aa_hit_width,aa_hit_height;
static struct {void *item;int links[5];} aa_navigation[2];
static BOOL (*attach_antialiasing)(void)=aa_attach;
static void aa_connect_navigation(void *menu){
    void *full=((void **)menu)[62],*apply=((void **)menu)[66];
    void *items[2]={full,apply};
    for(int i=0;i<2;i++){
        aa_navigation[i].item=items[i];
        memcpy(aa_navigation[i].links,(BYTE *)items[i]+0x98,sizeof(aa_navigation[i].links));
    }
    /* Native direction handler 0x44AA60: mode 1 uses IDs at +9C/+A0 for
     * Up/Down. Vanilla resolution/fullscreen links omit dynamically added rows. */
    ((int *)full)[38]=1;((int *)full)[40]=AA_CONTROL_ID;
    ((int *)aa_label)[38]=1;((int *)aa_label)[39]=((int *)full)[2];
    ((int *)aa_label)[40]=((int *)apply)[2];
    ((int *)aa_label)[41]=((int *)aa_label)[42]=-1;
    ((int *)apply)[38]=1;((int *)apply)[39]=AA_CONTROL_ID;
}
static void aa_load_preference(void){
    WCHAR value[16];GetPrivateProfileStringW(L"graphics",L"antialiasing",L"false",value,16,config_path);
    InterlockedExchange(&aa_requested,!_wcsicmp(value,L"true")||!wcscmp(value,L"1")||!_wcsicmp(value,L"yes"));
}
static void aa_sync_art(void *menu){
    if(aa_menu_owner!=menu||!aa_label)return;
    raw_text(aa_label,InterlockedCompareExchange(&aa_fault,0,0)?L"Antialiasing unavailable":L"Antialiasing");
    ((float *)aa_label)[12]=aa_hit_width;((float *)aa_label)[13]=aa_hit_height;
    if(!aa_bar||!aa_knob)return;
    sprite_def(aa_knob,aa_pending?74:75);sprite_origin(aa_knob,aa_pending?5:3);
    float scale=((float *)menu)[15],pos[2]={((float *)aa_bar)[8],((float *)aa_bar)[9]};
    if(!aa_pending)pos[0]-=((float *)aa_bar)[12]*scale;
    pos[0]+=(aa_pending?-25:25)*scale;sprite_position(aa_knob,pos);
}
static void aa_graphics_input_state(void *menu){
    if(aa_menu_owner!=menu||!aa_label)return;
    DWORD mask=*(DWORD *)((BYTE *)((void **)menu)[46]+40);
    row_visible(aa_label,TRUE,mask);row_visible(aa_bar,TRUE,0);row_visible(aa_knob,TRUE,0);
    BOOL focused=((int *)menu)[11]==AA_CONTROL_ID;
    *(DWORD *)((BYTE *)aa_label+64)=focused?768:0;
    if(focused)for(int i=62;i<=65;i++)if(((void **)menu)[i])*(DWORD *)((BYTE *)((void **)menu)[i]+64)=0;
}
static void __thiscall aa_graphics_init(void *menu,void *screen,void *lower){
    graphics_init_original(menu,screen,lower);aa_menu_owner=menu;aa_label=aa_bar=aa_knob=NULL;
    ZeroMemory(aa_navigation,sizeof(aa_navigation));
    void *full=((void **)menu)[62],*left=((void **)menu)[64],*label=((void **)menu)[73];
    if(!readable(full,404)||!readable(left,404)||!readable(label,484)||!readable(((void **)menu)[66],404))return;
    float gap=((float *)full)[9]-((float *)left)[9];if(gap<=0)return;
    float y=((float *)full)[9]+gap*0.5f,x=((float *)label)[8],right=((float *)full)[8];
    for(int i=62;i<=65;i++)if(((void **)menu)[i]){void *item=((void **)menu)[i];move_item(item,((float *)item)[8],((float *)item)[9]-gap*0.5f);}
    move_item(label,x,((float *)label)[9]-gap*0.5f);
    void **begin=((void ***)menu)[70],**end=((void ***)menu)[71];
    if(begin&&end&&end>=begin&&end-begin<256&&readable(begin,(size_t)(end-begin)*sizeof(void *)))
        for(void **it=begin;it!=end;it++)if(readable(*it,484))move_item(*it,((float *)*it)[8],((float *)*it)[9]-gap*0.5f);
    aa_pending=InterlockedCompareExchange(&aa_requested,0,0)!=0;
    aa_hit_width=right-x;aa_hit_height=gap;
    aa_label=create_text(L"Antialiasing",x,y,3,0,1,-1,FALSE,-1,FALSE,TRUE);
    if(!aa_label)return;
    ((int *)aa_label)[2]=AA_CONTROL_ID;((int *)aa_label)[15]=100;menu_item(aa_label,TRUE);
    ((float *)aa_label)[12]=right-x;((float *)aa_label)[13]=gap;
    static const float normal[4]={1,1,1,0.9f};colour_item(aa_label,normal);
    ((int *)aa_label)[1]++;add_screen(((void **)menu)[46],aa_label);
    aa_connect_navigation(menu);
    float pos[2]={right,y};
    aa_bar=create_sprite(*ui_instance,73,pos,5,-1);aa_knob=create_sprite(*ui_instance,75,pos,3,-1);
    void *parts[2]={aa_bar,aa_knob};
    for(int i=0;i<2;i++)if(parts[i]){
        void *item=parts[i];menu_item(item,FALSE);enable_input(item,FALSE);((int *)item)[15]=100;
        scale_item(item,((float *)menu)[14],((float *)menu)[15]);colour_item(item,normal);
        if(!i){((int *)item)[18]=0;((int *)item)[38]=((int *)item)[39]=((int *)item)[40]=1;}
        ((int *)item)[1]++;add_screen(((void **)menu)[46],item);
    }
    aa_sync_art(menu);
}
static void __thiscall aa_graphics_cleanup(void *menu,void *screen,void *lower){
    if(aa_menu_owner==menu){
        for(int i=0;i<2;i++)if(aa_navigation[i].item){
            memcpy((BYTE *)aa_navigation[i].item+0x98,aa_navigation[i].links,sizeof(aa_navigation[i].links));
            aa_navigation[i].item=NULL;
        }
        void *items[3]={aa_label,aa_bar,aa_knob};
        for(int i=0;i<3;i++)if(items[i]){((int *)items[i])[1]++;remove_screen(((void **)menu)[46],items[i]);}
        aa_menu_owner=aa_label=aa_bar=aa_knob=NULL;
    }
    graphics_cleanup_original(menu,screen,lower);
}
static void __thiscall aa_graphics_input(void *menu,int selection){
    if(aa_menu_owner==menu&&aa_label){
        if(selection==AA_CONTROL_ID){aa_pending=!aa_pending;InterlockedExchange(&aa_fault,0);aa_sync_art(menu);return;}
        if(selection==3){
            if(aa_pending&&!attach_antialiasing()){aa_pending=FALSE;aa_fail("Antialiasing unavailable: original graphics device could not be attached.");}
            if(WritePrivateProfileStringW(L"graphics",L"antialiasing",aa_pending?L"true":L"false",config_path))InterlockedExchange(&aa_requested,aa_pending);
            else log_line("Could not save antialiasing preference.");
            aa_sync_art(menu);
        }
    }
    graphics_input_original(menu,selection);
}
static void __thiscall aa_graphics_colour(void *menu,int id,const float *colour){
    graphics_colour_original(menu,id,colour);
    if(menu==aa_menu_owner&&id==AA_CONTROL_ID){if(aa_label)colour_item(aa_label,colour);if(aa_bar)colour_item(aa_bar,colour);if(aa_knob)colour_item(aa_knob,colour);}
}
static void __thiscall aa_graphics_tick(void *menu){
    aa_graphics_input_state(menu);graphics_tick_original(menu);aa_graphics_input_state(menu);
    if(aa_menu_owner==menu&&InterlockedCompareExchange(&aa_fault,0,0)){aa_pending=FALSE;aa_sync_art(menu);}
    if(aa_menu_owner==menu&&((int *)menu)[11]==AA_CONTROL_ID){
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
    if(!VirtualProtect(table,27*sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    graphics_tick_original=(TickFn)table[3];graphics_init_original=(ScreenFn)table[15];graphics_cleanup_original=(ScreenFn)table[16];graphics_input_original=(InputFn)table[24];graphics_colour_original=(ColourFn)table[26];
    InterlockedExchangePointer(&table[3],(void *)aa_graphics_tick);InterlockedExchangePointer(&table[15],(void *)aa_graphics_init);InterlockedExchangePointer(&table[16],(void *)aa_graphics_cleanup);InterlockedExchangePointer(&table[24],(void *)aa_graphics_input);InterlockedExchangePointer(&table[26],(void *)aa_graphics_colour);
    DWORD ignored;VirtualProtect(table,27*sizeof(void *),old,&ignored);aa_load_preference();return TRUE;
}
