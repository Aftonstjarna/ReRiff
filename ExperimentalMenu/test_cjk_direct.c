#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
/* Exercise extension ownership independently of the renderer. Real native
   instruction guards are separately checked by test_native_archive.c. */
static BYTE visuals[64][0x180],view[0x1d4],game[72],font[0x70],fallback[0x70];
static BYTE *contents[130];static unsigned allocated,destroyed,dirty,cleared,invalid;
static void __thiscall add(void *v,void *a){
    if(v!=view){invalid++;return;}if(cjk_view_contains(v,a))return;
    unsigned n=(unsigned)((*(uintptr_t *)(view+0x1b4)-(uintptr_t)contents)/4);
    contents[n]=a;*(BYTE ***)(view+0x1b4)=contents+n+1;++*(DWORD *)((BYTE *)a+0x178);
}
static void __thiscall remove_visual(void *v,void *a){
    if(v!=view){invalid++;return;}
    unsigned n=(unsigned)((*(uintptr_t *)(view+0x1b4)-(uintptr_t)contents)/4);
    for(unsigned i=0;i<n;i++)if(contents[i]==a){
        --*(DWORD *)((BYTE *)a+0x178);memmove(contents+i,contents+i+1,(n-i-1)*4);
        *(BYTE ***)(view+0x1b4)=contents+n-1;break;
    }
}
static void * __thiscall allocate(void *pool,unsigned size,unsigned stride){
    if(pool!=cjk_direct_base+0x7bcf74||size!=380||stride!=380||allocated>=64){invalid++;return NULL;}
    return visuals[allocated++];
}
static void * __thiscall construct(void *a,int virtual_base){
    if(virtual_base!=1)invalid++;
    memset(a,0,0x17c);*(BYTE **)a=cjk_direct_base+0x599efc;
    *(BYTE **)((BYTE *)a+0x174)=cjk_direct_base+0x599ef8;
    *(DWORD *)((BYTE *)a+0x18)=123;*(DWORD *)((BYTE *)a+0x1c)=456;
    *(float *)((BYTE *)a+0x78)=77;*(float *)((BYTE *)a+0x80)=.25f;
    return a;
}
static void * __thiscall destroy(void *ref,unsigned release){
    if(release!=1||*(DWORD *)((BYTE *)ref+4))invalid++;destroyed++;return (BYTE *)ref-0x174;
}
static void __cdecl mark_dirty(void *a){(void)a;dirty++;}
static void __cdecl visible(void *a,BOOL show){if(((BYTE *)a)[0x14]!=(show!=0))invalid++;}
static void __thiscall colour(void *a,const float *rgba){memcpy((BYTE *)a+0x90,rgba,16);}
static void __thiscall child(void *a,void *node){if(node)invalid++;*(void **)((BYTE *)a+0xbc)=NULL;cleared++;}
int main(void){
    cjk_direct_base=VirtualAlloc(NULL,0x810000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(cjk_direct_base);
    *(BYTE **)(cjk_direct_base+0x7e9d24)=game;*(BYTE **)(game+20)=view;
    *(BYTE ***)(view+0x1b0)=contents;*(BYTE ***)(view+0x1b4)=contents;
    cjk_visual_allocate=allocate;cjk_visual_construct=construct;cjk_visual_child=child;
    cjk_add_original=add;cjk_remove_original=remove_visual;cjk_destroy_original=destroy;
    cjk_dirty_original=mark_dirty;cjk_visible_original=visible;cjk_direct_colour_original=colour;
    cjk_fonts[0].font=fallback;cjk_font_count=1;*(BYTE **)(font+0x68)=fallback;
    BYTE primary[0x180]={0},other[0x180]={0};construct(primary,1);construct(other,1);
    float cyan[4]={0,.75f,.8f,1};memcpy(primary+0x90,cyan,16);primary[0x14]=1;
    for(unsigned i=0x28;i<0x78;i++)primary[i]=(BYTE)i;
    void *frame[]={NULL,NULL,L"A\x3042.mp3",primary,NULL};
    cjk_direct_layout(font,frame);CHECK(!allocated&&!frame[4]); /* default disabled */
    cjk_direct_enabled=TRUE;
    frame[2]=L"Latin.mp3";cjk_direct_layout(font,frame);CHECK(!allocated);
    frame[2]=L"A\x3042.mp3";frame[4]=other;cjk_direct_layout(font,frame);CHECK(!allocated&&frame[4]==other);
    frame[4]=NULL;*(BYTE **)(primary+0xc)=other;cjk_direct_layout(font,frame);CHECK(!allocated);
    *(BYTE **)(primary+0xc)=NULL;
    /* A primary already in a viewport needs its new alternate attached now. */
    add(view,primary);SetLastError(4321);cjk_direct_layout(font,frame);
    CHECK(allocated==1&&cjk_pair_count==1&&frame[4]==visuals[0]);
    CHECK(cjk_view_contains(view,visuals[0])&&*(DWORD *)(visuals[0]+0x178)==2);
    CHECK(!memcmp(visuals[0]+0x28,primary+0x28,0x50)&&!memcmp(visuals[0]+0x90,cyan,16));
    CHECK(*(float *)(visuals[0]+0x78)==77&&*(float *)(visuals[0]+0x80)==.25f&&*(DWORD *)(visuals[0]+0x18)==123);
    frame[4]=NULL;frame[2]=L"ASCII again";cjk_direct_layout(font,frame);
    CHECK(allocated==1&&frame[4]==visuals[0]&&*(DWORD *)(visuals[0]+0x178)==2);
    primary[0x14]=0;cjk_direct_visible(primary,FALSE);CHECK(!visuals[0][0x14]);
    *(float *)(primary+0x68)=900;SetLastError(4321);cjk_direct_dirty(primary);
    CHECK(*(float *)(visuals[0]+0x68)==900&&GetLastError()==4321);
    float white[4]={1,1,1,1};cjk_direct_colour(primary,white);CHECK(!memcmp(visuals[0]+0x90,white,16));
    memcpy(primary+0x90,white,16);*(float *)(primary+0x68)=912;
    cjk_direct_refresh();CHECK(*(float *)(visuals[0]+0x68)==912&&!memcmp(visuals[0]+0x90,white,16));
    cjk_direct_dirty(other);CHECK(!cjk_pair(other));
    frame[4]=other;cjk_direct_layout(font,frame);CHECK(cleared==1&&!cjk_view_contains(view,visuals[0]));
    frame[4]=NULL;cjk_direct_layout(font,frame);CHECK(cjk_view_contains(view,visuals[0]));
    cjk_direct_remove(view,primary);remove_visual(view,primary);
    CHECK(!cjk_view_contains(view,visuals[0])&&*(DWORD *)(visuals[0]+0x178)==1);
    cjk_direct_add(view,primary);CHECK(cjk_view_contains(view,visuals[0]));
    cjk_direct_remove(view,primary);cjk_direct_destroy(primary+0x174);
    CHECK(destroyed==1&&!cjk_pair_count&&!cjk_pair(primary));
    /* Primary dies during native viewport clearing: leave the viewport's
       independent SSP intact, without mutating its iterated vector. */
    frame[2]=L"\x3042";frame[4]=NULL;cjk_direct_layout(font,frame);CHECK(allocated==2);
    cjk_direct_add(view,primary);unsigned n=*(DWORD *)(visuals[1]+0x178);
    cjk_direct_destroy(primary+0x174);CHECK(!cjk_pair_count&&destroyed==1);
    CHECK(cjk_view_contains(view,visuals[1])&&*(DWORD *)(visuals[1]+0x178)==n-1);
    remove_visual(view,visuals[1]);
    CHECK(!invalid);CHECK(dirty>0);
    VirtualFree(cjk_direct_base,0,MEM_RELEASE);
    puts("PASS: opt-in native visual pairing, atlas isolation, existing/new viewport attachment, reuse, ASCII clearing path, colour/visibility/transform sync and independent reference lifetimes");return 0;
}
