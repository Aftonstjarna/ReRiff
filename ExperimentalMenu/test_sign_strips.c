/* Exercises the ahead-sign side-strip bookkeeping without a live race.
 * The type-6 ctor and tick are stubbed; every other input (manager, player
 * tracker, gaTrackGen, pace marker) is a fake readable allocation placed at
 * the same RVAs the real client uses. */
#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE *g_base;
static int ctor_calls,ctor_type,tick_calls;
static float tick_pace,tick_zone;
static int tick_count_at_call;
static void *__cdecl fake_ctor(void *storage,int type){
    ctor_calls++;ctor_type=type;
    BYTE *strip=(BYTE *)storage;
    *(int *)(strip+212)=type;
    BYTE *custom=VirtualAlloc(NULL,188,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    *(void **)(strip+256)=custom;
    for(int i=0;i<8;i++){
        BYTE *base=VirtualAlloc(NULL,148,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        BYTE *glow=VirtualAlloc(NULL,148,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        *(void **)(strip+268+i*4)=base;
        *(void **)(strip+300+i*4)=glow;
    }
    return storage;
}
static void __cdecl fake_tick(void *strip,BYTE update_visual,BYTE update_collision,float progress,void *node){
    (void)strip;(void)update_visual;(void)update_collision;(void)progress;(void)node;
    tick_calls++;tick_count_at_call=race_sign_strip_count;
    BYTE *race=*(BYTE **)(g_base+0x7d6668);
    BYTE *pace=*(BYTE **)(race+4);
    tick_pace=*((float *)(pace+8));
    BYTE *gen=*(BYTE **)(g_base+0x7dc750);
    tick_zone=*((float *)(gen+284));
}
static void *__cdecl fake_node(void *tracker,void **forward,float *node_t){
    (void)tracker;(void)forward;(void)node_t;return (void *)1;
}
static BYTE *alloc(size_t bytes){return (BYTE *)VirtualAlloc(NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);}
int main(void){
    BYTE *base=alloc(0x900000);CHECK(base);g_base=base;
    BYTE *manager=alloc(2732),*strip=alloc(424),*custom=alloc(188);
    BYTE *items=alloc(sizeof(void *));
    void **slot=(void **)items;slot[0]=strip;
    *(void ***)(manager+2720)=(void **)items;*(void ***)(manager+2724)=(void **)items+1;
    *(void **)(base+0x7dce7c)=manager;
    *(int *)(strip+212)=6;*(void **)(strip+256)=custom;
    for(int i=0;i<8;i++){
        *(void **)(strip+268+i*4)=alloc(148);
        *(void **)(strip+300+i*4)=alloc(148);
    }
    BYTE *players=alloc(52),*list=alloc(4),*local=alloc(8),*tracker=alloc(124);
    *(void **)(base+0x7d5930)=players;*(void **)(players+48)=list;
    *(void **)list=local;*(void **)(local+4)=tracker;
    *((float *)(tracker+116))=1000.0f;*((float *)(tracker+120))=400.0f;
    BYTE *gen=alloc(288);*(void **)(base+0x7dc750)=gen;
    *((float *)(gen+160))=120.0f;*((float *)(gen+280))=1000.0f;*((float *)(gen+284))=6.0f;
    BYTE *race=alloc(16),*pace=alloc(16);
    *(void **)(base+0x7d6668)=race;*(void **)(race+4)=pace;*((float *)(pace+8))=10.0f;

    race_culling_strip_ctor=fake_ctor;
    race_culling_strip_tick=fake_tick;
    race_culling_back_node=fake_node;
    race_culling_visual=NULL;
    race_retention_pool=512;race_window_ahead=256;
    race_retention_tier=10;race_photo_active=FALSE;
    race_track_block_total=1000;
    *(void **)(strip+64)=(void *)0x12345678;

    race_culling_sign_tick(base);
    CHECK(ctor_calls==4);CHECK(ctor_type==6);
    CHECK(tick_calls==4);CHECK(tick_count_at_call==4);
    /* block_distance=1, step=64, spacing=64/6. Last strip anchor=4*64 and
     * delta=anchor*(120/1000). */
    CHECK(tick_pace>40.0f&&tick_pace<41.0f);
    CHECK(tick_zone>10.6f&&tick_zone<10.7f);
    CHECK(*((float *)(pace+8))==10.0f);
    CHECK(*((float *)(gen+284))==6.0f);
    /* The side strips point at the live spline control blocks. */
    CHECK(*(BYTE **)(race_sign_strips[0]+64)==*(BYTE **)(strip+64));

    int created=ctor_calls;
    race_culling_sign_tick(base);
    CHECK(ctor_calls==created);
    CHECK(tick_calls==8);
    CHECK(*((float *)(pace+8))==10.0f);
    CHECK(*((float *)(gen+284))==6.0f);

    /* Tier 1 uses one strip and a 64-block slice. */
    race_sign_strip_count=0;race_sign_manager=NULL;
    race_retention_tier=1;race_window_ahead=64;
    race_culling_sign_tick(base);
    CHECK(ctor_calls==created+1);
    CHECK(tick_calls==9);
    CHECK(tick_count_at_call==1);
    CHECK(tick_pace>17.0f&&tick_pace<18.0f);
    CHECK(*((float *)(pace+8))==10.0f);
    CHECK(*((float *)(gen+284))==6.0f);

    /* No type-6 strip means no new scene content and no tick. */
    *(int *)(strip+212)=8;
    int before=tick_calls;
    race_culling_sign_tick(base);
    CHECK(tick_calls==before);
    puts("PASS: sign side strips, tier count, forward anchors, pace/zone restore");return 0;
}
