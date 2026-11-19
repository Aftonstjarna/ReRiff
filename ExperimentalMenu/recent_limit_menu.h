/* Reuse the Graphics resolution selector's native arrows and input masks. */
#define RECENT_LIMIT_ID 1607
#define RECENT_LEFT_ID 1608
#define RECENT_RIGHT_ID 1609
static void *recent_limit_label,*recent_limit_left,*recent_limit_right,*recent_limit_restart;
static int recent_limit_pending,recent_limit_saved;
static const int recent_limit_values[]={0,20,50,100};
static int recent_limit_index(int value){for(int i=0;i<4;i++)if(recent_limit_values[i]==value)return i;return 0;}
static BOOL recent_limit_focus(int id){return id==RECENT_LIMIT_ID||id==RECENT_LEFT_ID||id==RECENT_RIGHT_ID;}
static void recent_limit_art(void){
    if(!recent_limit_label)return;
    WCHAR text[64];if(recent_limit_pending)_snwprintf(text,64,L"RECENT TRACKS: %d",recent_limit_pending);else wcscpy(text,L"RECENT TRACKS: ALL");
    float width=((float *)recent_limit_label)[12],height=((float *)recent_limit_label)[13];
    raw_text(recent_limit_label,text);((float *)recent_limit_label)[12]=width;((float *)recent_limit_label)[13]=height;
}
static void recent_limit_create(void *menu,float y,float gap){
    recent_limit_pending=recent_limit_saved=recent_limit_values[recent_limit_index((int)GetPrivateProfileIntW(L"graphics",L"recent_tracks_limit",0,config_path))];
    void *left=((void **)menu)[64],*right=((void **)menu)[65];
    float x=(((float *)left)[8]+((float *)right)[8])*0.5f;
    recent_limit_label=create_text(L"",x,y-gap*0.12f,3,1,1,-1,FALSE,-1,FALSE,TRUE);
    if(recent_limit_label){
        ((int *)recent_limit_label)[2]=RECENT_LIMIT_ID;
        ((float *)recent_limit_label)[12]=((float *)right)[8]-((float *)left)[8]-((float *)left)[12]*((float *)menu)[14]*2;
        ((float *)recent_limit_label)[13]=gap*0.7f;menu_item(recent_limit_label,TRUE);
    }
    for(int i=0;i<2;i++){
        float pos[2]={((float *)(i?right:left))[8],y};
        void *item=graphics_button_sprite(*ui_instance,i?60:59,i?RECENT_RIGHT_ID:RECENT_LEFT_ID,0,pos,i?5:3);
        if(i)recent_limit_right=item;else recent_limit_left=item;
        if(item)scale_item(item,((float *)menu)[14],((float *)menu)[15]);
    }
    void *items[]={recent_limit_label,recent_limit_left,recent_limit_right};
    for(int i=0;i<3;i++)if(items[i]){
        void *item=items[i];((int *)item)[15]=100;((int *)item)[38]=1;
        ((int *)item)[39]=UI_SCALE_CONTROL_ID;((int *)item)[40]=((int *)((void **)menu)[66])[2];
        ((int *)item)[41]=((int *)item)[42]=-1;
        ((int *)item)[1]++;add_screen(((void **)menu)[46],item);
    }
    recent_limit_restart=create_text(L"Restart required",x,y+gap*0.30f,3,1,0,-1,FALSE,-1,FALSE,TRUE);
    if(recent_limit_restart){
        void *item=recent_limit_restart;float height=((float *)item)[13];
        if(height>0){float factor=gap*0.22f/height;scale_item(item,factor,factor);}
        static const float muted[4]={1,1,1,0.65f};colour_item(item,muted);
        menu_item(item,FALSE);enable_input(item,FALSE);((int *)item)[15]=100;
        ((int *)item)[1]++;add_screen(((void **)menu)[46],item);
    }
    recent_limit_art();
}
static BOOL recent_limit_save(void){
    WCHAR value[16];_snwprintf(value,16,L"%d",recent_limit_pending);
    if(!WritePrivateProfileStringW(L"graphics",L"recent_tracks_limit",value,config_path))return FALSE;
    recent_limit_saved=recent_limit_pending;return TRUE;
}
