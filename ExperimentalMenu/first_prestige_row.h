/* Extend the row built by the original game; never fabricate an SSP/reel.
 * Use supplied circular artwork, retaining the native placeholder on failure. */
#include "first_prestige_artwork.h"
/* Updated user-supplied artwork enabled for local appearance testing. */
#define FIRST_PRESTIGE_CUSTOM_ARTWORK 1
static void *first_prestige_row_owner,*first_prestige_row_icon;
static void first_prestige_row_clear(void){first_prestige_row_owner=NULL;first_prestige_row_icon=NULL;}
static BOOL first_prestige_row_get(void *menu,void **row,void ***items,unsigned *count){
    void **rows;unsigned length;
    if(!achievement_probe_vector(menu,472,&rows,&length,39)||length!=39
        ||!progression_probe_read(rows+18,row,4)||!achievement_probe_vector(*row,376,items,count,16)
        ||*count<4||*count>6)return FALSE;
    for(unsigned i=0;i<4;i++)if(!readable((*items)[i],i<2?484:404))return FALSE;
    return TRUE;
}
static void first_prestige_row_update(void *menu){
    if(!first_prestige_entry_installed||!progression_domain_active||!progression_load_observed
        ||!readable(progression_friend_length,4)||*progression_friend_length||!raw_text||!sprite_def)return;
    void *row;void **items;unsigned count;
    if(!first_prestige_row_get(menu,&row,&items,&count))return;
    raw_text(items[0],L"Re:Start");raw_text(items[1],L"Start again");
    if(row!=first_prestige_row_owner||items[3]!=first_prestige_row_icon){
        /* Original elUIItemSprite.setDefinition updates its existing owned
         * visual. Preserve geometry, tint, input and the locked-row overlay. */
        BOOL custom=FIRST_PRESTIGE_CUSTOM_ARTWORK&&readable(ui_instance,4)&&readable(*ui_instance,4)&&first_prestige_artwork(*ui_instance);
        sprite_def(items[3],custom?(count==5?187:148):(count==5?186:147));
        first_prestige_row_owner=row;first_prestige_row_icon=items[3];
        log_line(custom?"First Prestige row labelled with custom circular artwork; native reel owns layout and scrolling.":"First Prestige row labelled; neighbouring native achievement artwork borrowed; original reel owns layout and scrolling.");
    }
}
