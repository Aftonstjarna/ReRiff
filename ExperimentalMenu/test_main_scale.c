#include "engine_menu.c"
#include <math.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE menu[512],screen[512],items[11][512],spacers[11][512];
static void *methods[49],*menu_methods[27],*reel_methods[2];static int made,native_calls,tick_calls;static float height=1080;
struct Reel{void **table;int references;int first,maximum;};
static float *__thiscall resolution(void *g,float *s,int which){(void)g;(void)which;s[0]=height*16/9;s[1]=height;return s;}
static void __thiscall scale(void *i,const float *s){((float *)i)[43]=s[0];((float *)i)[44]=s[1];}
static void __thiscall position(void *i,const float *s){((float *)i)[8]=s[0];((float *)i)[9]=s[1];}
static void __thiscall visible(void *i,BYTE v){((BYTE *)i)[372]=(((BYTE *)i)[372]&~1)|!!v;}
static void __thiscall enabled(void *i,BYTE v){*(DWORD *)((BYTE *)i+76)=v?1:0;}
static void __thiscall inert(void *i,BYTE v){(void)i;(void)v;}
static void __thiscall direction(void *i,DWORD key,unsigned d){(void)d;int id=((int *)i)[key==64?39:40];for(int j=0;j<11;j++)if(((int *)items[j])[2]==id){if(!(items[j][372]&1)||!*(DWORD *)(items[j]+76))abort();}((int *)menu)[11]=id;native_calls++;}
static int __thiscall hit(void *row,float *point){(void)point;return ((int *)row)[2];}
static void __thiscall focus(void *m,int id){(void)m;((int *)menu)[11]=id;}
static void __thiscall selected(void *m,int id){((int *)m)[11]=id;}
static void *__cdecl allocate(size_t n){if(n!=532)abort();return calloc(1,sizeof(struct Reel));}
static void *__cdecl construct(void *r,void *m,float gap,int dir){(void)m;if(gap<=0||dir!=1)abort();*(void ***)r=reel_methods;return r;}
static void *__thiscall destroy(void *r,unsigned flag){if(flag!=1||((struct Reel *)r)->references)abort();free(r);return NULL;}
static void *__cdecl text(const WCHAR *s,float x,float y,unsigned f,int h,int v,float w,BOOL a,float b,BOOL c,BOOL d){(void)s;(void)x;(void)y;(void)f;(void)h;(void)v;(void)w;(void)a;(void)b;(void)c;(void)d;if(made>=11)abort();BYTE *p=spacers[made++];memset(p,0,512);*(void ***)p=methods;return p;}
static void __cdecl add(void *r,void *i,void *s){(void)r;(void)i;(void)s;}
static void __cdecl limit(void *r,int n){((struct Reel *)r)->maximum=11-n;}
static void __cdecl move(void *r,int n,float *t){if(*t)abort();((struct Reel *)r)->first=n;}
static int __cdecl selection(void *r){return ((struct Reel *)r)->first;}
static void __cdecl tick(void *r){(void)r;tick_calls++;}
static void __cdecl bar(void *r,float x,float t,float b,int pc,float *w){(void)r;(void)x;(void)t;(void)b;(void)pc;(void)w;}
int main(void){
    void *graphics=screen;ui_graphics_instance=&graphics;ui_resolution_original=resolution;bridge_ready=TRUE;
    methods[3]=visible;methods[9]=scale;methods[28]=direction;methods[25]=hit;menu_methods[25]=selected;reel_methods[0]=destroy;
    sprite_position=position;enable_input=enabled;menu_item=inert;set_focus=focus;
    game_allocate=allocate;reel_ctor=construct;reel_add=add;reel_limit=limit;reel_move=move;reel_selection=selection;reel_tick=tick;reel_bar=bar;create_text=text;
    const int sizes[]={25,50,55,75,100};const int heights[]={720,1080,1440,2160};
    for(unsigned h=0;h<4;h++)for(unsigned v=0;v<5;v++){
        height=heights[h];ui_scale_value=sizes[v];memset(menu,0,512);memset(screen,0,512);made=0;
        *(void ***)menu=menu_methods;((void **)menu)[46]=screen;*(DWORD *)(screen+40)=1;((int *)menu)[11]=701;
        ((float *)menu)[14]=((float *)menu)[15]=(1080.f/1536.f)*ui_design_factor(ui_scale_value);
        float gap=60*(height-220*((float *)menu)[15])/(1080-220*(1080.f/1536.f));
        for(int i=0;i<11;i++){memset(items[i],0,512);*(void ***)items[i]=methods;((void **)menu)[62+i]=items[i];((int *)items[i])[2]=701+i;((int *)items[i])[39]=701+(i+10)%11;((int *)items[i])[40]=701+(i+1)%11;((float *)items[i])[9]=height*0.3f+i*gap;((float *)items[i])[43]=((float *)items[i])[44]=1;}
        main_scale_size(menu);CHECK(main_scale_count==11);CHECK(fabsf(main_scale_gap-60*ui_design_factor(ui_scale_value))<0.01f);
        CHECK(main_scale_top>=170*((float *)menu)[15]);CHECK(main_scale_top+(main_scale_visible-1)*main_scale_gap<=height-50*((float *)menu)[15]+0.01f);
        attach_main_hits(menu,30);CHECK(main_hit_original_table==methods);main_scale_scroll_init(menu);
        for(int n=0;n<22;n++){int i=((int *)menu)[11]-701;((DirectionFn)(*(void ***)items[i])[28])(items[i],128,0);main_scale_reveal(menu);int next=((int *)menu)[11]-701;CHECK(next>=main_scale_first&&next<main_scale_first+main_scale_visible);}
        if(main_scale_reel){main_scale_tick(menu);CHECK(main_scroll_wheel(1));main_scale_tick(menu);main_scale_reveal(menu);CHECK(((int *)menu)[11]-701>=main_scale_first);}
        main_scale_cleanup();for(int i=0;i<11;i++)CHECK(*(void ***)items[i]==main_hit_table+1);detach_main_hits();for(int i=0;i<11;i++)CHECK(*(void ***)items[i]==methods);
    }
    ui_scale_value=0;
    for(unsigned h=0;h<4;h++){
        height=heights[h];float saved_y=((float *)items[0])[9],saved_sx=((float *)items[0])[43];
        main_scale_size(menu);CHECK(!main_scale_owner&&!main_scale_count&&!main_scale_reel);
        CHECK(((float *)items[0])[9]==saved_y&&((float *)items[0])[43]==saved_sx);
    }
    /* Native Apply stays outside the reel, including its bottom cap, at every
     * tested resolution and manual size. It must reserve list space too. */
    ((void **)menu)[66]=items[0];
    for(unsigned h=0;h<4;h++)for(unsigned v=0;v<5;v++){
        float sy=(1080.f/1536.f)*ui_design_factor(sizes[v]);
        ((float *)menu)[15]=sy;((float *)items[0])[9]=heights[h]-24*sy;
        ((float *)items[0])[13]=240;((float *)items[0])[44]=sy;((int *)items[0])[14]=8;
        float top=((float *)items[0])[9]-240*sy;
        CHECK(graphics_scroll_bottom(menu,heights[h]-100*sy)<=top-24*sy+0.01f);
    }
    puts("PASS: manual main-menu sizing at four resolutions/five scales, native overflow reel, reveal-before-direction, wheel focus, cleanup and Auto.");return 0;
}
