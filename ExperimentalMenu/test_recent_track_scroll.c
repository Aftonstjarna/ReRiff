#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE menu[1284],screen[64],rows_storage[3][404];static void *rows[3],*table[29];
static BYTE reel_storage[532];static int follows,last_follow;
static void __cdecl follow_mock(void *reel,int target,float *duration){if(reel!=reel_storage||!duration||*duration!=0)abort();follows++;last_follow=target;*(int *)(reel_storage+436)=0;}
static DWORD held,now;static BOOL foreground=TRUE;static int moves;
static BOOL active_mock(void){return foreground;}
static DWORD input_mock(void){return held;}
static DWORD WINAPI clock_mock(void){return now;}
static void __thiscall direction_mock(void *row,DWORD key,unsigned device){
    if(device)abort();int index=row==rows[0]?0:row==rows[1]?1:2;int next=index+(key==64?-1:1);
    if(next<0||next>2||!(rows_storage[next][372]&4))abort();
    *(void **)(screen+20)=rows[next];moves++;
}
int main(void){
    CHECK(recent_repeat_interval(0)==250&&recent_repeat_interval(1000)==180&&recent_repeat_interval(2000)==110);
    CHECK(recent_repeat_interval(4000)==20&&recent_repeat_interval(100000)==20);
    CHECK(!recent_repeat_due(menu,rows,0,1,0,FALSE));
    CHECK(!recent_repeat_due(menu,rows,0,1,200,FALSE));CHECK(recent_repeat_due(menu,rows,0,1,250,FALSE));
    CHECK(!recent_repeat_due(menu,rows,0,1,300,TRUE));CHECK(!recent_repeat_due(menu,rows,0,1,400,FALSE));
    CHECK(!recent_repeat_due(menu,rows,0,-1,450,FALSE));
    CHECK(!recent_repeat_due(menu,rows,1,-1,500,FALSE));
    CHECK(!recent_repeat_due(menu,rows,1,0,501,FALSE));CHECK(!recent_repeat.owner);
    recent_repeat_reset();CHECK(!recent_repeat_due(menu,rows,0,1,0xffffff80u,FALSE));
    CHECK(recent_repeat_due(menu,rows,0,1,0x80u,FALSE)); /* tick counter rollover */
    CHECK(!recent_repeat_due(menu,rows,0,1,3000,FALSE)); /* pause/re-entry */
    recent_scroll_active=active_mock;recent_scroll_clock=clock_mock;direction_input=input_mock;
    ((int *)menu)[7]=1;((int *)menu)[102]=12345; /* embedded tab data is not a lifecycle flag */
    ((void **)menu)[46]=screen;*(DWORD *)(screen+40)=1;
    *(void ***)(menu+1248)=rows;*(void ***)(menu+1252)=rows+3;*(void ***)(menu+1256)=rows+3;
    table[28]=(void *)direction_mock;
    for(int i=0;i<3;i++){rows[i]=rows_storage[i];*(void ***)rows[i]=table;*(DWORD *)(rows_storage[i]+76)=1;}
    *(void **)(screen+20)=rows[0];recent_repeat_reset();held=128;now=0;
    recent_scroll_after(menu,rows[0]);CHECK(moves==0);
    now=250;recent_scroll_after(menu,rows[0]);CHECK(moves==1&&*(void **)(screen+20)==rows[1]);
    now=500;recent_scroll_after(menu,rows[1]);CHECK(moves==2&&*(void **)(screen+20)==rows[2]);
    now=750;recent_scroll_after(menu,rows[2]);CHECK(moves==2); /* bottom clamp */
    held=192;recent_scroll_after(menu,rows[2]);CHECK(!recent_repeat.owner&&moves==2);
    held=64;now=800;recent_scroll_after(menu,rows[2]);now=1050;recent_scroll_after(menu,rows[2]);CHECK(moves==3);
    *(void **)(screen+20)=rows[0];now=1100;recent_scroll_after(menu,rows[1]);CHECK(moves==3); /* original already moved */
    now=1350;recent_scroll_after(menu,rows[0]);CHECK(moves==3); /* top clamp */
    foreground=FALSE;recent_scroll_after(menu,rows[0]);CHECK(!recent_repeat.owner);
    foreground=TRUE;((int *)menu)[7]=0;recent_scroll_after(menu,rows[0]);CHECK(!recent_repeat.owner);
    ((int *)menu)[7]=1;*(void **)(screen+20)=NULL;recent_scroll_after(menu,NULL);CHECK(moves==3);
    BYTE overlays[48]={0};void *manager=overlays;overlay_manager=&manager;
    *(void **)(screen+20)=rows[0];held=128;now=1500;((void **)overlays)[11]=menu;
    recent_scroll_after(menu,rows[0]);CHECK(recent_repeat.owner==menu); /* the menu may itself be an overlay */
    ((void **)overlays)[11]=screen;recent_scroll_after(menu,rows[0]);CHECK(!recent_repeat.owner);
    ((void **)overlays)[11]=NULL;
    *(void **)(menu+1244)=reel_storage;*(void ***)reel_storage=table;reel_vtable=table;reel_move=follow_mock;
    reel_storage[452]=1;*(int *)(reel_storage+436)=4;*(int *)(reel_storage+456)=2;
    held=0;recent_scroll_after(menu,rows[0]);CHECK(follows==1&&last_follow==2);
    recent_scroll_after(menu,rows[0]);CHECK(follows==1); /* settled: no repeated callbacks */
    *(int *)(reel_storage+436)=3;recent_scroll_after(menu,rows[0]);CHECK(follows==1); /* preserve drag/other states */
    *(int *)(reel_storage+436)=4;*(int *)(reel_storage+456)=3;recent_scroll_after(menu,rows[0]);CHECK(follows==1); /* reject bad target */
    *(int *)(reel_storage+456)=0;foreground=FALSE;recent_scroll_after(menu,rows[0]);CHECK(follows==1);
    foreground=TRUE;recent_scroll_after(menu,rows[0]);CHECK(follows==2&&last_follow==0);
    puts("PASS: picker acceleration curve, native-step deduplication, direction/sort/pause resets, rollover, native direction dispatch, boundaries, inactive guards and native immediate viewport follow");return 0;
}
