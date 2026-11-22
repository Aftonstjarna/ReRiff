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
typedef void (__cdecl *RaceCullingTrackTick)(void *,float *);
typedef void (__cdecl *RaceCullingStripTick)(void *,BYTE,BYTE,float,void *);
typedef void *(__cdecl *RaceCullingBackNode)(void *,void **,float *);

#include "gaTrackStrip_ctor_blob.h"

static BOOL race_culling_enabled,race_culling_trace,race_culling_trace_announced,race_culling_visual_announced;
static unsigned race_culling_frames,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count,race_culling_prop_count,race_culling_prop_forced;
static unsigned race_culling_ramp_count,race_culling_ramp_forced,race_culling_cuboid_count,race_culling_segment_count;
static unsigned race_culling_strip_count,race_culling_strip_block_count,race_culling_strip_forced;
static unsigned race_culling_block_unique,race_culling_block_duplicate;
static float race_culling_block_min,race_culling_block_max;
static BOOL race_culling_pool_expanded;
static BOOL race_track_pool_relocated;
static void *race_culling_prewarm_done;
static RaceCullingVisible race_culling_force_visible,race_culling_on_visible;
static RaceCullingVisualVisible race_culling_visual;
typedef void (__cdecl *RaceCullingContextDirty)(void *);
static RaceCullingContextDirty race_culling_context_dirty;
static RaceCullingTrackTick race_culling_track_tick;
static RaceCullingStripTick race_culling_strip_tick;
static RaceCullingBackNode race_culling_back_node;
__declspec(dllexport) int __cdecl ReRiffConfigureCullingVisible(void *callback){
    if(!callback)return 0;race_culling_visual=(RaceCullingVisualVisible)callback;return 1;
}
__declspec(dllexport) int __cdecl ReRiffConfigureCullingTrackTick(void *callback){
    if(!callback)return 0;race_culling_track_tick=(RaceCullingTrackTick)callback;return 1;
}
__declspec(dllexport) int __cdecl ReRiffConfigureCullingStripTick(void *callback){
    if(!callback)return 0;race_culling_strip_tick=(RaceCullingStripTick)callback;return 1;
}
__declspec(dllexport) int __cdecl ReRiffConfigureCullingBackNode(void *callback){
    if(!callback)return 0;race_culling_back_node=(RaceCullingBackNode)callback;return 1;
}
static void race_culling_prewarm(BYTE *base){
    if(!race_culling_enabled||!race_track_pool_relocated||!race_culling_strip_tick||!race_culling_back_node
        ||!readable(base+0x7dce7c,4))return;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2728)||manager==race_culling_prewarm_done)return;
    if(!readable(base+0x7d5930,4))return;
    void *players=*(void **)(base+0x7d5930);
    if(!readable(players,52))return;
    void *list=*(void **)((BYTE *)players+48);
    if(!readable(list,4))return;
    void *local=*(void **)list;
    if(!readable(local,8))return;
    void *tracker=*(void **)((BYTE *)local+4);
    if(!readable(tracker,124))return;
    void *node=race_culling_back_node(tracker,NULL,NULL);
    if(!node)return;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return;

    /* The MethodDef patch only takes effect if the CLR had not already
     * compiled the constructor. Bail out rather than overflow a 127-slot
     * pool and make the track vanish. */
    unsigned small=0;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        int type=*((int *)(strip+212));
        if(type!=8&&type!=9)continue;
        void **pool_begin=*(void ***)(strip+164),**pool_end=*(void ***)(strip+168);
        unsigned pool=pool_begin&&pool_end&&pool_end>=pool_begin?(unsigned)(pool_end-pool_begin):0;
        if(pool<512)small++;
    }
    if(small){
        char detail[160];snprintf(detail,sizeof(detail),"Photo mode prewarm skipped: %u large strips still have a small pool.",small);log_line(detail);
        return;
    }

    float total=*((float *)((BYTE *)tracker+116)),position=*((float *)((BYTE *)tracker+120));
    float current=total>0.01f?position/total:0.0f;
    if(!(current>=0.0f&&current<=1.0f))current=0.0f;
    unsigned strips=0;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        for(int step=0;step<=100;step++)race_culling_strip_tick(strip,1,0,step/100.0f,node);
        race_culling_strip_tick(strip,1,0,current,node);
        BYTE *custom=*(BYTE **)(strip+256);
        if(race_culling_context_dirty&&custom&&readable(custom,180))race_culling_context_dirty(custom);
        strips++;
    }
    race_culling_prewarm_done=manager;
    char line[200];snprintf(line,sizeof(line),"Photo mode visual prewarm: strips=%u steps=101 progress=%.4f large_pool=512.",strips,current);log_line(line);
}

/* The constructor's own 64-slot pool is far too small for the photo-mode
 * prewarm. Relocate its IL to a copy whose three ldc.i4.s 64 sites use
 * ldc.i4 512, then point the MethodDef RVA at the copy. The original body
 * stays in place as a fallback for any path that already compiled it. */
static BOOL race_track_pool_install(BYTE *base){
    static BOOL attempted,installed;
    if(attempted)return installed;
    attempted=TRUE;
    if(!readable(base+RERIFF_TRACK_CTOR_RVA_FIELD,4)){
        log_line("Track ctor relocation: MethodDef RVA is not readable.");
        return FALSE;
    }
    DWORD *slot=(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD);
    if(*slot!=0x271e78){
        char line[160];snprintf(line,sizeof(line),"Track ctor relocation skipped: MethodDef RVA is %08X.",(unsigned)*slot);log_line(line);
        return FALSE;
    }
    /* The CLR rejects method RVAs outside the mapped image ("Method has zero
     * rva"), so the relocated body has to live inside the image. The .data
     * virtual tail at 0x7C9200..0x7FFFC0 has no absolute references in the
     * executable. Scan backwards for the last all-zero blob-sized window so
     * early runtime-initialized globals are avoided. */
    BYTE *map_begin=base+0x7c9200;
    BYTE *map_end=base+0x7fffc0;
    BYTE *memory=NULL;
    if(readable(map_begin,(size_t)(map_end-map_begin))){
        for(BYTE *candidate=map_end-RERIFF_TRACK_CTOR_BLOB_SIZE;candidate>=map_begin;candidate-=0x10){
            unsigned i=0;
            while(i<RERIFF_TRACK_CTOR_BLOB_SIZE&&candidate[i]==0)i++;
            if(i==RERIFF_TRACK_CTOR_BLOB_SIZE){memory=candidate;break;}
        }
    }
    if(!memory){
        log_line("Track ctor relocation: no zero in-image range is available.");
        return FALSE;
    }
    char range[128];snprintf(range,sizeof(range),"Track ctor in-image range=%08X.",(unsigned)((BYTE *)memory-base));log_line(range);
    DWORD old,ignored;
    if(!VirtualProtect(memory,RERIFF_TRACK_CTOR_BLOB_SIZE,PAGE_READWRITE,&old)){
        log_line("Track ctor relocation: in-image body range is not writable.");
        return FALSE;
    }
    memcpy(memory,race_track_ctor_blob,RERIFF_TRACK_CTOR_BLOB_SIZE);
    VirtualProtect(memory,RERIFF_TRACK_CTOR_BLOB_SIZE,old,&ignored);
    DWORD rva=(DWORD)(memory-base);
    if(!VirtualProtect(slot,4,PAGE_READWRITE,&old)){
        log_line("Track ctor relocation: metadata page is not writable.");
        return FALSE;
    }
    *slot=rva;
    VirtualProtect(slot,4,old,&ignored);
    FlushInstructionCache(GetCurrentProcess(),memory,RERIFF_TRACK_CTOR_BLOB_SIZE);
    char readback[128];snprintf(readback,sizeof(readback),"Track ctor RVA readback=%08X.",(unsigned)*slot);log_line(readback);
    char line[192];snprintf(line,sizeof(line),"Track ctor relocated: RVA %08X -> %08X, pool=512 for types 8/9 and 127 otherwise, blob=%u bytes.",
        0x271e78u,(unsigned)rva,(unsigned)RERIFF_TRACK_CTOR_BLOB_SIZE);log_line(line);
    installed=TRUE;
    race_track_pool_relocated=TRUE;
    return TRUE;
}

/* Managed startup calls these to force the CLR to JIT the relocated body
 * before a race exists. If the JIT rejects it, restore the original RVA and
 * leave the 127-slot fallback in place instead of crashing at song load. */
__declspec(dllexport) int __cdecl ReRiffTrackPoolRelocated(void){
    return race_track_pool_relocated?1:0;
}
__declspec(dllexport) unsigned __cdecl ReRiffTrackPoolRva(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!readable(base+RERIFF_TRACK_CTOR_RVA_FIELD,4))return 0;
    return *(unsigned *)(base+RERIFF_TRACK_CTOR_RVA_FIELD);
}
__declspec(dllexport) void __cdecl ReRiffTrackPoolValidated(void){
    if(race_track_pool_relocated)log_line("Track ctor relocation JIT-validated.");
}
__declspec(dllexport) int __cdecl ReRiffTrackPoolRestore(void){
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    DWORD *slot=(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD);
    DWORD old,ignored;
    if(!race_track_pool_relocated||!readable(slot,4))return 0;
    if(!VirtualProtect(slot,4,PAGE_READWRITE,&old))return 0;
    *slot=0x271e78;
    VirtualProtect(slot,4,old,&ignored);
    race_track_pool_relocated=FALSE;
    log_line("Track ctor relocation reverted to the original 64-slot body.");
    return 1;
}

/* gaTrackStrip.{ctor} creates three 64-entry pools: the elVisual3d vector at
 * +164, blockPoolFlags at +176, and the initialization loop bound. Raise the
 * three ldc.i4.s 64 sites to 127 before the first track load. The constructor
 * then allocates distinct visuals for the whole track instead of recycling
 * the 64-slot pool. */
static BOOL race_culling_expand_pool(BYTE *base){
    static const DWORD sites[]={0x2731c4,0x2731cd,0x273a7a};
    BYTE *addresses[3];
    for(int i=0;i<3;i++){
        addresses[i]=base+sites[i];
        if(!readable(addresses[i],2)||addresses[i][0]!=0x1f||addresses[i][1]!=0x40)return FALSE;
    }
    DWORD old,ignored;
    if(!VirtualProtect(addresses[0],sites[2]-sites[0]+2,PAGE_READWRITE,&old))return FALSE;
    for(int i=0;i<3;i++)addresses[i][1]=0x7f;
    VirtualProtect(addresses[0],sites[2]-sites[0]+2,old,&ignored);
    return TRUE;
}

/* tickBlocks retires the oldest block through the branch at IL_01e7. Turn
 * that beq into an unconditional br so blocks accumulate for photo mode;
 * the post-tick visual pass keeps their visuals visible. Applied before the
 * first race tick so the CLR has not compiled this method yet. */
static BOOL race_culling_retain_blocks(BYTE *base){
    /* tickBlocks retires every block behind the current progress through
     * this beq. Replace the num2 load with ldc.i4.m1 + nop so the compare is
     * always -1 == -1 and the erase path is skipped. Keeping beq.s (instead
     * of a raw br.s) leaves the IL evaluation stack balanced. Applied before
     * the first race tick, so the CLR has not compiled tickBlocks yet. */
    BYTE *site=base+0x26c624;
    static const BYTE expected[]={0x11,0x08,0x15,0x2e,0x33};
    if(!readable(site,sizeof(expected))||memcmp(site,expected,sizeof(expected)))return FALSE;
    DWORD old,ignored;
    if(!VirtualProtect(site,sizeof(expected),PAGE_READWRITE,&old))return FALSE;
    site[0]=0x15;
    site[1]=0x00;
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
    race_culling_context_dirty=(RaceCullingContextDirty)(base+0x53580);
    BOOL expanded=race_culling_expand_pool(base);
    race_culling_pool_expanded=expanded;
    WCHAR pool_value[16];
    GetPrivateProfileStringW(L"graphics",L"track_pool_512",L"true",pool_value,16,client);
    BOOL use_512=!_wcsicmp(pool_value,L"true")||!_wcsicmp(pool_value,L"yes")||!wcscmp(pool_value,L"1");
    BOOL relocated=use_512?race_track_pool_install(base):FALSE;
    BOOL retained=race_culling_retain_blocks(base);
    char detail[192];snprintf(detail,sizeof(detail),"Track culling disable enabled; trace=%d; block retention=%d; visual pool=%s.",race_culling_trace?1:0,retained?1:0,relocated?"512/127":"127");log_line(detail);
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
    static BYTE seen[10000];
    race_culling_strip_count=race_culling_strip_block_count=race_culling_strip_forced=0;
    race_culling_block_unique=race_culling_block_duplicate=0;
    race_culling_block_min=race_culling_block_max=0;
    memset(seen,0,sizeof(seen));
    if(!race_culling_visual||!readable(base+0x7dce7c,4))return;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2732))return;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return;
    race_culling_strip_count=(unsigned)(end-begin);
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        /* The custom road/wall/zone mesh is one elVisualCustom at strip+256.
         * Prewarming fills the mesh, but its bounding volume is left at the
         * last spawned segment, so the backed-out camera culls the whole
         * surface. Keep it visible and clear the setCullable bit (137.0). */
        BYTE *custom=*(BYTE **)(strip+256);
        if(custom&&readable(custom,180)){
            *((BYTE *)custom+20)=1;
            *((BYTE *)custom+137)&=(BYTE)~1;
            *((DWORD *)((BYTE *)custom+128))=0x0FFF;
            BYTE *shader=*(BYTE **)(custom+152);
            if(shader&&readable(shader,1592)){
                if(race_culling_trace&&(race_culling_frames%120)==0){
                    char window[200];snprintf(window,sizeof(window),"Culling mesh type=%d shader_window=%.3f..%.3f fade=%.3f..%.3f.",
                        *((int *)(strip+212)),*((float *)(shader+1576)),*((float *)(shader+1580)),
                        *((float *)(shader+1584)),*((float *)(shader+1588)));log_line(window);
                }
                *((float *)(shader+1576))=0.0f;
                *((float *)(shader+1580))=100000.0f;
                *((float *)(shader+1584))=0.0f;
                *((float *)(shader+1588))=100000.0f;
                *((float *)(shader+1604))=1.0f;
                *((float *)(shader+1608))=1.0f;
                *((float *)(shader+1612))=1.0f;
            }
        }
        if(race_culling_trace&&(race_culling_frames%120)==0){
            void **pool_begin=*(void ***)(strip+164),**pool_end=*(void ***)(strip+168);
            unsigned pool=pool_begin&&pool_end&&pool_end>=pool_begin?(unsigned)(pool_end-pool_begin):0;
            unsigned mesh_v=0,mesh_t=0;BYTE mesh_visible=0;
            BYTE *custom=*(BYTE **)(strip+256);
            if(custom&&readable(custom,180)){
                BYTE *vb=*(BYTE **)(custom+156),*ve=*(BYTE **)(custom+160);
                BYTE *tb=*(BYTE **)(custom+168),*te=*(BYTE **)(custom+172);
                if(vb&&ve&&ve>=vb)mesh_v=(unsigned)((ve-vb)/40);
                if(tb&&te&&te>=tb)mesh_t=(unsigned)((te-tb)/12);
                mesh_visible=*((BYTE *)custom+20);
            }
            char detail[200];snprintf(detail,sizeof(detail),"Culling strip type=%d pool=%u mesh_v=%u mesh_t=%u mesh_visible=%u.",*((int *)(strip+212)),pool,mesh_v,mesh_t,mesh_visible);log_line(detail);
        }
        BYTE *blocks=*(BYTE **)(strip+152),*blocks_end=*(BYTE **)(strip+156);
        if(!blocks||!blocks_end||blocks_end<blocks||(size_t)(blocks_end-blocks)>10000*88
            ||((size_t)(blocks_end-blocks)%88)||!readable(blocks,(size_t)(blocks_end-blocks)))continue;
        void **visuals=*(void ***)(strip+164);
        if(!visuals||!readable(visuals,4))continue;
        unsigned strip_blocks=0,strip_unique=0;BYTE strip_seen[256]={0};
        for(BYTE *block=blocks;block<blocks_end;block+=88){
            int index=*((int *)(block+80));
            if(index<0||index>=10000)continue;
            strip_blocks++;
            if(index<256&&!strip_seen[index]){strip_seen[index]=1;strip_unique++;}
            float progress=*((float *)(block+76));
            if(race_culling_strip_block_count==0||progress<race_culling_block_min)race_culling_block_min=progress;
            if(race_culling_strip_block_count==0||progress>race_culling_block_max)race_culling_block_max=progress;
            if(seen[index])race_culling_block_duplicate++;
            else{seen[index]=1;race_culling_block_unique++;}
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
        if(race_culling_trace&&(race_culling_frames%120)==0){
            void **pool_begin=*(void ***)(strip+164),**pool_end=*(void ***)(strip+168);
            unsigned pool=pool_begin&&pool_end&&pool_end>=pool_begin?(unsigned)(pool_end-pool_begin):0;
            char detail[160];snprintf(detail,sizeof(detail),"Culling strip type=%d blocks=%u unique=%u current=%d pool=%u.",*((int *)(strip+212)),strip_blocks,strip_unique,*((int *)(strip+188)),pool);log_line(detail);
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
    char line[560];
    snprintf(line,sizeof(line),"Culling trace frame %u manager=%p volumes=%u valid=%u forced=%u props=%u prop_forced=%u ramps=%u ramp_forced=%u strips=%u blocks=%u block_forced=%u block_span=%.3f..%.3f block_unique=%u block_dup=%u cuboids=%u segments=%u paused=%d",race_culling_frames,manager,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count,race_culling_prop_count,race_culling_prop_forced,race_culling_ramp_count,race_culling_ramp_forced,race_culling_strip_count,race_culling_strip_block_count,race_culling_strip_forced,race_culling_block_min,race_culling_block_max,race_culling_block_unique,race_culling_block_duplicate,race_culling_cuboid_count,race_culling_segment_count,paused?1:0);
    log_line(line);
}
