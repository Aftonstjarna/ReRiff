/* Recent Tracks uses cached localSong difficulty (+192), as does its original
 * difficulty comparator. Reuse TrackSummary's filled/empty sprites 89/90. */
static TickFn recent_stars_original_tick;
static void **recent_stars_save;
static BOOL recent_stars_string(const BYTE *s,const BYTE **text,unsigned *length){
    unsigned capacity;if(!readable(s,24))return FALSE;memcpy(length,s+16,4);memcpy(&capacity,s+20,4);
    if(*length>4096||capacity<*length)return FALSE;
    *text=capacity<16?s:*(const BYTE **)s;return !*length||readable(*text,*length);
}
static BOOL recent_stars_equal(const BYTE *a,const BYTE *b){
    const BYTE *x,*y;unsigned nx,ny;return recent_stars_string(a,&x,&nx)&&recent_stars_string(b,&y,&ny)&&nx&&nx==ny&&!memcmp(x,y,nx);
}
static BOOL recent_stars_vector(const BYTE *owner,unsigned offset,unsigned stride,BYTE **begin,unsigned *count){
    BYTE *end;if(!owner||!stride||!readable(owner+offset,12))return FALSE;*begin=*(BYTE **)(owner+offset);end=*(BYTE **)(owner+offset+4);
    uintptr_t first=(uintptr_t)*begin,last=(uintptr_t)end;if(last<first||(uintptr_t)*(BYTE **)(owner+offset+8)<last||(last-first)%stride||(last-first)/stride>100000)return FALSE;
    *count=(unsigned)((last-first)/stride);return !*count||readable(*begin,last-first);
}
static BOOL recent_stars_row(void *row,int rating){
    BYTE *children;unsigned count;if(!readable(row,404)||!recent_stars_vector(row,376,4,&children,&count)||count!=4)return FALSE;
    if(rating<1||rating>5)rating=0; /* no cached rating: empty native stars */
    float width=((float *)row)[12],height=((float *)row)[13],scale=width/1400.0f;
    if(!(scale>0.05f&&scale<10&&height>0))return FALSE;
    float size=32*scale;if(size>height*0.65f)size=height*0.65f;
    for(int star=0;star<5;star++){
        float pos[2]={width-230*scale+star*40*scale,height*0.5f};
        void *item=create_sprite(*ui_instance,star<rating?89:90,pos,4,-1);if(!item)return FALSE;
        float original=((float *)item)[12];if(!(original>0))return FALSE;
        float factors[2]={size/original,size/original};
        ((void (__thiscall *)(void *,const float *))(*(void ***)item)[9])(item,factors);
        menu_item(item,FALSE);enable_input(item,FALSE);
        ((int *)item)[1]++;add_item(row,item); /* native consuming SSP convention */
    }
    return TRUE;
}
static void recent_stars_update(void *menu){
    BYTE *rows,*ids,*songs,*children;unsigned count,ids_count,song_count,child_count;
    if(!recent_stars_save||!readable(recent_stars_save,4)||!*recent_stars_save||!create_sprite||!add_item||!ui_instance||!*ui_instance)return;
    if(!recent_stars_vector(menu,1248,4,&rows,&count)||!count||!recent_stars_vector(menu,1260,24,&ids,&ids_count)||count!=ids_count)return;
    /* Sorting rebuilds native rows. A decorated first row avoids rescanning
     * the catalogue on every tick; no row pointers survive native cleanup. */
    if(!recent_stars_vector(*(BYTE **)rows,376,4,&children,&child_count)||child_count!=4)return;
    if(!recent_stars_vector(*recent_stars_save,176,200,&songs,&song_count))return;
    for(unsigned i=0;i<count;i++){
        int rating=0;
        for(unsigned j=0;j<song_count;j++)if(recent_stars_equal(ids+i*24,songs+j*200+24)){
            memcpy(&rating,songs+j*200+192,4);break;
        }
        recent_stars_row(*(void **)(rows+i*4),rating);
    }
}
#include "recent_track_scroll.h"
static void __thiscall recent_stars_tick(void *menu){void *before=recent_scroll_focus(menu);recent_stars_original_tick(menu);recent_stars_update(menu);recent_scroll_after(menu,before);}
static BOOL install_recent_stars(BYTE *base){
    void **slot=(void **)(base+0x7a79c8)+3;BYTE *thunk=base+0x364e74;
    /* Reserve a right-hand column by narrowing native text clipping only;
     * row hitboxes, spacing, separators and reel bounds remain original. */
    BYTE *clip=base+0x36394c+12+0x3c6;static const BYTE original[]={0x23,0,0,0,0,0,0xe0,0x95,0x40};
    if(!readable(slot,4)||*slot!=thunk||!readable(thunk,6)||thunk[0]!=0xff||thunk[1]!=0x25||*(DWORD *)(thunk+2)!=(DWORD)(base+0x7a7a3c)
        ||!readable(clip,9)||memcmp(clip,original,9))return FALSE;
    DWORD old,clip_old,ignored;if(!VirtualProtect(clip,9,PAGE_READWRITE,&clip_old))return FALSE;
    if(!VirtualProtect(slot,4,PAGE_READWRITE,&old)){VirtualProtect(clip,9,clip_old,&ignored);return FALSE;}
    double text_width=1100;memcpy(clip+1,&text_width,8);VirtualProtect(clip,9,clip_old,&ignored);
    recent_stars_original_tick=(TickFn)*slot;recent_stars_save=(void **)(base+0x7ed7b8);InterlockedExchangePointer(slot,(void *)recent_stars_tick);VirtualProtect(slot,4,old,&ignored);
    log_line("Recent Tracks: native difficulty stars added to rows; title/artist clipping reserves the rating column.");return TRUE;
}
