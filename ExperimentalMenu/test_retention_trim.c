/* Verifies the retention window uses the strip's own block-step coordinate.
 *
 * tick() converts the car's track distance with (strip+216 * distance) /
 * strip+228, and block+76 stores that step. Comparing distance directly made
 * the window jump to the end of the block list and hide the scenery around
 * the car. */
#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE *alloc(size_t bytes){return (BYTE *)VirtualAlloc(NULL,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);}
static void set_block(BYTE *blocks,int i,float step,int visual){
    ZeroMemory(blocks+i*88,88);
    *((float *)(blocks+i*88+76))=step;
    *((int *)(blocks+i*88+80))=visual;
}
int main(void){
    enum{BLOCKS=12,VISUALS=16};
    BYTE *strip=alloc(424);
    CHECK(strip);
    BYTE *blocks=alloc(BLOCKS*88);
    void **visuals=(void **)alloc(VISUALS*sizeof(void *));
    CHECK(blocks&&visuals);
    for(int i=0;i<VISUALS;i++){
        visuals[i]=alloc(132);
        CHECK(visuals[i]);
        *((BYTE *)visuals[i]+20)=1;
    }
    for(int i=0;i<BLOCKS;i++)set_block(blocks,i,(float)(i+1)*10.0f,i);
    *(BYTE **)(strip+152)=blocks;
    *(BYTE **)(strip+156)=blocks+BLOCKS*88;
    *(void ***)(strip+164)=visuals;
    *(void ***)(strip+168)=visuals+VISUALS;
    *((int *)(strip+216))=1;
    *((int *)(strip+228))=1;

    /* Distance 50 is block step 50: keep 2 behind and 3 ahead (steps 30..70). */
    race_window_behind=2;race_window_ahead=3;race_player_distance=50.0f;
    race_culling_trim_strip(strip);
    CHECK(*(BYTE **)(strip+156)-blocks==5*88);
    CHECK(*((float *)(blocks+76))==30.0f);
    CHECK(*((float *)(blocks+4*88+76))==70.0f);
    CHECK(*((BYTE *)visuals[0]+20)==0);
    CHECK(*((BYTE *)visuals[1]+20)==0);
    CHECK(*((BYTE *)visuals[2]+20)==1);
    CHECK(*((BYTE *)visuals[6]+20)==1);
    CHECK(*((BYTE *)visuals[7]+20)==0);

    /* A non-unit step scale maps the same distance to the strip's steps:
     * 100 * (2/4) = step 50. */
    for(int i=0;i<BLOCKS;i++)set_block(blocks,i,(float)(i+1)*10.0f,i);
    for(int i=0;i<VISUALS;i++)*((BYTE *)visuals[i]+20)=1;
    *(BYTE **)(strip+156)=blocks+BLOCKS*88;
    *((int *)(strip+216))=2;
    *((int *)(strip+228))=4;
    race_player_distance=100.0f;
    race_culling_trim_strip(strip);
    CHECK(*(BYTE **)(strip+156)-blocks==5*88);
    CHECK(*((float *)(blocks+76))==30.0f);

    /* The whole list stays when it already fits the window. */
    for(int i=0;i<BLOCKS;i++)set_block(blocks,i,(float)(i+1)*10.0f,i);
    *(BYTE **)(strip+156)=blocks+BLOCKS*88;
    race_player_distance=10.0f;race_window_behind=32;race_window_ahead=32;
    race_culling_trim_strip(strip);
    CHECK(*(BYTE **)(strip+156)-blocks==BLOCKS*88);
    puts("PASS: retention trims around the car in the strip step coordinate");return 0;
}
