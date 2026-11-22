/* Optional track culling disable.
 *
 * The race manager keeps every gaTriggerCullingVolume that was created from
 * the track. Each volume owns the props/triggers for a section and the game
 * hides the whole section when its per-view visibility counters reach zero.
 * Writing the counters non-zero keeps those sections eligible on the next
 * tick. While the race is paused the trigger manager does not tick, so the
 * paused path also forces the volume's existing onVisible callbacks.
 *
 * Opt-in: [graphics] disable_culling=1 in client.ini. Restart to apply.
 */
typedef void (__thiscall *RaceCullingVisible)(void *,int,BYTE);
typedef void (__cdecl *RaceCullingVisualVisible)(void *,BYTE);

static BOOL race_culling_enabled,race_culling_trace,race_culling_trace_announced,race_culling_visual_announced;
static unsigned race_culling_frames,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count,race_culling_prop_count,race_culling_prop_forced;
static unsigned race_culling_ramp_count,race_culling_ramp_forced,race_culling_cuboid_count,race_culling_segment_count;
static unsigned race_culling_strip_count,race_culling_strip_block_count,race_culling_strip_forced;
static RaceCullingVisible race_culling_force_visible,race_culling_on_visible;
static RaceCullingVisualVisible race_culling_visual;
__declspec(dllexport) int __cdecl ReRiffConfigureCullingVisible(void *callback){
    if(!callback)return 0;race_culling_visual=(RaceCullingVisualVisible)callback;return 1;
}

/* tickBlocks retires the oldest block through the branch at IL_01e7. Turn
 * that beq into an unconditional br so blocks accumulate for photo mode;
 * the post-tick visual pass keeps their visuals visible. Applied before the
 * first race tick so the CLR has not compiled this method yet. */
static BOOL race_culling_retain_blocks(BYTE *base){
    BYTE *site=base+0x26c624;
    static const BYTE expected[]={0x11,0x08,0x15,0x2e,0x33};
    if(!readable(site,sizeof(expected))||memcmp(site,expected,sizeof(expected)))return FALSE;
    DWORD old,ignored;
    if(!VirtualProtect(site,sizeof(expected),PAGE_READWRITE,&old))return FALSE;
    site[3]=0x2b;
    VirtualProtect(site,sizeof(expected),old,&ignored);
    return TRUE;
}

static BOOL install_race_culling(BYTE *base){
    WCHAR value[16];
    WCHAR client[MAX_PATH];wcsncpy(client,config_path,MAX_PATH-1);client[MAX_PATH-1]=0;
    WCHAR *slash=wcsrchr(client,L'\\');if(slash)wcscpy(slash+1,L"client.ini");
    GetPrivateProfileStringW(L"graphics",L"disable_culling",L"false",value,16,client);
    race_culling_enabled=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    GetPrivateProfileStringW(L"camera",L"race_orbit_trace",L"false",value,16,client);
    race_culling_trace=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    if(!race_culling_enabled)return TRUE;
    if(!readable(base+0x278ea0,8)||!readable(base+0x27959c,8))return FALSE;
    race_culling_force_visible=(RaceCullingVisible)(base+0x278ea0);
    race_culling_on_visible=(RaceCullingVisible)(base+0x27959c);
    BOOL retained=race_culling_retain_blocks(base);
    char detail[160];snprintf(detail,sizeof(detail),"Track culling disable enabled; trace=%d; block retention=%d.",race_culling_trace?1:0,retained?1:0);log_line(detail);
    return TRUE;
}

static BOOL race_culling_paused(BYTE *base){
    if(!readable(base+0x7d6668,4))return FALSE;
    void *race=*(void **)(base+0x7d6668);
    return readable(race,13)&&*((BYTE *)race+12)!=0;
}

static void race_culling_apply(void *manager,BOOL paused){
    race_culling_volume_count=race_culling_valid_count=race_culling_forced_count=0;
    if(!readable(manager,36))return;
    void **begin=*(void ***)((BYTE *)manager+28),**end=*(void ***)((BYTE *)manager+32);
    if(!begin||!end||end<begin||(BYTE *)end-(BYTE *)begin>32768)return;
    race_culling_volume_count=(unsigned)(end-begin);
    for(void **item=begin;item<end;item++){
        void *volume=*item;
        if(!readable(volume,316))continue;
        race_culling_valid_count++;
        /* All twelve viewport counters, not just the four the tick helper
         * samples, so split-screen and replay views stay eligible too. */
        for(int view=0;view<12;view++)*((WORD *)((BYTE *)volume+124+view*2))=1;
        if(!paused)continue;
        race_culling_forced_count++;
        for(int view=0;view<4;view++){
            if(race_culling_force_visible)race_culling_force_visible(volume,view,1);
            if(race_culling_on_visible)race_culling_on_visible(volume,view,1);
        }
    }
}

static void race_culling_props(BYTE *base){
    race_culling_prop_count=race_culling_prop_forced=0;
    if(!race_culling_visual){
        if(!race_culling_visual_announced){race_culling_visual_announced=TRUE;log_line("Culling prop visibility bridge unavailable; trigger culling fallback only.");}
        return;
    }
    if(!readable(base+0x7e3080,4))return;
    void *manager=*(void **)(base+0x7e3080);
    if(!readable(manager,456))return;
    void **begin=*(void ***)((BYTE *)manager+448),**end=*(void ***)((BYTE *)manager+452);
    if(!begin||!end||end<begin||(BYTE *)end-(BYTE *)begin>20000*4)return;
    race_culling_prop_count=(unsigned)(end-begin);
    for(void **item=begin;item<end;item++){
        void *prop=*item;
        if(!readable(prop,572))continue;
        void *visual=*(void **)((BYTE *)prop+288);
        if(!readable(visual,132))continue;
        *((BYTE *)prop+496)=0;
        *((BYTE *)prop+464)=0xFF;
        *((DWORD *)((BYTE *)visual+128))=0x0FFF;
        *((BYTE *)visual+20)=1;
        race_culling_visual(visual,1);
        race_culling_prop_forced++;
    }
}

static void race_culling_ramps(BYTE *base){
    race_culling_ramp_count=race_culling_ramp_forced=0;
    if(!race_culling_visual)return;
    if(!readable(base+0x7db0cc,4))return;
    void *manager=*(void **)(base+0x7db0cc);
    if(!readable(manager,44))return;
    BYTE *begin=*(BYTE **)((BYTE *)manager+36),*end=*(BYTE **)((BYTE *)manager+40);
    if(!begin||!end||end<begin||(size_t)(end-begin)>10000*24)return;
    race_culling_ramp_count=(unsigned)((end-begin)/24);
    for(BYTE *ramp=begin;ramp<end;ramp+=24){
        void *visual=*(void **)ramp;
        if(!readable(visual,132))continue;
        *((BYTE *)visual+20)=1;
        *((DWORD *)((BYTE *)visual+128))=0x0FFF;
        race_culling_visual(visual,1);
        race_culling_ramp_forced++;
    }
}

/* gaSegmentManager::Segment is logical track data: two node indices, a type,
 * spline values and a name. The rendered track blocks live on gaTrackStrip.
 * tickBlocks resolves each block's elVisual through strip+164; force those
 * visuals after the original tick so distance-based hiding cannot remove the
 * road surface. The strip vector is trackManager+2720 (clearTrackPhys). */
static void race_culling_strips(BYTE *base){
    race_culling_strip_count=race_culling_strip_block_count=race_culling_strip_forced=0;
    if(!race_culling_visual||!readable(base+0x7dce7c,4))return;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2732))return;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return;
    race_culling_strip_count=(unsigned)(end-begin);
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        BYTE *blocks=*(BYTE **)(strip+152),*blocks_end=*(BYTE **)(strip+156);
        if(!blocks||!blocks_end||blocks_end<blocks||(size_t)(blocks_end-blocks)>10000*88
            ||((size_t)(blocks_end-blocks)%88)||!readable(blocks,(size_t)(blocks_end-blocks)))continue;
        void **visuals=*(void ***)(strip+164);
        if(!visuals||!readable(visuals,4))continue;
        for(BYTE *block=blocks;block<blocks_end;block+=88){
            int index=*((int *)(block+80));
            if(index<0||index>=10000)continue;
            void **slot=(void **)((BYTE *)visuals+index*4);
            if(!readable(slot,4))continue;
            void *visual=*slot;
            if(!readable(visual,132))continue;
            race_culling_strip_block_count++;
            if(*((BYTE *)visual+20)==1){
                *((DWORD *)((BYTE *)visual+128))=0x0FFF;
                continue;
            }
            *((DWORD *)((BYTE *)visual+128))=0x0FFF;
            *((BYTE *)visual+20)=1;
            race_culling_visual(visual,1);
            race_culling_strip_forced++;
        }
    }
}

static void race_culling_counts(BYTE *base){
    race_culling_cuboid_count=race_culling_segment_count=0;
    if(readable(base+0x7dce7c,4)){
        void *track=*(void **)(base+0x7dce7c);
        if(readable(track,2428)){
            BYTE *begin=*(BYTE **)((BYTE *)track+2420),*end=*(BYTE **)((BYTE *)track+2424);
            if(begin&&end>=begin&&(size_t)(end-begin)<=10000*144)race_culling_cuboid_count=(unsigned)((end-begin)/144);
        }
    }
    if(readable(base+0x7e237c,4)){
        void *segments=*(void **)(base+0x7e237c);
        if(readable(segments,28)){
            BYTE *begin=*(BYTE **)((BYTE *)segments+20),*end=*(BYTE **)((BYTE *)segments+24);
            if(begin&&end>=begin&&(size_t)(end-begin)<=10000*80)race_culling_segment_count=(unsigned)((end-begin)/80);
        }
    }
}

static void race_culling_frame(void){
    if(!race_culling_enabled)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    void *manager=readable(base+0x7df324,4)?*(void **)(base+0x7df324):NULL;
    BOOL paused=race_culling_paused(base);
    race_culling_apply(manager,paused);
    race_culling_strips(base);
    if((++race_culling_frames%10)==0){race_culling_props(base);race_culling_ramps(base);}
    if(!race_culling_trace)return;
    if(!race_culling_trace_announced){
        race_culling_trace_announced=TRUE;
        log_line("Culling trace enabled.");
    }
    race_culling_counts(base);
    if((race_culling_frames%120)!=0)return;
    char line[480];
    snprintf(line,sizeof(line),"Culling trace frame %u manager=%p volumes=%u valid=%u forced=%u props=%u prop_forced=%u ramps=%u ramp_forced=%u strips=%u blocks=%u block_forced=%u cuboids=%u segments=%u paused=%d",race_culling_frames,manager,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count,race_culling_prop_count,race_culling_prop_forced,race_culling_ramp_count,race_culling_ramp_forced,race_culling_strip_count,race_culling_strip_block_count,race_culling_strip_forced,race_culling_cuboid_count,race_culling_segment_count,paused?1:0);
    log_line(line);
}
