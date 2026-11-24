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
static unsigned race_building_strips,race_building_blocks,race_building_visible;
static BOOL race_retain_buildings;
static float race_culling_block_min,race_culling_block_max;
static BOOL race_culling_pool_expanded;
static BOOL race_track_pool_relocated;
static BYTE *race_track_pool_base;
static int race_retention_tier;
static int race_retention_pool;
static int race_retention_block_cap;
static int race_window_behind,race_window_ahead;
static float race_player_distance;
static int race_track_block_total;
static void *race_culling_prewarm_done;
static void *race_culling_prewarm_road;
static RaceCullingVisible race_culling_force_visible,race_culling_on_visible;
static RaceCullingVisualVisible race_culling_visual;
typedef void (__cdecl *RaceCullingContextDirty)(void *);
static RaceCullingContextDirty race_culling_context_dirty;
static RaceCullingTrackTick race_culling_track_tick;
static RaceCullingStripTick race_culling_strip_tick;
static RaceCullingBackNode race_culling_back_node;
typedef void *(__cdecl *RaceCullingStripCtor)(void *,int);
typedef void (__thiscall *RaceCullingSetCollision)(void *,BYTE);
typedef void *(__cdecl *RaceCullingOperatorNew)(size_t);
typedef void (__cdecl *RaceCullingOperatorDelete)(void *);
/* The capture buffer must hold the whole track: the road mesh wraps at the
 * capacity, so a short buffer overwrites the start with the end of the track.
 * 131072 vertices covers ~14500 blocks; the pre-race window is reused. */
#define RACE_CAPTURE_VERTEX_CAP 131072u
#define RACE_ROAD_CHUNK_MAX 128
static RaceCullingStripCtor race_culling_strip_ctor;
static RaceCullingSetCollision race_culling_set_collision;
static RaceCullingOperatorNew race_culling_operator_new;
static RaceCullingOperatorDelete race_culling_operator_delete;
static BYTE *race_capture_vertices;
static unsigned race_capture_vertex_capacity;
static void *race_culling_road_chunks[RACE_ROAD_CHUNK_MAX];
static unsigned race_culling_road_chunk_count;
/* Side type-6 strips reuse the original sign tick at a shifted pace time.
 * Each one adds a forward window of scrolling base/glow signs without
 * touching the original strip's +268/+300 arrays or its destructor. */
static void *race_sign_strips[4];
static int race_sign_strip_count;
static void *race_sign_manager;
static BOOL race_sign_announced;
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
__declspec(dllexport) int __cdecl ReRiffConfigureCullingStripCtor(void *callback){
    if(!callback)return 0;race_culling_strip_ctor=(RaceCullingStripCtor)callback;return 1;
}
static void race_culling_force_custom(BYTE *strip,BOOL visible){
    BYTE *custom=*(BYTE **)(strip+256);
    if(!custom||!readable(custom,180))return;
    *((BYTE *)custom+20)=visible?1:0;
    if(!visible)return;
    *((BYTE *)custom+137)&=(BYTE)~1;
    *((DWORD *)((BYTE *)custom+128))=0x0FFF;
    BYTE *shader=*(BYTE **)(custom+152);
    if(shader&&readable(shader,1592)){
        *((float *)(shader+1576))=0.0f;
        *((float *)(shader+1580))=100000.0f;
        *((float *)(shader+1584))=0.0f;
        *((float *)(shader+1588))=100000.0f;
    }
    /* The road rails/signs are the elSSP<elVisual3d> arrays at +268 and +300. */
    for(int off=268;off<=300;off+=32){
        for(int i=0;i<8;i++){
            void **slot=(void **)(strip+off+i*4);
            if(!readable(slot,4))continue;
            void *rail=*slot;
            if(!rail||!readable(rail,132))continue;
            *((BYTE *)rail+20)=visible?1:0;
            if(!visible)continue;
            *((BYTE *)rail+137)&=(BYTE)~1;
            *((DWORD *)((BYTE *)rail+128))=0x0FFF;
        }
    }
}
static void race_culling_road_chunks_split(BYTE *base,BYTE *main,BYTE *capture_custom,int vertex_start,int vertex_end,int master_total,void *node){
    (void)base;
    if(!race_culling_strip_ctor||!capture_custom||master_total<=0)return;
    BYTE *main_custom=*(BYTE **)(main+256);
    if(!main_custom||!readable(main_custom,180)){
        log_line("Road chunks skipped: live road visual is not readable.");
        return;
    }
    BYTE *cvb=*(BYTE **)(capture_custom+156),*cve=*(BYTE **)(capture_custom+160);
    BYTE *ctb=*(BYTE **)(capture_custom+168),*cte=*(BYTE **)(capture_custom+172);
    if(!cvb||!cve||cve<cvb||!ctb||!cte||cte<ctb){
        log_line("Road chunks skipped: capture mesh is not readable.");
        return;
    }
    unsigned total_verts=(unsigned)((cve-cvb)/40);
    unsigned used=(unsigned)master_total*9;
    if(used>total_verts)used=total_verts;
    unsigned total_tris=(unsigned)((cte-ctb)/12);
    if(vertex_start<0)vertex_start=0;
    if(vertex_end>(int)used)vertex_end=(int)used;
    if(vertex_start>=vertex_end)return;
    const unsigned chunk_verts=2000,overlap=32,stride=chunk_verts-overlap;
    unsigned added=0;
    for(unsigned start=(unsigned)vertex_start;start<(unsigned)vertex_end;start+=stride){
        if(race_culling_road_chunk_count>=RACE_ROAD_CHUNK_MAX)break;
        unsigned end=start+chunk_verts;
        if(end>(unsigned)vertex_end)end=(unsigned)vertex_end;
        unsigned ext_start=start>overlap?start-overlap:0;
        unsigned ext_end=end+overlap;
        if(ext_end>used)ext_end=used;
        unsigned nt=0;
        for(unsigned t=0;t<total_tris;t++){
            int *tri=(int *)(ctb+t*12);
            int a=tri[0],b=tri[1],c=tri[2];
            if(a>=(int)ext_start&&a<(int)ext_end&&b>=(int)ext_start&&b<(int)ext_end&&c>=(int)ext_start&&c<(int)ext_end)nt++;
        }
        if(!nt)continue;
        BYTE *extra=(BYTE *)VirtualAlloc(NULL,424,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(!extra)break;
        if(!race_culling_strip_ctor(extra,0))continue;
        void *body=*(void **)(extra+264);
        if(body&&race_culling_set_collision)race_culling_set_collision(body,0);
        /* Position this chunk's rail/sign visuals over its block range. */
        memcpy(extra+64,main+64,72);
        memcpy(extra+136,main+136,12);
        memcpy(extra+216,main+216,16);
        int start_block=(int)(ext_start/9);
        int end_block=(int)(ext_end/9);
        *((int *)(extra+56))=start_block;
        *((int *)(extra+48))=0;
        float chunk_progress=master_total>0?(float)end_block/(float)master_total:0.0f;
        if(chunk_progress>1.0f)chunk_progress=1.0f;
        race_culling_strip_tick(extra,1,0,chunk_progress,node);
        BYTE *custom=*(BYTE **)(extra+256);
        if(!custom||!readable(custom,180))continue;
        memcpy(custom+32,main_custom+32,16);
        if(readable(main_custom+152,4))*(DWORD *)(custom+152)=*(DWORD *)(main_custom+152);
        unsigned vcount=ext_end-ext_start;
        BYTE *vbuf=race_culling_operator_new?(BYTE *)race_culling_operator_new((size_t)vcount*40):NULL;
        BYTE *tbuf=race_culling_operator_new?(BYTE *)race_culling_operator_new((size_t)nt*2*12):NULL;
        if(!vbuf||!tbuf)continue;
        memcpy(vbuf,cvb+(size_t)ext_start*40,(size_t)vcount*40);
        unsigned k=0;
        for(unsigned t=0;t<total_tris;t++){
            int *tri=(int *)(ctb+t*12);
            int a=tri[0],b=tri[1],c=tri[2];
            if(a<(int)ext_start||a>=(int)ext_end||b<(int)ext_start||b>=(int)ext_end||c<(int)ext_start||c>=(int)ext_end)continue;
            /* Keep the original face and add the reversed one so the chunk is
             * visible from below (the source road mesh is single-sided). */
            int *dst=(int *)(tbuf+(size_t)(k*2)*12);
            dst[0]=a-(int)ext_start;
            dst[1]=b-(int)ext_start;
            dst[2]=c-(int)ext_start;
            dst=(int *)(tbuf+(size_t)(k*2+1)*12);
            dst[0]=a-(int)ext_start;
            dst[1]=c-(int)ext_start;
            dst[2]=b-(int)ext_start;
            k++;
        }
        *(BYTE **)(custom+156)=vbuf;
        *(BYTE **)(custom+160)=vbuf+(size_t)vcount*40;
        *(BYTE **)(custom+164)=vbuf+(size_t)vcount*40;
        *(BYTE **)(custom+168)=tbuf;
        *(BYTE **)(custom+172)=tbuf+(size_t)nt*2*12;
        *(BYTE **)(custom+176)=tbuf+(size_t)nt*2*12;
        if(race_culling_context_dirty){
            BOOL have_next=readable(custom+12,4);
            DWORD next=have_next?*(DWORD *)(custom+12):0;
            if(have_next)*(DWORD *)(custom+12)=0;
            race_culling_context_dirty(custom);
            if(have_next)*(DWORD *)(custom+12)=next;
        }
        race_culling_force_custom(extra,1);
        char detail[200];snprintf(detail,sizeof(detail),"Road chunk %u exact verts=%u..%u tris=%u cursor=%d.",
            race_culling_road_chunk_count,ext_start,ext_end,nt*2,extra?*((int *)(extra+56)):0);log_line(detail);
        race_culling_road_chunks[race_culling_road_chunk_count++]=extra;
        added++;
    }
    char detail[180];snprintf(detail,sizeof(detail),"Road chunks built: range=%d..%d total_blocks=%d added=%u all=%u.",
        vertex_start,vertex_end,master_total,added,race_culling_road_chunk_count);log_line(detail);
}
static void race_culling_road_chunks_apply(void){
    for(unsigned i=0;i<race_culling_road_chunk_count;i++)race_culling_force_custom((BYTE *)race_culling_road_chunks[i],1);
}

/* A chunk owns its mesh buffers but not a game object, so hide it and return
 * the two large allocations when the race manager changes. Leaving them made
 * every prewarm leak the previous track's road chunks. */
static void race_culling_road_chunk_release(void *chunk){
    BYTE *strip=(BYTE *)chunk;
    if(!strip||!readable(strip,424))return;
    BYTE *custom=*(BYTE **)(strip+256);
    if(!custom||!readable(custom,180))return;
    *((BYTE *)custom+20)=0;
    BYTE *vertices=*(BYTE **)(custom+156),*triangles=*(BYTE **)(custom+168);
    *(BYTE **)(custom+156)=NULL;
    *(BYTE **)(custom+160)=NULL;
    *(BYTE **)(custom+164)=NULL;
    *(BYTE **)(custom+168)=NULL;
    *(BYTE **)(custom+172)=NULL;
    *(BYTE **)(custom+176)=NULL;
    if(race_culling_operator_delete){
        if(vertices)race_culling_operator_delete(vertices);
        if(triangles)race_culling_operator_delete(triangles);
    }
}

static void race_culling_prewarm(BYTE *base){
    if(!race_culling_enabled||!race_track_pool_relocated||!race_culling_strip_tick||!race_culling_back_node
        ||!readable(base+0x7dce7c,4))return;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2728))return;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return;
    /* The track manager is a singleton reused by the menu preview and the
     * race, so the manager pointer alone is not enough: a short preview track
     * would leave its 720-block chunks in place for a 5000-block race. Each
     * gaTrackManager.load creates new gaTrackStrip objects, so key the cache on
     * the live type-0 strip too. */
    BYTE *road=NULL;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(readable(strip,424)&&*((int *)(strip+212))==0){road=strip;break;}
    }
    if(!road||(manager==race_culling_prewarm_done&&road==race_culling_prewarm_road))return;
    race_track_block_total=0;
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

    unsigned small=0;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        int type=*((int *)(strip+212));
        if(type!=8&&type!=9)continue;
        void **pool_begin=*(void ***)(strip+164),**pool_end=*(void ***)(strip+168);
        unsigned pool=pool_begin&&pool_end&&pool_end>=pool_begin?(unsigned)(pool_end-pool_begin):0;
        if(pool<(unsigned)race_retention_pool)small++;
    }
    if(small){
        char detail[160];snprintf(detail,sizeof(detail),"Track prewarm skipped: %u large strips still have a small pool.",small);log_line(detail);
        return;
    }
    if(race_culling_road_chunk_count){
        for(unsigned i=0;i<race_culling_road_chunk_count;i++)race_culling_road_chunk_release(race_culling_road_chunks[i]);
        char detail[160];snprintf(detail,sizeof(detail),"Track prewarm reset: released %u chunks for a new track.",race_culling_road_chunk_count);log_line(detail);
        race_culling_road_chunk_count=0;
    }

    float total=*((float *)((BYTE *)tracker+116)),position=*((float *)((BYTE *)tracker+120));
    float current=total>0.01f?position/total:0.0f;
    if(!(current>=0.0f&&current<=1.0f))current=0.0f;
    int steps=race_retention_tier>0?race_retention_tier*10:100;
    float max_progress=(float)steps/100.0f;
    if(current>max_progress)current=max_progress;
    unsigned strips=0;
    BYTE *main_road=NULL;
    int main_cursor=0;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        int type=*((int *)(strip+212));
        if(type==0){
            /* Do not sweep the live road strip; its current window is the
             * near-camera surface that must stay intact. */
            main_road=strip;
            main_cursor=*((int *)(strip+56));
            strips++;
            continue;
        }
        if(type==6||type==10){
            /* Scrolling sign rails, score zones and special blocks stay vanilla. */
            strips++;
            continue;
        }
        /* Never sweep a live strip across the track. Every strip keeps its
         * spawn cursor, and the cursor only advances: a full-track sweep leaves
         * it at the end, so walls, chevrons and score-zone edge meshes stay
         * built at the far end while the car drives through an empty track.
         * Only the throwaway capture strip below is swept to build the road
         * chunks. Live strips spawn naturally with the car, and photo mode
         * advances them with the camera. */
        race_culling_strip_tick(strip,1,0,current,node);
        BYTE *custom=*(BYTE **)(strip+256);
        if(race_culling_context_dirty&&custom&&readable(custom,180)){
            BOOL have_next=readable(custom+12,4);
            DWORD next=have_next?*(DWORD *)(custom+12):0;
            if(have_next)*(DWORD *)(custom+12)=0;
            race_culling_context_dirty(custom);
            if(have_next)*(DWORD *)(custom+12)=next;
        }
        strips++;
    }

    int master_total=0;
    BYTE *capture_custom=NULL;
    if(main_road&&race_culling_strip_ctor){
        BYTE *capture=(BYTE *)VirtualAlloc(NULL,424,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        if(capture&&race_culling_strip_ctor(capture,0)){
            memcpy(capture+64,main_road+64,72);
            memcpy(capture+136,main_road+136,12);
            memcpy(capture+216,main_road+216,16);
            *((int *)(capture+56))=0;
            *((int *)(capture+48))=0;
            void *body=*(void **)(capture+264);
            if(body&&race_culling_set_collision)race_culling_set_collision(body,0);
            capture_custom=*(BYTE **)(capture+256);
            if(capture_custom&&readable(capture_custom,180)){
                const unsigned cap=RACE_CAPTURE_VERTEX_CAP;
                if(race_capture_vertex_capacity<cap){
                    if(race_capture_vertices&&race_culling_operator_delete)race_culling_operator_delete(race_capture_vertices);
                    race_capture_vertices=race_culling_operator_new?(BYTE *)race_culling_operator_new((size_t)cap*40):NULL;
                    race_capture_vertex_capacity=race_capture_vertices?cap:0;
                }
                BYTE *vbig=race_capture_vertices;
                if(vbig){
                    *(BYTE **)(capture_custom+156)=vbig;
                    *(BYTE **)(capture_custom+160)=vbig+(size_t)cap*40;
                    *(BYTE **)(capture_custom+164)=vbig+(size_t)cap*40;
                }
            }
            for(int step=0;step<=steps;step++)race_culling_strip_tick(capture,1,0,(float)step/(float)steps*max_progress,node);
            master_total=*((int *)(capture+56));
            race_track_block_total=master_total;
            if(master_total>0&&(unsigned)master_total*9>race_capture_vertex_capacity){
                char detail[180];snprintf(detail,sizeof(detail),"Road capture truncated: need=%u cap=%u; the road mesh wrapped.",
                    (unsigned)master_total*9,race_capture_vertex_capacity);log_line(detail);
            }
            if(capture_custom&&readable(capture_custom,180))*((BYTE *)capture_custom+20)=0;
            for(int off=268;off<=300;off+=32){
                for(int i=0;i<8;i++){
                    void *visual=*(void **)(capture+off+i*4);
                    if(visual&&readable(visual,132))*((BYTE *)visual+20)=0;
                }
            }
            char detail[180];snprintf(detail,sizeof(detail),"Road capture: total_blocks=%d main_cursor=%d verts=%u cap=%u.",
                master_total,main_cursor,master_total>0?(unsigned)master_total*9:0,race_capture_vertex_capacity);log_line(detail);
        }
    }
    if(main_road&&master_total>0&&capture_custom){
        /* Cover the whole road, including the pre-start blocks. The old split
         * skipped [0, main_cursor*9], assuming the live strip covered it, so
         * retention only began at the race start line. Hide the live road mesh
         * once the chunks exist so the two surfaces do not overlap. */
        race_culling_road_chunks_split(base,main_road,capture_custom,0,master_total*9,master_total,node);
        race_culling_force_custom(main_road,0);
    }else{
        log_line("Track prewarm deferred: road data is not ready.");
        return;
    }
    race_culling_prewarm_done=manager;
    race_culling_prewarm_road=main_road;
    char line[260];snprintf(line,sizeof(line),"Track prewarm: manager=%p road=%p strips=%u progress=%.4f main_cursor=%d total=%d chunks=%u.",
        manager,main_road,strips,current,main_cursor,master_total,race_culling_road_chunk_count);log_line(line);
}

/* The constructor's own 64-slot pool is far too small for the photo-mode
 * prewarm. Relocate its IL to a copy whose three ldc.i4.s 64 sites use the
 * tier-sized pool, then point the MethodDef RVA at the copy. The original body
 * stays in place, expanded to 127 slots, as a fallback for any path that
 * already compiled it or if the copy cannot be installed safely.
 *
 * A method RVA must stay inside the mapped PE image, and the rewritten body is
 * 10760 bytes, so it fits no inter-section gap. The zero tail of .data looked
 * free, but the game's static objects and arenas reuse it; the JIT then read
 * an overwritten header and track load failed with BadImageFormatException
 * "Invalid method header format 1". Place the copy in .reloc instead: the
 * loader consumes the base relocation records before this runs, no runtime
 * code reads them again, and the section lies inside both SizeOfRawData and
 * VirtualSize for every PE reader. */
static BYTE *race_track_ctor_image_slot(BYTE *base){
    /* .reloc spans RVA 0x854000..0x885000. Stay clear of its first page so
     * the choice does not depend on the blob size. */
    static const DWORD rva=0x855000;
    BYTE *memory=base+rva;
    return readable(memory,RERIFF_TRACK_CTOR_BLOB_SIZE)?memory:NULL;
}
static BOOL race_track_pool_install(BYTE *base,int pool){
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
    BYTE *memory=race_track_ctor_image_slot(base);
    if(!memory){
        log_line("Track ctor relocation: .reloc image range is unavailable; keeping the 127-slot body.");
        return FALSE;
    }
    char range[128];snprintf(range,sizeof(range),"Track ctor .reloc range=%08X.",(unsigned)((BYTE *)memory-base));log_line(range);
    DWORD old,ignored;
    if(!VirtualProtect(memory,RERIFF_TRACK_CTOR_BLOB_SIZE,PAGE_READWRITE,&old)){
        log_line("Track ctor relocation: in-image body range is not writable.");
        return FALSE;
    }
    static BYTE body[RERIFF_TRACK_CTOR_BLOB_SIZE];
    memcpy(body,race_track_ctor_blob,sizeof(body));
    /* Give the type-2/3 wall custom mesh room for the whole driven edge. */
    for(unsigned i=0;i+5<=sizeof(body);i++){
        if(body[i]==0x20&&body[i+1]==0x58&&body[i+2]==0x06&&body[i+3]==0x00&&body[i+4]==0x00){
            *(int *)(body+i+1)=65536;break;
        }
    }
    if(pool!=512){
        int replaced=0;
        for(unsigned i=0;i+5<=sizeof(body);i++){
            if(body[i]==0x20&&body[i+1]==0x00&&body[i+2]==0x02&&body[i+3]==0x00&&body[i+4]==0x00){
                *(int *)(body+i+1)=pool;replaced++;break;
            }
        }
        if(replaced!=1)log_line("Track ctor pool patch: expected one 512 immediate; using the blob value.");
    }
    memcpy(memory,body,sizeof(body));
    VirtualProtect(memory,RERIFF_TRACK_CTOR_BLOB_SIZE,old,&ignored);
    /* The CLR reads this header at first JIT. Refuse to redirect the MethodDef
     * if the copy did not survive the write; the 127-slot original remains. */
    if(memcmp(memory,body,sizeof(body))||(memory[0]&3)!=3){
        log_line("Track ctor relocation: .reloc body did not verify; keeping the 127-slot body.");
        return FALSE;
    }
    DWORD rva=(DWORD)(memory-base);
    if(!VirtualProtect(slot,4,PAGE_READWRITE,&old)){
        log_line("Track ctor relocation: metadata page is not writable.");
        return FALSE;
    }
    *slot=rva;
    VirtualProtect(slot,4,old,&ignored);
    FlushInstructionCache(GetCurrentProcess(),memory,RERIFF_TRACK_CTOR_BLOB_SIZE);
    char readback[128];snprintf(readback,sizeof(readback),"Track ctor RVA readback=%08X.",(unsigned)*slot);log_line(readback);
    char line[192];snprintf(line,sizeof(line),"Track ctor relocated: RVA %08X -> %08X, large pool=%d, small pool=127, blob=%u bytes.",
        0x271e78u,(unsigned)rva,pool,(unsigned)RERIFF_TRACK_CTOR_BLOB_SIZE);log_line(line);
    installed=TRUE;
    race_track_pool_base=base;
    race_track_pool_relocated=TRUE;
    return TRUE;
}

/* Managed startup calls these to have the runtime parse the relocated header
 * before a race exists. If it rejects the copy, restore the original RVA and
 * leave the 127-slot in-place body as the fallback instead of crashing at song
 * load. */
__declspec(dllexport) int __cdecl ReRiffTrackPoolRelocated(void){
    return race_track_pool_relocated?1:0;
}
__declspec(dllexport) unsigned __cdecl ReRiffTrackPoolRva(void){
    BYTE *base=race_track_pool_base?race_track_pool_base:(BYTE *)GetModuleHandleW(NULL);
    if(!readable(base+RERIFF_TRACK_CTOR_RVA_FIELD,4))return 0;
    return *(unsigned *)(base+RERIFF_TRACK_CTOR_RVA_FIELD);
}
__declspec(dllexport) void __cdecl ReRiffTrackPoolValidated(void){
    if(race_track_pool_relocated)log_line("Track ctor relocation validated.");
}
__declspec(dllexport) int __cdecl ReRiffTrackPoolRestore(void){
    BYTE *base=race_track_pool_base?race_track_pool_base:(BYTE *)GetModuleHandleW(NULL);
    DWORD *slot=(DWORD *)(base+RERIFF_TRACK_CTOR_RVA_FIELD);
    DWORD old,ignored;
    if(!race_track_pool_relocated||!readable(slot,4))return 0;
    if(!VirtualProtect(slot,4,PAGE_READWRITE,&old))return 0;
    *slot=0x271e78;
    VirtualProtect(slot,4,old,&ignored);
    race_track_pool_relocated=FALSE;
    log_line("Track ctor relocation reverted to the 127-slot in-place body.");
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
    /* tickBlocks retires blocks behind the current progress through this beq.
     * Replace the num2 load with ldc.i4.m1 + nop so the compare is always
     * -1 == -1 and the erase path is skipped. Keeping beq.s (instead of a raw
     * br.s) leaves the IL evaluation stack balanced. tickSpecialBlocks is left
     * vanilla so destructible special blocks (the barriers across the track)
     * still retire after they are hit. */
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

/* Edge barriers are wall segments appended to the type-2/3 custom mesh.
 * spawnNewWallSegment erases the triangles behind the forward-only cursor, and
 * the ctor allocates only 1624 wall vertices, so the barrier vanishes just
 * behind the car. Enlarge the vertex pool and skip the erase: the wall then
 * accumulates over the driven track and stays visible in free look/photo. */
static BOOL race_culling_retain_walls(BYTE *base){
    BOOL ok=FALSE;
    BYTE *pool=base+0x27306c;
    static const BYTE pool_expected[]={0x20,0x58,0x06,0x00,0x00};
    if(readable(pool,sizeof(pool_expected))&&!memcmp(pool,pool_expected,sizeof(pool_expected))){
        DWORD old,ignored;
        if(VirtualProtect(pool,sizeof(pool_expected),PAGE_READWRITE,&old)){
            *(int *)(pool+1)=65536;
            VirtualProtect(pool,sizeof(pool_expected),old,&ignored);
            ok=TRUE;
        }
    }
    BYTE *erase=base+0x26e93d;
    static const BYTE erase_expected[]={0x11,0x04,0x2d,0x3e};
    if(readable(erase,sizeof(erase_expected))&&!memcmp(erase,erase_expected,sizeof(erase_expected))){
        DWORD old,ignored;
        if(VirtualProtect(erase,sizeof(erase_expected),PAGE_READWRITE,&old)){
            erase[0]=0x2b;erase[1]=0x40;erase[2]=0x00;erase[3]=0x00;
            VirtualProtect(erase,sizeof(erase_expected),old,&ignored);
            ok=TRUE;
        }
    }
    return ok;
}

/* Barricades and ramps use a hard-coded -30 behind window: the wall is hidden
 * and its pooled prop is released as soon as the player is 30 units past it.
 * A ramp throws the car forward, which pushes the ramp-side walls past that
 * window while they are still beside the car, so they vanish early and the
 * prop is recycled. Raise the inline -30.0 literal to the retained behind
 * window so those walls stay attached and visible. */
static BOOL race_culling_widen_behind(BYTE *base,double behind){
    static const DWORD sites[]={0x23afd7,0x23c940};
    BOOL ok=FALSE;
    for(int i=0;i<2;i++){
        BYTE *site=base+sites[i];
        if(!readable(site,sizeof(double)))continue;
        double current,value=-behind;
        memcpy(&current,site,sizeof(current));
        if(current==value){ok=TRUE;continue;}
        if(current!=-30.0)continue;
        DWORD old,ignored;
        if(!VirtualProtect(site,sizeof(value),PAGE_READWRITE,&old))continue;
        memcpy(site,&value,sizeof(value));
        VirtualProtect(site,sizeof(value),old,&ignored);
        ok=TRUE;
    }
    return ok;
}

static BOOL install_race_culling(BYTE *base){
    WCHAR value[16];
    WCHAR client[MAX_PATH];wcsncpy(client,config_path,MAX_PATH-1);client[MAX_PATH-1]=0;
    WCHAR *slash=wcsrchr(client,L'\\');if(slash)wcscpy(slash+1,L"client.ini");
    GetPrivateProfileStringW(L"graphics",L"disable_culling",L"false",value,16,client);
    BOOL disable_culling=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    int tier=GetPrivateProfileIntW(L"graphics",L"visual_retention",0,client);
    if(tier<0)tier=0;if(tier>10)tier=10;
    race_retention_tier=tier;
    BOOL visual_retention=tier>0;
    race_culling_enabled=disable_culling||visual_retention;
    reriff_two_sided=race_culling_enabled;
    GetPrivateProfileStringW(L"camera",L"race_orbit_trace",L"false",value,16,client);
    race_culling_trace=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    GetPrivateProfileStringW(L"graphics",L"retain_buildings",L"false",value,16,client);
    race_retain_buildings=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    if(!race_culling_enabled)return TRUE;
    if(!readable(base+0x278ea0,8)||!readable(base+0x27959c,8))return FALSE;
    race_culling_force_visible=(RaceCullingVisible)(base+0x278ea0);
    race_culling_on_visible=(RaceCullingVisible)(base+0x27959c);
    race_culling_context_dirty=(RaceCullingContextDirty)(base+0x53580);
    race_culling_set_collision=(RaceCullingSetCollision)(base+0x3bc80);
    if(readable(base+0x52d4b4,4))race_culling_operator_new=(RaceCullingOperatorNew)*(void **)(base+0x52d4b4);
    if(readable(base+0x52d4b0,4))race_culling_operator_delete=(RaceCullingOperatorDelete)*(void **)(base+0x52d4b0);
    BOOL expanded=race_culling_expand_pool(base);
    race_culling_pool_expanded=expanded;
    WCHAR pool_value[16];
    GetPrivateProfileStringW(L"graphics",L"track_pool_512",L"true",pool_value,16,client);
    BOOL use_pool=visual_retention||!_wcsicmp(pool_value,L"true")||!_wcsicmp(pool_value,L"yes")||!wcscmp(pool_value,L"1");
    int pool=visual_retention?128+((tier-1)*896)/9:512;
    race_retention_pool=pool;
    race_retention_block_cap=visual_retention?pool:0;
    race_window_behind=visual_retention?pool/2:0;
    race_window_ahead=visual_retention?pool-race_window_behind:0;
    BOOL behind=race_culling_widen_behind(base,race_window_behind>60?race_window_behind:120);
    BOOL relocated=use_pool?race_track_pool_install(base,pool):FALSE;
    BOOL retained=race_culling_retain_blocks(base);
    BOOL walls=race_culling_retain_walls(base);
    char detail[300];snprintf(detail,sizeof(detail),"Track visual retention tier=%d; trace=%d; block retention=%d; wall retention=%d; release window=%d; buildings=%s; large pool=%d; window=%d behind + %d ahead.",
        tier,race_culling_trace?1:0,retained?1:0,walls?1:0,behind?1:0,race_retain_buildings?"retained":"vanilla",relocated?pool:0,race_window_behind,race_window_ahead);log_line(detail);
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

#define RACE_SNAPSHOT_MAX 1024
static void *race_prop_snapshot[RACE_SNAPSHOT_MAX],*race_prop_vtable[RACE_SNAPSHOT_MAX];
static unsigned race_prop_snapshot_count;
static void *race_node_snapshot[RACE_SNAPSHOT_MAX],*race_node_vtable[RACE_SNAPSHOT_MAX];
static unsigned race_node_snapshot_count;
static void *race_prop_manager;

/* gaPropManager.tick drops props/nodes from the active vectors once their own
 * tick/setup call says they are inactive. The object is still alive (usually
 * asleep), and the vector keeps its capacity, so re-append the snapshot entry
 * at the current end when it is missing. The vtable check rejects entries that
 * were destroyed and reused. */
static unsigned race_culling_readd(void **vector,size_t object_size,void **snapshot,void **vtable,unsigned *count,size_t dead_offset){
    if(!vector)return 0;
    void **begin=(void **)vector[0],**end=(void **)vector[1],**cap=(void **)vector[2];
    if(!begin||!end||!cap||end<begin||cap<end)return 0;
    if((size_t)((BYTE *)cap-(BYTE *)begin)>20000*4)return 0;
    unsigned readded=0;
    for(void **item=begin;item<end;item++){
        void *object=*item;
        if(!readable(object,object_size))continue;
        if(dead_offset&&readable((BYTE *)object+dead_offset,1)&&*((BYTE *)object+dead_offset)!=0)continue;
        BOOL known=FALSE;
        for(unsigned i=0;i<*count;i++)if(snapshot[i]==object){known=TRUE;break;}
        if(!known&&*count<RACE_SNAPSHOT_MAX){
            snapshot[*count]=object;
            vtable[*count]=*(void **)object;
            (*count)++;
        }
    }
    if(dead_offset){
        unsigned kept=0;
        for(unsigned i=0;i<*count;i++){
            void *object=snapshot[i];
            if(!readable((BYTE *)object+dead_offset,1)||*((BYTE *)object+dead_offset)!=0)continue;
            snapshot[kept]=object;
            vtable[kept]=vtable[i];
            kept++;
        }
        *count=kept;
    }
    for(unsigned i=0;i<*count;i++){
        void *object=snapshot[i];
        BOOL present=FALSE;
        for(void **item=begin;item<end;item++)if(*item==object){present=TRUE;break;}
        if(present)continue;
        if(!readable(object,object_size)||*(void **)object!=vtable[i])continue;
        if(end+1>cap){
            size_t used=(size_t)((BYTE *)end-(BYTE *)begin)/4;
            size_t next=used+16;
            BYTE *fresh=race_culling_operator_new?(BYTE *)race_culling_operator_new(next*4):NULL;
            if(!fresh)continue;
            memcpy(fresh,begin,used*4);
            ((void **)fresh)[used]=object;
            vector[0]=fresh;
            vector[1]=fresh+(used+1)*4;
            vector[2]=fresh+next*4;
            begin=(void **)fresh;
            end=begin+used+1;
            cap=begin+next;
            readded++;
            continue;
        }
        *end=object;
        end++;
        vector[1]=end;
        readded++;
    }
    return readded;
}

static void race_culling_props(BYTE *base){
    static BOOL announced;
    race_culling_prop_count=race_culling_prop_forced=0;
    if(!race_culling_visual){
        if(!race_culling_visual_announced){race_culling_visual_announced=TRUE;log_line("Culling prop visibility bridge unavailable; trigger culling fallback only.");}
        return;
    }
    if(!readable(base+0x7e3080,4))return;
    void *manager=*(void **)(base+0x7e3080);
    if(!readable(manager,484))return;
    if(manager!=race_prop_manager){
        race_prop_manager=manager;
        race_prop_snapshot_count=race_node_snapshot_count=0;
    }
    unsigned prop_readd=race_culling_readd((void **)((BYTE *)manager+448),572,race_prop_snapshot,race_prop_vtable,&race_prop_snapshot_count,493);
    prop_readd+=race_culling_readd((void **)((BYTE *)manager+460),572,race_prop_snapshot,race_prop_vtable,&race_prop_snapshot_count,493);
    unsigned node_readd=race_culling_readd((void **)((BYTE *)manager+472),52,race_node_snapshot,race_node_vtable,&race_node_snapshot_count,121);
    if(race_culling_trace&&(race_culling_frames%120)==0){
        void **pb=*(void ***)((BYTE *)manager+460),**pe=*(void ***)((BYTE *)manager+464);
        unsigned active=(pb&&pe&&pe>=pb)?(unsigned)(pe-pb):0;
        char line[220];snprintf(line,sizeof(line),"Culling props: active=%u snapshots=%u/%u readd=%u/%u.",
            active,race_prop_snapshot_count,race_node_snapshot_count,prop_readd,node_readd);log_line(line);
    }
    /* gaPropManager keeps its ticking props in the vector at +460. The +448
     * vector is only the add/dedupe list and is normally empty, which made the
     * old code report props=0 and skip the road-edge prop visuals. Grouped
     * destructibles (breakable walls) are gaPropNodes in the +472 vector; each
     * node owns DamagePiece pointers at +40 and each piece's visual is +16. */
    void **begin=*(void ***)((BYTE *)manager+460),**end=*(void ***)((BYTE *)manager+464);
    if(begin&&end&&end>=begin&&(BYTE *)end-(BYTE *)begin<=20000*4){
        race_culling_prop_count=(unsigned)(end-begin);
        if(!announced){
            announced=TRUE;
            char line[160];snprintf(line,sizeof(line),"Culling props: manager=%p active=%u.",manager,race_culling_prop_count);log_line(line);
        }
        for(void **item=begin;item<end;item++){
            void *prop=*item;
            if(!readable(prop,572))continue;
            /* forceBreak sets +493 once the prop is broken and hides its
             * visual. Keep forcing only intact props so a smashed wall clears. */
            if(*((BYTE *)prop+493)!=0)continue;
            void *visual=*(void **)((BYTE *)prop+288);
            if(!readable(visual,132))continue;
            /* Do not touch gaPropBase+496/+464. Those drive the prop's own
             * setVisible bookkeeping; clearing them made destructible props
             * (barricades/fences) stop drawing. Only push the visual itself. */
            *((DWORD *)((BYTE *)visual+128))=0x0FFF;
            *((BYTE *)visual+137)&=(BYTE)~1; /* disable view-frustum culling */
            *((BYTE *)visual+20)=1;
            race_culling_visual(visual,1);
            race_culling_prop_forced++;
        }
    }
    void **nbegin=*(void ***)((BYTE *)manager+472),**nend=*(void ***)((BYTE *)manager+476);
    if(!nbegin||!nend||nend<nbegin||(BYTE *)nend-(BYTE *)nbegin>20000*4)return;
    for(void **item=nbegin;item<nend;item++){
        BYTE *node=(BYTE *)*item;
        if(!readable(node,122))continue;
        if(*((BYTE *)node+121)==1)continue; /* damaged: let the broken wall clear */
        void **pbegin=*(void ***)(node+40),**pend=*(void ***)(node+44);
        if(!pbegin||!pend||pend<pbegin||(BYTE *)pend-(BYTE *)pbegin>2000*4)continue;
        for(void **piece_item=pbegin;piece_item<pend;piece_item++){
            BYTE *piece=(BYTE *)*piece_item;
            if(!readable(piece,132))continue;
            void *visual=*(void **)(piece+16);
            if(!readable(visual,132))continue;
            /* Leave the piece's own visible flag alone: a broken-off piece is
             * hidden by gaPropNodes and must stay hidden. Only widen its draw
             * mask and clear the frustum flag. */
            *((DWORD *)((BYTE *)visual+128))=0x0FFF;
            *((BYTE *)visual+137)&=(BYTE)~1;
            race_culling_prop_forced++;
        }
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
        *((BYTE *)visual+137)&=(BYTE)~1; /* disable view-frustum culling */
        race_culling_visual(visual,1);
        race_culling_ramp_forced++;
    }
}

/* Bound the retained visual blocks to the tier's behind + ahead window. The
 * original erase is suppressed by the retention patch, so trim here and hide
 * the pool visuals that leave the window before compacting the block vector. */
static float race_culling_player_distance(BYTE *base){
    if(!readable(base+0x7d5930,4))return 0.0f;
    void *players=*(void **)(base+0x7d5930);
    if(!readable(players,52))return 0.0f;
    void *list=*(void **)((BYTE *)players+48);
    if(!readable(list,4))return 0.0f;
    void *local=*(void **)list;
    if(!readable(local,8))return 0.0f;
    void *tracker=*(void **)((BYTE *)local+4);
    if(!readable(tracker,124))return 0.0f;
    return *((float *)((BYTE *)tracker+120));
}
static void race_culling_hide_block(BYTE *blocks,BYTE *block,void **visuals){
    int index=*((int *)(block+80));
    if(index<0||!visuals)return;
    void **slot=(void **)((BYTE *)visuals+index*4);
    if(!readable(slot,4))return;
    void *visual=*slot;
    if(!visual||!readable(visual,132))return;
    *((BYTE *)visual+20)=0;
    *((DWORD *)((BYTE *)visual+128))=0;
    if(race_culling_visual)race_culling_visual(visual,0);
    (void)blocks;
}
static void race_culling_trim_strip(BYTE *strip){
    if(race_photo_active)return; /* photo mode keeps every spawned block */
    if(race_window_behind<=0&&race_window_ahead<=0)return;
    BYTE *blocks=*(BYTE **)(strip+152),*blocks_end=*(BYTE **)(strip+156);
    if(!blocks||!blocks_end||blocks_end<blocks||((size_t)(blocks_end-blocks)%88))return;
    size_t count=(size_t)(blocks_end-blocks)/88;
    if(!count)return;
    /* Block +76 stores the strip's spawn cursor value, and tick() advances that
     * cursor to the car's node index as it spawns. Use the cursor itself as the
     * window centre; the car's world distance is a different coordinate space
     * and sliding it against block steps pushed the window to the list end. */
    int cursor=*((int *)(strip+56)),cursor_b=*((int *)(strip+60));
    if(cursor_b>0&&cursor_b<cursor)cursor=cursor_b;
    float player_step=cursor>0?(float)cursor:0.0f;
    size_t player_index=0;
    while(player_index<count&&*((float *)(blocks+player_index*88+76))<player_step)player_index++;
    size_t keep_start=player_index>(size_t)race_window_behind?player_index-(size_t)race_window_behind:0;
    size_t keep_end=player_index+(size_t)race_window_ahead;
    if(keep_end>count)keep_end=count;
    if(keep_start==0&&keep_end==count)return;
    void **visuals=*(void ***)(strip+164);
    for(size_t i=0;i<keep_start;i++)race_culling_hide_block(blocks,blocks+i*88,visuals);
    for(size_t i=keep_end;i<count;i++)race_culling_hide_block(blocks,blocks+i*88,visuals);
    size_t keep=keep_end-keep_start;
    memmove(blocks,blocks+keep_start*88,keep*88);
    *(BYTE **)(strip+156)=blocks+keep*88;
}

/* ---- Ahead scrolling signs via side type-6 strips -----------------------
 * The original type-6 strip owns exactly 8 base + 8 glow sign visuals and
 * tickScoreZone places them at the pace-marker time and three score-zone
 * steps behind it. Instead of converting +268/+300 to vectors (which would
 * also require destructor and EH changes), allocate independent type-6
 * strips, share the live strip's spline control blocks and tick each one at
 * a shifted pace time. Their own custom mesh and score-zone text are hidden;
 * only the sign visuals remain. */
static BOOL race_culling_player_state(BYTE *base,float *distance,float *progress,void **tracker_out){
    if(!readable(base+0x7d5930,4))return FALSE;
    void *players=*(void **)(base+0x7d5930);
    if(!readable(players,52))return FALSE;
    void *list=*(void **)((BYTE *)players+48);
    if(!readable(list,4))return FALSE;
    void *local=*(void **)list;
    if(!readable(local,8))return FALSE;
    void *tracker=*(void **)((BYTE *)local+4);
    if(!readable(tracker,124))return FALSE;
    float total=*((float *)((BYTE *)tracker+116)),position=*((float *)((BYTE *)tracker+120));
    if(distance)*distance=position;
    if(progress)*progress=total>0.01f?position/total:0.0f;
    if(tracker_out)*tracker_out=tracker;
    return TRUE;
}
static void *race_culling_camera_player(BYTE *base){
    if(!readable(base+0x7cb408,4))return NULL;
    void *manager=*(void **)(base+0x7cb408);
    if(!readable(manager,4))return NULL;
    void *set=((void **)manager)[0];
    if(!readable(set,8))return NULL;
    void *player=*(void **)((BYTE *)set+4);
    return readable(player,76)?player:NULL;
}
/* gaPaceMarker.get() is ((uint*)gaRaceManager)[1]; the race manager pointer
 * is the same global the pause check already reads. */
static void *race_culling_pace(BYTE *base){
    if(!readable(base+0x7d6668,4))return NULL;
    BYTE *race=*(BYTE **)(base+0x7d6668);
    if(!readable(race,8))return NULL;
    void *pace=*(void **)(race+4);
    return readable(pace,12)?pace:NULL;
}
static BOOL race_culling_trackgen(BYTE *base,float *song_len,float *track_len,float *zone){
    if(!readable(base+0x7dc750,4))return FALSE;
    BYTE *gen=*(BYTE **)(base+0x7dc750);
    if(!gen||!readable(gen,288))return FALSE;
    float song=*((float *)(gen+160)),length=*((float *)(gen+280)),step=*((float *)(gen+284));
    if(song_len)*song_len=song;
    if(track_len)*track_len=length;
    if(zone)*zone=step;
    return song>0.001f&&length>0.001f;
}
static BYTE *race_culling_sign_main(BYTE *base,void **manager_out){
    if(!readable(base+0x7dce7c,4))return NULL;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2728))return NULL;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return NULL;
    if(manager_out)*manager_out=manager;
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        if(*((int *)(strip+212))==6)return strip;
    }
    return NULL;
}
static void race_culling_sign_hide_extras(BYTE *strip){
    if(!readable(strip,424))return;
    BYTE *custom=*(BYTE **)(strip+256);
    if(custom&&readable(custom,132)){
        *((BYTE *)custom+20)=0;
        *((DWORD *)((BYTE *)custom+128))=0;
        if(race_culling_visual)race_culling_visual(custom,0);
    }
    for(int i=0;i<8;i++){
        void *two=*(void **)(strip+332+i*4);
        if(two&&readable(two,132)){
            /* elVisual2d shares elVisual's visible byte at +20. Do not touch
             * +128 here; that mask belongs to the 3D visual layout. */
            *((BYTE *)two+20)=0;
            if(race_culling_visual)race_culling_visual(two,0);
        }
    }
}
static void race_culling_sign_hide(BYTE *strip){
    race_culling_sign_hide_extras(strip);
    if(!readable(strip,424))return;
    for(int off=268;off<=300;off+=32){
        for(int i=0;i<8;i++){
            void *sign=*(void **)(strip+off+i*4);
            if(!sign||!readable(sign,132))continue;
            *((BYTE *)sign+20)=0;
            *((DWORD *)((BYTE *)sign+128))=0;
            if(race_culling_visual)race_culling_visual(sign,0);
        }
    }
}
/* tickScoreZone always writes the base sign transform but only toggles the
 * glow's visibility, so re-enable each placed base sign after the side tick.
 * A zero translation means the slot has never been placed (the ctor leaves
 * the matrix at identity); leave that one hidden. */
static void race_culling_sign_present(BYTE *strip){
    for(int i=0;i<8;i++){
        void *base=*(void **)(strip+268+i*4);
        if(base&&readable(base,132)){
            float *translation=(float *)((BYTE *)base+76);
            if(translation[0]!=0.0f||translation[1]!=0.0f||translation[2]!=0.0f){
                *((BYTE *)base+20)=1;
                if(race_culling_visual)race_culling_visual(base,1);
            }
            *((DWORD *)((BYTE *)base+128))=0x0FFF;
            *((BYTE *)base+137)&=(BYTE)~1;
        }
        void *glow=*(void **)(strip+300+i*4);
        if(glow&&readable(glow,132)){
            *((DWORD *)((BYTE *)glow+128))=0x0FFF;
            *((BYTE *)glow+137)&=(BYTE)~1;
        }
    }
}
static void race_culling_sign_link(BYTE *strip,BYTE *main){
    if(!readable(strip,424)||!readable(main,232))return;
    /* Reuse the live type-6 spline and score-zone state. The side strip is
     * never destroyed, so sharing these control blocks is safe. */
    memcpy(strip+64,main+64,72);
    memcpy(strip+136,main+136,12);
    memcpy(strip+216,main+216,16);
    /* Type 6 has no spawn case, so nothing is created by tick. Mark both
     * side counters as already complete to skip the empty spawn loop. */
    *((int *)(strip+56))=-1;
    *((int *)(strip+60))=-1;
}
static BYTE *race_culling_sign_create(BYTE *main){
    if(!race_culling_strip_ctor)return NULL;
    BYTE *strip=(BYTE *)VirtualAlloc(NULL,424,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!strip)return NULL;
    if(!race_culling_strip_ctor(strip,6)){
        VirtualFree(strip,0,MEM_RELEASE);
        return NULL;
    }
    race_culling_sign_link(strip,main);
    race_culling_sign_hide(strip);
    return strip;
}
static void race_culling_sign_tick(BYTE *base){
    (void)base;
    return; /* scrolling rails and score zones stay vanilla for now */
    if(race_retention_tier<=0||!race_culling_strip_tick||!race_culling_strip_ctor)return;
    void *manager=NULL;
    BYTE *main=race_culling_sign_main(base,&manager);
    if(!main||!manager){
        for(int i=0;i<race_sign_strip_count;i++)race_culling_sign_hide((BYTE *)race_sign_strips[i]);
        race_sign_manager=NULL;
        return;
    }
    /* The live type-6 strip owns the rails/bases around the car. Its own tick
     * only toggles glow visibility, and the base signs stay hidden once the
     * distance rule trips; re-present them like the side strips. */
    race_culling_sign_present(main);
    if(race_sign_manager!=manager){
        for(int i=0;i<race_sign_strip_count;i++)race_culling_sign_hide((BYTE *)race_sign_strips[i]);
        race_sign_manager=manager;
        for(int i=0;i<race_sign_strip_count;i++)race_culling_sign_link((BYTE *)race_sign_strips[i],main);
        race_sign_announced=FALSE;
    }
    float distance=0.0f,progress=0.0f,song_len=0.0f,track_len=0.0f,zone=6.0f;
    void *tracker=NULL;
    if(!race_culling_player_state(base,&distance,&progress,&tracker))return;
    if(!race_culling_trackgen(base,&song_len,&track_len,&zone))return;

    int want=1;
    {
        int ahead=race_window_ahead>0?race_window_ahead:64;
        want=(ahead+63)/64;
        if(want<1)want=1;
        if(want>4)want=4;
    }
    while(race_sign_strip_count<want){
        BYTE *strip=race_culling_sign_create(main);
        if(!strip)break;
        race_sign_strips[race_sign_strip_count++]=strip;
    }
    if(!race_sign_strip_count)return;

    /* Block distance lets the tier's ahead block window become world units.
     * The prewarm records the measured block total for this manager. */
    float block_distance=1.8f;
    if(race_track_block_total>0&&track_len>0.001f)block_distance=track_len/(float)race_track_block_total;
    if(block_distance<0.25f)block_distance=0.25f;
    int strips=race_sign_strip_count;
    if(strips>4)strips=4;
    /* One strip covers the tier-1 ahead window (64 blocks). Higher tiers add
     * more strips at the same sign density instead of stretching one strip. */
    float step_distance=64.0f*block_distance;
    if(step_distance<12.0f)step_distance=12.0f;
    float window=step_distance*(float)strips;
    float spacing=step_distance/6.0f;
    /* Camera-relative in photo mode: centre the ahead window on the camera's
     * projected along-track offset instead of pinning it to the car. */
    float offset=0.0f;
    if(race_photo_active){
        void *player=race_culling_camera_player(base);
        float position[3],forward[3];
        if(player&&race_camera_player(player,position,forward)){
            offset=(race_photo_position[0]-position[0])*forward[0]
                  +(race_photo_position[1]-position[1])*forward[1]
                  +(race_photo_position[2]-position[2])*forward[2];
        }
        offset-=window*0.5f;
    }
    void *node=race_culling_back_node?race_culling_back_node(tracker,NULL,NULL):NULL;
    if(!node)return;
    void *pace=race_culling_pace(base);
    float *pace_time=(pace&&readable(pace,12))?(float *)((BYTE *)pace+8):NULL;
    float *zone_slot=NULL;
    if(readable(base+0x7dc750,4)){
        BYTE *gen=*(BYTE **)(base+0x7dc750);
        if(gen&&readable(gen,288))zone_slot=(float *)(gen+284);
    }
    /* tickScoreZone reads the global score-zone step for both its hidden mesh
     * and the sign pairs. Temporarily widening it spreads four pairs across
     * the strip's slice of the ahead window; restoring it keeps the live
     * strip's own layout untouched. */
    float saved_zone=zone;
    for(int i=0;i<strips;i++){
        BYTE *strip=(BYTE *)race_sign_strips[i];
        BYTE *custom=*(BYTE **)(strip+256);
        if(!custom||!readable(custom,180)||!*(void **)(strip+268)||!readable(*(void **)(strip+268),132))continue;
        race_culling_sign_link(strip,main);
        float anchor=offset+(float)(i+1)*step_distance;
        float delta=pace_time?anchor*(song_len/track_len):0.0f;
        float saved_time=pace_time?*pace_time:0.0f;
        if(zone_slot)*zone_slot=spacing;
        if(pace_time)*pace_time=saved_time+delta;
        race_culling_strip_tick(strip,1,0,progress,node);
        if(pace_time)*pace_time=saved_time;
        if(zone_slot)*zone_slot=saved_zone;
        race_culling_sign_hide_extras(strip);
        race_culling_sign_present(strip);
    }
    if(!race_sign_announced){
        race_sign_announced=TRUE;
        char detail[240];
        snprintf(detail,sizeof(detail),"Sign side strips: tier=%d count=%d window=%.1f step=%.1f spacing=%.1f zone=%.1f block=%.2f photo_offset=%.1f.",
            race_retention_tier,strips,window,step_distance,spacing,saved_zone,block_distance,offset);
        log_line(detail);
    }
    if(race_culling_trace&&(race_culling_frames%120)==0){
        void *visual=*(void **)((BYTE *)race_sign_strips[0]+268);
        float *matrix=(visual&&readable(visual,132))?(float *)((BYTE *)visual+76):NULL;
        int live_placed=0,live_visible=0;
        if(readable(main,424)){
            for(int i=0;i<8;i++){
                void *base=*(void **)(main+268+i*4);
                if(!base||!readable(base,132))continue;
                float *translation=(float *)((BYTE *)base+76);
                if(translation[0]!=0.0f||translation[1]!=0.0f||translation[2]!=0.0f)live_placed++;
                if(*((BYTE *)base+20)==1)live_visible++;
            }
        }
        char trace[340];
        snprintf(trace,sizeof(trace),"Sign trace: strips=%d window=%.1f step=%.1f spacing=%.1f distance=%.1f pace=%.2f photo=%d live=%d/%d sign=%.1f,%.1f,%.1f.",
            strips,window,step_distance,spacing,distance,pace_time?*pace_time:-1.0f,race_photo_active?1:0,
            live_visible,live_placed,
            matrix?matrix[0]:0.0f,matrix?matrix[1]:0.0f,matrix?matrix[2]:0.0f);
        log_line(trace);
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
    race_building_strips=race_building_blocks=race_building_visible=0;
    race_culling_block_min=race_culling_block_max=0;
    memset(seen,0,sizeof(seen));
    if(!race_culling_visual||!readable(base+0x7dce7c,4))return;
    void *manager=*(void **)(base+0x7dce7c);
    if(!readable(manager,2732))return;
    void **begin=*(void ***)((BYTE *)manager+2720),**end=*(void ***)((BYTE *)manager+2724);
    if(!begin||!end||end<begin||(size_t)((BYTE *)end-(BYTE *)begin)>256*sizeof(void *))return;
    race_culling_strip_count=(unsigned)(end-begin);
    race_player_distance=race_culling_player_distance(base);
    /* Photo mode can move the camera far from the car while the race position
     * stays frozen. Advance the prop strips to the camera's along-track
     * position so scenery (road-edge rails, turbines, blocks) spawns where the
     * camera is looking instead of only near the car. Incremental ticks keep
     * model resources loading naturally, unlike a synthetic full-track sweep. */
    float photo_progress=-1.0f;
    void *photo_node=NULL;
    if(race_photo_active&&race_culling_strip_tick&&race_culling_back_node){
        float distance=0.0f,progress=0.0f,song_len=0.0f,track_len=0.0f,zone=6.0f;
        void *tracker=NULL;
        if(race_culling_player_state(base,&distance,&progress,&tracker)
            &&race_culling_trackgen(base,&song_len,&track_len,&zone)&&track_len>1.0f){
            void *player=race_culling_camera_player(base);
            float position[3],forward[3];
            if(player&&race_camera_player(player,position,forward)){
                float offset=(race_photo_position[0]-position[0])*forward[0]
                            +(race_photo_position[1]-position[1])*forward[1]
                            +(race_photo_position[2]-position[2])*forward[2];
                photo_progress=progress+offset/track_len;
                if(photo_progress<0.0f)photo_progress=0.0f;
                if(photo_progress>1.0f)photo_progress=1.0f;
                photo_node=race_culling_back_node(tracker,NULL,NULL);
                if(!photo_node)photo_progress=-1.0f;
            }
        }
    }
    for(void **item=begin;item<end;item++){
        BYTE *strip=(BYTE *)*item;
        if(!readable(strip,424))continue;
        int type=*((int *)(strip+212));
        if(type==13){
            /* Census the building strip before deciding whether to retain it,
             * so the trace can compare vanilla and retained draws. Each block
             * maps to one visual, so visible blocks are visible draw calls. */
            race_building_strips++;
            BYTE *bb=*(BYTE **)(strip+152),*be=*(BYTE **)(strip+156);
            void **vis=*(void ***)(strip+164);
            if(bb&&be&&be>bb&&(size_t)(be-bb)<=10000*88&&((size_t)(be-bb)%88)==0
                &&vis&&readable(vis,4)&&readable(bb,(size_t)(be-bb))){
                for(BYTE *block=bb;block<be;block+=88){
                    int index=*((int *)(block+80));
                    if(index<0||index>=10000)continue;
                    race_building_blocks++;
                    void *visual=*((void **)((BYTE *)vis+index*4));
                    if(visual&&readable(visual,132)&&*((BYTE *)visual+20)==1)race_building_visible++;
                }
            }
        }
        /* Leave destructible special blocks (type 10) vanilla so a wall that
         * has been hit retires instead of being forced back on screen. Distant
         * buildings (type 13) are also vanilla by default: the base game hides
         * a building two units behind the car, and forcing the retained window
         * would keep every building under and behind the track on the draw
         * list. [graphics] retain_buildings keeps the old behaviour for A/B. */
        if(type==10||(type==13&&!race_retain_buildings))continue;
        if(photo_progress>=0.0f&&photo_node&&type!=0&&type!=6
            &&race_culling_strip_tick)race_culling_strip_tick(strip,1,0,photo_progress,photo_node);
        race_culling_trim_strip(strip);
        /* The custom road/wall/zone mesh is one elVisualCustom at strip+256.
         * When the road is split into chunks the original type-0 mesh is
         * hidden; everything else is forced visible and its fade window is
         * widened for the backed-out camera. */
        if(type!=0&&type!=6)race_culling_force_custom(strip,1);
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
            char detail[200];snprintf(detail,sizeof(detail),"Culling strip type=%d pool=%u mesh_v=%u mesh_t=%u mesh_visible=%u cursor=%d.",*((int *)(strip+212)),pool,mesh_v,mesh_t,mesh_visible,*((int *)(strip+56)));log_line(detail);
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
                *((BYTE *)visual+137)&=(BYTE)~1;
                continue;
            }
            *((DWORD *)((BYTE *)visual+128))=0x0FFF;
            *((BYTE *)visual+137)&=(BYTE)~1;
            *((BYTE *)visual+20)=1;
            race_culling_visual(visual,1);
            race_culling_strip_forced++;
        }
        if(race_culling_trace&&(race_culling_frames%120)==0){
            void **pool_begin=*(void ***)(strip+164),**pool_end=*(void ***)(strip+168);
            unsigned pool=pool_begin&&pool_end&&pool_end>=pool_begin?(unsigned)(pool_end-pool_begin):0;
            int cursor=*((int *)(strip+56)),cursor_b=*((int *)(strip+60));
            if(cursor_b>0&&cursor_b<cursor)cursor=cursor_b;
            float player_step=cursor>0?(float)cursor:0.0f;
            float first=blocks_end>blocks?*((float *)(blocks+76)):0.0f;
            float last=blocks_end>blocks?*((float *)(blocks_end-88+76)):0.0f;
            char detail[220];snprintf(detail,sizeof(detail),"Culling strip type=%d blocks=%u unique=%u current=%d pool=%u step=%.1f span=%.1f..%.1f.",
                *((int *)(strip+212)),strip_blocks,strip_unique,*((int *)(strip+188)),pool,player_step,first,last);log_line(detail);
        }
    }
    if(race_culling_trace&&(race_culling_frames%120)==0&&race_building_strips){
        char line[180];snprintf(line,sizeof(line),"Buildings: mode=%s strips=%u blocks=%u visible=%u.",
            race_retain_buildings?"retained":"vanilla",race_building_strips,race_building_blocks,race_building_visible);
        log_line(line);
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
    race_culling_frames++;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    race_culling_prewarm(base);
    void *manager=readable(base+0x7df324,4)?*(void **)(base+0x7df324):NULL;
    BOOL paused=race_culling_paused(base);
    race_culling_apply(manager,paused);
    race_culling_strips(base);
    race_culling_sign_tick(base);
    race_culling_road_chunks_apply();
    race_culling_props(base);race_culling_ramps(base);
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
