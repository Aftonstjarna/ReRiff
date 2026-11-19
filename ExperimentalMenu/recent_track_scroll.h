/* gaFileSelectInterface.tickInput (06001055): delay .25, acceleration .07/s,
 * floor .02. Add only missing repeat steps through native row navigation. */
typedef struct RecentRepeat {void *owner,*rows;int sort,direction;DWORD started,last,seen;} RecentRepeat;
static RecentRepeat recent_repeat;
static void recent_repeat_reset(void){ZeroMemory(&recent_repeat,sizeof(recent_repeat));}
static DWORD recent_repeat_interval(DWORD elapsed){return elapsed>=3286?20:250-(elapsed*7)/100;}
static BOOL recent_repeat_due(void *owner,void *rows,int sort,int direction,DWORD now,BOOL native_moved){
    if(!direction){recent_repeat_reset();return FALSE;}
    if(recent_repeat.owner!=owner||recent_repeat.rows!=rows||recent_repeat.sort!=sort||recent_repeat.direction!=direction||now-recent_repeat.seen>500){
        recent_repeat=(RecentRepeat){owner,rows,sort,direction,now,now,now};return FALSE;
    }
    recent_repeat.seen=now;
    if(native_moved){recent_repeat.last=now;return FALSE;}
    if(now-recent_repeat.last<recent_repeat_interval(now-recent_repeat.started))return FALSE;
    recent_repeat.last=now;return TRUE;
}
static BOOL recent_scroll_foreground(void){DWORD pid=0;HWND foreground=GetForegroundWindow();if(foreground)GetWindowThreadProcessId(foreground,&pid);return pid==GetCurrentProcessId();}
static BOOL (*recent_scroll_active)(void)=recent_scroll_foreground;
static DWORD (WINAPI *recent_scroll_clock)(void)=GetTickCount;
static void *recent_scroll_focus(void *menu){
    if(!readable(menu,1284))return NULL;void *screen=((void **)menu)[46];
    if(!readable(screen,44)||!*(DWORD *)((BYTE *)screen+40))return NULL;
    return *(void **)((BYTE *)screen+20);
}
static void recent_scroll_trace(int reason){
    static int previous=-1;static unsigned lines;
    if(previous==reason)return;previous=reason;
    if(lines++<24){char text[128];snprintf(text,sizeof(text),"Recent Tracks acceleration: state=%d (0=ready,1=inactive,2=no-focus,3=non-row-focus).",reason);log_line(text);}
}
/* Finish the native focus-follow target, not a second input/reel tick.
 * moveToSelection(..., &zero) updates position and calls onReelSettled itself. */
static void recent_scroll_follow(void *menu,unsigned count){
    void *reel=*(void **)((BYTE *)menu+1244);
    if(!reel_move||!reel_vtable||!readable(reel,532)||*(void ***)reel!=reel_vtable)return;
    if(!*((BYTE *)reel+452)||*(int *)((BYTE *)reel+436)!=4)return;
    int target=*(int *)((BYTE *)reel+456);
    if(target<0||(unsigned)target>=count)return;
    float instant=0;reel_move(reel,target,&instant);
}
static void recent_scroll_after(void *menu,void *before){
    BYTE *rows;unsigned count;
    if(!recent_scroll_active()||(menu_overlay_active()&&((void **)*overlay_manager)[11]!=menu)||!readable(menu,1284)||((int *)menu)[7]!=1
        ||!recent_stars_vector(menu,1248,4,&rows,&count)||!count){recent_scroll_trace(1);recent_repeat_reset();return;}
    void *focus=recent_scroll_focus(menu);if(!readable(focus,404)){recent_scroll_trace(2);recent_repeat_reset();return;}
    unsigned index=0;for(;index<count;index++)if(((void **)rows)[index]==focus)break;
    if(index==count||!*(DWORD *)((BYTE *)focus+76)){recent_scroll_trace(3);recent_repeat_reset();return;}
    recent_scroll_trace(0);
    recent_scroll_follow(menu,count);
    DWORD held=direction_input();int direction=held==64?-1:held==128?1:0;
    if(!recent_repeat_due(menu,rows,*(int *)((BYTE *)menu+1232),direction,recent_scroll_clock(),focus!=before))return;
    if((direction<0&&!index)||(direction>0&&index+1>=count))return; /* no wraparound */
    void *target=((void **)rows)[direction<0?index-1:index+1];
    void **table=*(void ***)focus;
    if(!readable(target,404)||!readable(table,29*sizeof(void *))||!table[28])return;
    /* tickInput enables the adjacent native row before gaMenu dispatches input. */
    *((BYTE *)target+372)|=4;
    ((DirectionFn)table[28])(focus,direction<0?64:128,0);
    static unsigned traced;
    if(traced++<16){char text[160];snprintf(text,sizeof(text),"Recent Tracks accelerated step: row=%u direction=%d interval_ms=%lu focus_changed=%u.",index,direction,(unsigned long)recent_repeat_interval(recent_repeat.seen-recent_repeat.started),recent_scroll_focus(menu)!=focus);log_line(text);}
}
