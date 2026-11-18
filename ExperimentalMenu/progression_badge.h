/* Difficulty uses native sprite 89 (filled), 90 (outline): original
 * gaTrackSummary screen. Keep the original Profile number; its owned child
 * sprite and text are siblings in its original level panel, sharing its
 * coordinate space and cleanup. Native Profile state gates visibility. */
typedef unsigned (__thiscall *ProgressFontFn)(void *);
static ProgressFontFn progression_font;
static void *progression_badge_parent,*progression_badge_number,*progression_badge_star,*progression_badge_suffix;
static float progression_badge_anchor;
static BOOL progression_badge_shifted;
static float progression_badge_logged_height;
static ScreenFn progression_cleanup_original;
static BOOL progression_badge_child(void *number,void *child){
    if(!child||!readable(number,404))return FALSE;
    void **begin=*(void ***)((BYTE *)number+376),**end=*(void ***)((BYTE *)number+380);
    uintptr_t size=(uintptr_t)end-(uintptr_t)begin;
    if((uintptr_t)end<(uintptr_t)begin||size>64*sizeof(void *)||size%sizeof(void *)||!readable(begin,size))return FALSE;
    for(void **at=begin;at!=end;at++)if(*at==child)return TRUE;return FALSE;
}
static void progression_badge_clear(void){progression_badge_parent=NULL;progression_badge_number=NULL;progression_badge_star=NULL;progression_badge_suffix=NULL;progression_badge_shifted=FALSE;progression_badge_logged_height=0;}
static void progression_badge_restore(void *menu){
    void *number=readable(menu,408)?*(void **)((BYTE *)menu+276):NULL;
    void *parent=readable(menu,408)?*(void **)((BYTE *)menu+384):NULL;
    if(number!=progression_badge_number||parent!=progression_badge_parent||!progression_badge_child(parent,number)||!progression_badge_child(parent,progression_badge_star)){progression_badge_clear();return;}
    if(progression_badge_shifted){move_item(number,progression_badge_anchor,((float *)number)[9]);progression_badge_shifted=FALSE;}
    show_item(progression_badge_star,FALSE);if(progression_badge_suffix)show_item(progression_badge_suffix,FALSE);
}
/* Original UIFont glyph metrics supply the actual digit ink rectangle.
 * Text's line bounds contain vertical padding and are not the visible height. */
static BOOL progression_badge_ink(void *number,WCHAR digit,float line_height,float *height,float *centre){
    if(!readable(number,484))return FALSE;BYTE *font=*(BYTE **)((BYTE *)number+0x1cc);
    if(!readable(font,0x70))return FALSE;
    BYTE *begin=*(BYTE **)(font+0x28),*end=*(BYTE **)(font+0x2c);uintptr_t size=(uintptr_t)end-(uintptr_t)begin;
    if((uintptr_t)end<(uintptr_t)begin||size>65536*44||size%44||!readable(begin,size))return FALSE;
    float texture_height=*(float *)(font+0x14);if(!(texture_height>0&&texture_height<=32768))return FALSE;
    for(BYTE *glyph=begin;glyph<end;glyph+=44)if(*(WORD *)glyph==digit){
        float cap=*(float *)(glyph+0x18)*texture_height,offset=*(float *)(glyph+0x20);
        if(!(cap>0&&cap<=4096&&offset>=-4096&&offset<=4096))return FALSE;
        *height=cap;*centre=offset+cap*.5f-line_height*.5f;return TRUE;
    }
    return FALSE;
}
static BOOL progression_badge_render(void *parent,void *number,const WCHAR *identity){
    const WCHAR *marker=wcschr(identity,L'*');if(!marker){raw_text(number,identity);return TRUE;}
    size_t length=(size_t)(marker-identity);if(!length||length>=64||marker[-1]!=L' '||!progression_font||!create_sprite||!create_text||!add_item||!readable(ui_instance,4)||!readable(*ui_instance,4))return FALSE;
    WCHAR level[64];wmemcpy(level,identity,length-1);level[length-1]=0;
    if(!progression_badge_child(parent,number))return FALSE;
    if(progression_badge_number!=number||progression_badge_parent!=parent||!progression_badge_child(parent,progression_badge_star)){
        progression_badge_clear();progression_badge_number=number;progression_badge_parent=parent;
        float pos[2]={0,0};progression_badge_star=create_sprite(*ui_instance,89,pos,4,-1);
        if(!progression_badge_star){progression_badge_clear();return FALSE;}
        menu_item(progression_badge_star,FALSE);enable_input(progression_badge_star,FALSE);show_item(progression_badge_star,FALSE);
        ((int *)progression_badge_star)[1]++;add_item(parent,progression_badge_star);
    }
    unsigned font=progression_font(number);if(font>64)return FALSE;
    if(!progression_badge_suffix){
        progression_badge_suffix=create_text(L"",0,0,font,0,1,608,FALSE,-1,FALSE,TRUE);
        if(!progression_badge_suffix)return FALSE;
        menu_item(progression_badge_suffix,FALSE);enable_input(progression_badge_suffix,FALSE);show_item(progression_badge_suffix,FALSE);
        ((int *)progression_badge_suffix)[1]++;add_item(parent,progression_badge_suffix);
    }
    raw_text(number,level);raw_text(progression_badge_suffix,marker+1);
    float width=((float *)number)[12],height=((float *)number)[13],star_width=((float *)progression_badge_star)[12],star_height=((float *)progression_badge_star)[13];
    float suffix_width=marker[1]?((float *)progression_badge_suffix)[12]:0;
    if(!(width>0&&width<10000&&height>0&&height<10000&&star_width>0&&star_height>0&&suffix_width>=0&&suffix_width<10000))return FALSE;
    float size=0,centre=0;if(!progression_badge_ink(number,level[0],height,&size,&centre))return FALSE;
    if(size!=progression_badge_logged_height){char detail[192];snprintf(detail,sizeof(detail),"Prestige star layout: digit_ink_height=%.2f text_line_height=%.2f baseline_centre=%.2f parent=native-level-panel.",size,height,centre);log_line(detail);progression_badge_logged_height=size;}
    float gap=size*.16f,star_size=star_width*size/star_height;
    float extra=gap+star_size+(marker[1]?gap+suffix_width:0);
    progression_badge_anchor=((float *)number)[8];progression_badge_shifted=TRUE;
    move_item(number,progression_badge_anchor-extra*.5f,((float *)number)[9]);
    scale_item(progression_badge_star,size/star_height,size/star_height);
    float x=((float *)number)[8],y=((float *)number)[9];
    move_item(progression_badge_star,x+width*.5f+gap+star_size*.5f,y+centre);
    move_item(progression_badge_suffix,x+width*.5f+gap+star_size+gap,y);
    static const float white[4]={1,1,1,1};colour_item(progression_badge_star,white);colour_item(progression_badge_suffix,white);
    show_item(progression_badge_star,TRUE);show_item(progression_badge_suffix,marker[1]!=0);return TRUE;
}
static void __thiscall progression_profile_cleanup(void *menu,void *screen,void *lower){
    progression_badge_restore(menu);progression_badge_clear();first_prestige_row_clear();progression_cleanup_original(menu,screen,lower);
}
