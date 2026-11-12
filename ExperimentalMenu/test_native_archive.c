#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
typedef void *(__fastcall *ArchiveOpen)(const WCHAR *);
typedef void (__fastcall *ArchiveClose)(void *);
typedef BYTE (WINAPI *ArchiveValidate)(void *);
static void __cdecl glyph_blocks(void *,void *,void *,void *,unsigned,float *,float *,float *);
static void __cdecl split_blocks(void *,void *,void *);
static void *vertical_code;
static BYTE hd_visuals[8][0x180];static unsigned hd_allocations,hd_destroys;
static void *__thiscall hd_allocate(void *pool,unsigned size,unsigned stride){
    (void)pool;if(size!=380||stride!=380||hd_allocations>=8)return NULL;return hd_visuals[hd_allocations++];
}
static void *__thiscall hd_construct(void *visual,const int *resource,const int *texture,int virtual_base){
    (void)virtual_base;memset(visual,0,0x180);*(int *)((BYTE *)visual+0x18)=*resource;*(int *)((BYTE *)visual+0x1c)=*texture;return visual;
}
static void __thiscall hd_resource(void *visual,const int *token){*(int *)((BYTE *)visual+0x18)=*token;}
static void __thiscall hd_texture(void *visual,const int *token){*(int *)((BYTE *)visual+0x1c)=*token;}
static void *__thiscall hd_destroy(void *ref,unsigned flags){(void)ref;(void)flags;hd_destroys++;return NULL;}
static BOOL hd_fixture(BYTE *base,BYTE *font){
    CjkAtlas atlas={0};atlas.pages=3;atlas.pixels=96;
    for(unsigned p=0;p<3;p++){atlas.tokens_ready[p]=TRUE;atlas.resource[p]=123+p;atlas.texture[p]=456+p;}
    cjk_direct_base=base;cjk_direct_enabled=TRUE;
    cjk_visual_allocate=hd_allocate;cjk_visual_construct_texture=hd_construct;
    cjk_page_resource=hd_resource;cjk_page_texture=hd_texture;cjk_destroy_original=hd_destroy;
    cjk_visual_child=(void (__thiscall *)(void *,void *))(base+0x1eef0);
    cjk_dirty_original=base+0x53580;cjk_visible_original=base+0x535e0;cjk_direct_colour_original=base+0x42f50;
    cjk_font_count=1;cjk_fonts[0].atlas=&atlas;*(int *)(font+8)=123;*(int *)(font+0xc)=456;
    BYTE visual[0x180]={0},nodes[4][64];*(int *)(visual+0x18)=123;*(int *)(visual+0x1c)=456;
    visual[0x14]=1;*(float *)(visual+0x90)=.25f;*(float *)(visual+0xc4)=.5f;
    visual[0xd4]=1;*(float *)(visual+0xd8)=100;
    const unsigned pages[]={2,0,1,2};
    for(unsigned iteration=0;iteration<3;iteration++){
        for(unsigned i=0;i<4;i++){
            ((void *(__thiscall *)(void *,unsigned))(base+0x1ee00))(nodes[i],1);
            *(DWORD *)(nodes[i]+0x3c)=1; /* one fixture owner, not a fake high count */
            *(float *)(nodes[i]+4)=pages[i]*2.f+.25f;*(float *)(nodes[i]+0xc)=.5f;
            *(float *)(nodes[i]+0x14)=-40.f+i*25;*(float *)(nodes[i]+0x18)=-18;nodes[i][0x34]=1;
            if(i){*(BYTE **)(nodes[i-1]+0x24)=nodes[i];++*(DWORD *)(nodes[i]+0x3c);}
        }
        cjk_visual_child(visual,nodes[0]);if(!cjk_route_pages(visual))return FALSE;
        if(hd_allocations!=2||cjk_pair_count!=2)return FALSE;
        BYTE *groups[]={visual,hd_visuals[0],hd_visuals[1]};unsigned seen=0;
        for(unsigned p=0;p<3;p++){
            if(*(int *)(groups[p]+0x18)!=(int)(123+p)||*(int *)(groups[p]+0x1c)!=(int)(456+p))return FALSE;
            for(BYTE *n=*(BYTE **)(groups[p]+0xbc);n;n=*(BYTE **)(n+0x24)){
                uintptr_t distance=(uintptr_t)n-(uintptr_t)nodes;if(distance>=sizeof(nodes)||distance%64)return FALSE;
                unsigned i=(unsigned)(distance/64);if(pages[i]!=p||(seen&(1u<<i)))return FALSE;seen|=1u<<i;
                if(*(float *)(n+4)!=.25f||*(float *)(n+0xc)!=.5f||*(float *)(n+0x14)!=-40.f+i*25
                    ||*(float *)(n+0x18)!=-18||*(DWORD *)(n+0x3c)!=2)return FALSE;
            }
        }
        if(seen!=15||*(float *)(hd_visuals[1]+0xc4)!=.5f||*(float *)(hd_visuals[1]+0xd8)!=100)return FALSE;
        /* Native empty redraw must clear old extra page glyphs. */
        cjk_visual_child(visual,NULL);if(!cjk_route_pages(visual))return FALSE;
        for(unsigned p=0;p<3;p++)if(*(BYTE **)(groups[p]+0xbc))return FALSE;
        for(unsigned i=0;i<4;i++){
            BYTE *next=*(BYTE **)(nodes[i]+0x24);*(BYTE **)(nodes[i]+0x24)=NULL;if(next)cjk_glyph_release(next);
        }
        for(unsigned i=0;i<4;i++)if(*(DWORD *)(nodes[i]+0x3c)!=1)return FALSE;
    }
    cjk_direct_destroy(visual+0x174);
    if(cjk_pair_count||hd_destroys!=2)return FALSE;
    cjk_direct_enabled=FALSE;cjk_font_count=0;cjk_fonts[0].atlas=NULL;
    *(int *)(font+8)=-1;*(int *)(font+0xc)=-1;return TRUE;
}
static BYTE __thiscall font_dimensions(void *self,const int *resource,const char *name,float *w,float *h){
    (void)self;(void)resource;(void)name;*w=8192;*h=8192;return 1;
}
__attribute__((naked,noinline)) static void split_return(void){__asm__("ret\n");}
__attribute__((naked,noinline)) static void __cdecl alignment_blocks(void *entry,void *primary,void *font,unsigned mode){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ebx\npush %esi\npush %edi\nsub $256,%esp\n"
        "mov %ebp,240(%esp)\nmov 8(%ebp),%eax\nmov 12(%ebp),%esi\n"
        "mov 16(%ebp),%edx\nmov %edx,16(%esp)\nmov 20(%ebp),%ecx\n"
        "lea 128(%esp),%ebp\nmov %ecx,28(%ebp)\nxorps %xmm3,%xmm3\ncall *%eax\n"
        "mov 240(%esp),%ebp\nadd $256,%esp\npop %edi\npop %esi\npop %ebx\nleave\nret\n");
}
__attribute__((naked,noinline)) static void __cdecl vertical_blocks(void *entry,void *primary,void *alternate){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ebx\npush %esi\npush %edi\nsub $256,%esp\n"
        "mov %ebp,240(%esp)\nmov 8(%ebp),%eax\nmov 12(%ebp),%edx\nmov 16(%ebp),%edi\n"
        "movl $0x42100000,20(%esp)\nlea 128(%esp),%ebp\nmov %edx,16(%ebp)\n"
        "mov %edi,20(%ebp)\nmovl $1,32(%ebp)\ncall *%eax\n"
        "mov 240(%esp),%ebp\nadd $256,%esp\npop %edi\npop %esi\npop %ebx\nleave\nret\n");
}
static unsigned alignment_test_page;
static float alignment_fixture(BYTE *base,BYTE *font,BYTE *fallback,unsigned mode,unsigned pattern,BOOL single_atlas,float *positions){
    BYTE primary[0x180]={0},nodes[3][64];
    BOOL alternate[3]={TRUE,TRUE,TRUE};
    if(pattern==1){alternate[0]=FALSE;alternate[2]=FALSE;}
    if(pattern==2)for(unsigned i=0;i<3;i++)alternate[i]=FALSE;
    if(pattern==3)alternate[0]=FALSE;
    const float offsets[4][3]={{0,32,32},{2,8,34},{2,10,10},{2,8,32}};
    for(unsigned i=0;i<3;i++){
        ((void *(__thiscall *)(void *,unsigned))(base+0x1ee00))(nodes[i],1);
        *(DWORD *)(nodes[i]+0x3c)=100;
        *(float *)(nodes[i]+4)=alternate[i]&&!single_atlas?2.25f+alignment_test_page*2.f:.25f;
        *(float *)(nodes[i]+0x14)=offsets[pattern][i];
        *(float *)(nodes[i]+0x1c)=alternate[i]?24.f/(single_atlas?1024:8192):8.f/1024;
        nodes[i][0x34]=i==0;
        if(i)*(BYTE **)(nodes[i-1]+0x24)=nodes[i];
    }
    *(BYTE **)(font+0x68)=fallback;*(float *)(font+0x10)=1024;
    *(BYTE **)(primary+0xbc)=nodes[0];
    alignment_blocks(base+0x55d3c,primary,font,mode);
    /* Native alignment must not erase the texture-selection tags. */
    for(unsigned i=0;i<3;i++)if(*(float *)(nodes[i]+4)!=(alternate[i]&&!single_atlas?2.25f+alignment_test_page*2.f:.25f))return 99999;
    float first=*(float *)(nodes[0]+0x14);
    if(positions){
        BYTE alternate_visual[0x180]={0};
        *(DWORD *)(alternate_visual+0x18)=123;*(DWORD *)(alternate_visual+0x1c)=456;
        split_blocks(base+0x55f43,primary,alternate_visual);
        vertical_blocks(vertical_code,primary,alternate_visual);
        BYTE *lists[]={*(BYTE **)(primary+0xbc),*(BYTE **)(alternate_visual+0xbc)};unsigned seen=0;
        for(unsigned list=0;list<2;list++){
            float x=0,y=0;unsigned count=0;
            for(BYTE *node=lists[list];node;node=*(BYTE **)(node+0x24)){
                if(++count>3)return 99999;
                uintptr_t distance=(uintptr_t)node-(uintptr_t)nodes;
                if(distance>=sizeof(nodes)||distance%64)return 99999;
                unsigned index=(unsigned)(distance/64);if(seen&(1u<<index))return 99999;seen|=1u<<index;
                x=node[0x34]?*(float *)(node+0x14):x+*(float *)(node+0x14);positions[index]=x;
                y=node[0x34]?*(float *)(node+0x18):y+*(float *)(node+0x18);positions[index+3]=y;
            }
        }
        if(seen!=7)return 99999;
    }
    return first;
}
__attribute__((naked,noinline)) static void __cdecl split_blocks(void *entry,void *primary,void *alternate){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ebx\npush %esi\npush %edi\nsub $256,%esp\n"
        "mov %ebp,240(%esp)\nmov 8(%ebp),%eax\nmov 12(%ebp),%edx\nmov 16(%ebp),%edi\n"
        "lea 128(%esp),%ebp\nmov %edx,16(%ebp)\nmov %edi,20(%ebp)\ncall *%eax\n"
        "mov 240(%esp),%ebp\nadd $256,%esp\npop %edi\npop %esi\npop %ebx\nleave\nret\n");
}
static BOOL split_fixture(BYTE *base,BYTE *blocks,BYTE *latin,BYTE *glyphs,BOOL expect_correct,unsigned pattern){
    BYTE nodes[9][64],primary[0x180]={0},alternate[0x180]={0};
    BYTE *sequence[]={latin,latin,glyphs,latin,latin,glyphs+44,glyphs,glyphs+44,latin};
    if(pattern==1)for(unsigned i=0;i<9;i++)sequence[i]=glyphs+(i%2)*44;
    if(pattern==2){sequence[0]=glyphs+44;sequence[1]=glyphs;}
    if(pattern==3)for(unsigned i=0;i<9;i++)sequence[i]=latin;
    float expected_x[9],expected_y[9],advance=0,x=0,y=0,width=0;
    for(unsigned i=0;i<9;i++){
        ((void *(__thiscall *)(void *,unsigned))(base+0x1ee00))(nodes[i],1);
        *(unsigned *)(nodes[i]+0x3c)=100; /* Fixture owns references; no stack deletion. */
        BOOL fallback=sequence[i]!=latin;
        glyph_blocks(blocks,blocks+512,nodes[i],sequence[i],fallback,&x,&y,&width);
        nodes[i][0x34]=i==0;
        if(i)*(BYTE **)(nodes[i-1]+0x24)=nodes[i];
        expected_x[i]=advance+*(float *)(sequence[i]+0x1c);
        expected_y[i]=*(float *)(sequence[i]+0x20);advance+=*(float *)(sequence[i]+0x24);
    }
    if(pattern==4){ /* Native wrapping/alignment may already anchor a later node. */
        nodes[7][0x34]=1;*(float *)(nodes[7]+0x14)=expected_x[7];*(float *)(nodes[7]+0x18)=expected_y[7];
    }
    *(BYTE **)(primary+0xbc)=nodes[0];
    *(int *)(alternate+0x18)=123;*(int *)(alternate+0x1c)=456;
    split_blocks(base+0x55f43,primary,alternate);
    unsigned seen=0;BOOL correct=TRUE;
    BYTE *lists[]={*(BYTE **)(primary+0xbc),*(BYTE **)(alternate+0xbc)};
    for(unsigned group=0;group<2;group++){
        float world_x=0,world_y=0;unsigned limit=0;
        for(BYTE *node=lists[group];node;node=*(BYTE **)(node+0x24)){
            if(++limit>9)return FALSE;
            uintptr_t distance=(uintptr_t)node-(uintptr_t)nodes;
            if(distance>=sizeof(nodes)||distance%64)return FALSE;
            unsigned i=(unsigned)(distance/64);
            if(seen&(1u<<i))return FALSE;seen|=1u<<i;
            world_x=node[0x34]?*(float *)(node+0x14):world_x+*(float *)(node+0x14);
            world_y=node[0x34]?*(float *)(node+0x18):world_y+*(float *)(node+0x18);
            if(world_x!=expected_x[i]||world_y!=expected_y[i]){
                correct=FALSE;
                if(expect_correct)fprintf(stderr,"split glyph %u: actual=(%.1f,%.1f) expected=(%.1f,%.1f)\n",i,world_x,world_y,expected_x[i],expected_y[i]);
            }
            if(group!=(unsigned)(sequence[i]!=latin))return FALSE;
        }
    }
    return seen==0x1ff&&correct==expect_correct;
}
/* Run recovered glyph-position/pen blocks against their real ESP-relative
 * locals. The two blocks return to this runner instead of entering rendering. */
__attribute__((naked,noinline)) static void __cdecl glyph_blocks(void *position,void *pen,void *node,void *glyph,
    unsigned fallback,float *x,float *y,float *width){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ebx\npush %esi\npush %edi\nsub $128,%esp\n"
        "mov 20(%ebp),%eax\nmov %eax,28(%esp)\nmov 24(%ebp),%eax\nmov %al,10(%esp)\n"
        "movl $0x3f800000,12(%esp)\nmov 28(%ebp),%eax\nmovss (%eax),%xmm0\nmovss %xmm0,24(%esp)\n"
        "mov 32(%ebp),%eax\nmovss (%eax),%xmm0\nmovss %xmm0,44(%esp)\n"
        "mov 36(%ebp),%eax\nmovss (%eax),%xmm0\nmovss %xmm0,32(%esp)\n"
        "mov 16(%ebp),%edi\ncall *8(%ebp)\ncall *12(%ebp)\n"
        "mov 28(%ebp),%eax\nmovss 24(%esp),%xmm0\nmovss %xmm0,(%eax)\n"
        "mov 32(%ebp),%eax\nmovss 44(%esp),%xmm0\nmovss %xmm0,(%eax)\n"
        "mov 36(%ebp),%eax\nmovss 32(%esp),%xmm0\nmovss %xmm0,(%eax)\n"
        "add $128,%esp\npop %edi\npop %esi\npop %ebx\nleave\nret\n");
}
int main(int argc,char **argv){
    CHECK(argc==4);WCHAR exe[MAX_PATH],zip[MAX_PATH],dat[MAX_PATH];
    CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,exe,MAX_PATH));CHECK(supported_file(exe));
    CHECK(MultiByteToWideChar(CP_UTF8,0,argv[2],-1,zip,MAX_PATH));CHECK(MultiByteToWideChar(CP_UTF8,0,argv[3],-1,dat,MAX_PATH));
    BYTE *base=(BYTE *)LoadLibraryExW(exe,NULL,DONT_RESOLVE_DLL_REFERENCES);CHECK(base);
    IMAGE_NT_HEADERS32 *nt=(void *)(base+((IMAGE_DOS_HEADER *)base)->e_lfanew);
    /* Wine maps an EXE without relocating its native code in this mode.
       Relocate this private test mapping only; never change the source file. */
    DWORD delta=(DWORD)(uintptr_t)base-nt->OptionalHeader.ImageBase;
    if(delta && *(DWORD *)(base+0x198433)==0x0092d39c){
        IMAGE_DATA_DIRECTORY reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        CHECK(reloc.VirtualAddress && reloc.Size);
        DWORD cursor=0;
        while(cursor<reloc.Size){
            IMAGE_BASE_RELOCATION *block=(void *)(base+reloc.VirtualAddress+cursor);
            CHECK(block->SizeOfBlock>=8 && block->SizeOfBlock<=reloc.Size-cursor);
            WORD *entries=(void *)(block+1);
            for(DWORD i=0;i<(block->SizeOfBlock-8)/2;i++){
                unsigned type=entries[i]>>12;if(type==IMAGE_REL_BASED_ABSOLUTE)continue;
                CHECK(type==IMAGE_REL_BASED_HIGHLOW);
                DWORD *target=(void *)(base+block->VirtualAddress+(entries[i]&0xfff));
                DWORD old,ignored;CHECK(VirtualProtect(target,4,PAGE_READWRITE,&old));
                *target+=delta;CHECK(VirtualProtect(target,4,old,&ignored));
            }
            cursor+=block->SizeOfBlock;
        }
        FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);
    }
    IMAGE_IMPORT_DESCRIPTOR *imports=(void *)(base+nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    for(unsigned i=0;imports[i].Name;i++){
        const char *name=(char *)(base+imports[i].Name);
        if(_stricmp(name,"kernel32.dll")&&_stricmp(name,"msvcr120.dll")&&_stricmp(name,"msvcp120.dll"))continue;
        HMODULE library=LoadLibraryA(name);CHECK(library);
        IMAGE_THUNK_DATA32 *names=(void *)(base+imports[i].OriginalFirstThunk),*slots=(void *)(base+imports[i].FirstThunk);
        for(unsigned j=0;names[j].u1.AddressOfData;j++){
            const char *symbol=IMAGE_SNAP_BY_ORDINAL32(names[j].u1.Ordinal)?(const char *)(uintptr_t)IMAGE_ORDINAL32(names[j].u1.Ordinal):(char *)((IMAGE_IMPORT_BY_NAME *)(base+names[j].u1.AddressOfData))->Name;
            FARPROC function=GetProcAddress(library,symbol);CHECK(function);DWORD old,ignored;CHECK(VirtualProtect(slots+j,4,PAGE_READWRITE,&old));
            slots[j].u1.Function=(DWORD)(uintptr_t)function;VirtualProtect(slots+j,4,old,&ignored);
        }
    }
    HANDLE file=CreateFileW(dat,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);CHECK(file!=INVALID_HANDLE_VALUE);
    DWORD header[5],count;CHECK(ReadFile(file,header,sizeof(header),&count,NULL)&&count==sizeof(header));CloseHandle(file);CHECK(header[0]==0x0300ffee);
    ArchiveOpen open=(ArchiveOpen)(base+0x198420);ArchiveClose close=(ArchiveClose)(base+0x198770);
    CHECK(*(DWORD *)(base+0x198433)==(DWORD)(uintptr_t)(base+0x52d39c));
    /* The original JSON parser accepts raw UTF-8 without legacy conversion. */
    const char *json_fixtures[]={"{\"artist\":{\"name\":\"BUCK\xe2\x80\x90TICK\"}}",
        "{\"artist\":{\"name\":\"BUCK\\u2010TICK\"}}",
        "{\"artist\":{\"name\":\"ASCII\"}}"};
    for(unsigned i=0;i<3;i++){
        BYTE json[4]={0};
        CHECK(((BYTE (__thiscall *)(void *,const char *))(base+0x1d0b00))(json,json_fixtures[i]));
        const char *name=((const char *(__thiscall *)(void *,const char *))(base+0x1d0e90))(json,"artist/name");
        if(strcmp(name,i==2?"ASCII":i==1?"BUCKTICK":"BUCK\xe2\x80\x90TICK")){
            fprintf(stderr,"JSON fixture %u bytes:",i);for(unsigned k=0;name[k]&&k<64;k++)fprintf(stderr," %02X",(unsigned char)name[k]);fputc('\n',stderr);
        }
        CHECK(!strcmp(name,i==2?"ASCII":i==1?"BUCKTICK":"BUCK\xe2\x80\x90TICK"));
        ((void (__thiscall *)(void *))(base+0x1d0ad0))(json);
    }
    /* The original parser drops \u2010 in escaped JSON; record that separate
       limitation rather than claiming this response-copy patch fixes it. */
    puts("PASS: original JSON parser retains raw UTF-8/ASCII; separate escaped-Unicode loss reproduced");
    struct {void *handle;DWORD unused[3],crc;} record={open(zip),{0,0,0},header[4]};CHECK(record.handle);
    CHECK(((ArchiveValidate)(base+0x160cb0))(&record));
    record.crc^=1;CHECK(!((ArchiveValidate)(base+0x160cb0))(&record));close(record.handle);
    puts("PASS: original native ZIP/index validator accepts generated checksum and rejects a changed checksum");
    /* Execute the original font constructor with real CRT allocation and a
       controlled graphics interface. Negative tokens bypass string-manager
       ownership while retaining the real raw-glyph conversion and teardown. */
    void *methods[31]={0};methods[30]=font_dimensions;void **graphics=methods;
    void *previous=*(void **)(base+0x80d408);*(void **)(base+0x80d408)=&graphics;
    CjkRawGlyph raw[2]={{'?',1,2,3,4,5,6,7,0,{0}},{0x6708,16,32,24,28,0,4,32,0,{0}}};
    struct {void *begin,*end,*capacity;} vector={raw,raw+2,raw+2};
    BYTE font[0x70]={0};int token=-1;
    typedef void *(__thiscall *FontCtor)(void *,const int *,const int *,void *,BYTE,unsigned);
    CHECK(((FontCtor)(base+0x551b0))(font,&token,&token,&vector,0,0)==font);
    CHECK(*(float *)(font+0x10)==8192&&*(float *)(font+0x14)==8192);
    BYTE *glyphs=*(BYTE **)(font+0x28);
    CHECK(*(BYTE **)(font+0x2c)==glyphs+88&&*(BYTE **)(font+0x34)==glyphs);
    CHECK(*(WORD *)(glyphs+44)==0x6708);
    CHECK(*(float *)(glyphs+44+4)==16.f/8192&&*(float *)(glyphs+44+0x10)==60.f/8192);
    CHECK(*(float *)(glyphs+44+0x14)==24.f/8192&&*(float *)(glyphs+44+0x18)==28.f/8192);
    CHECK(*(float *)(glyphs+44+0x20)==4&&*(float *)(glyphs+44+0x24)==32);
    static const BYTE width_bytes[]={0x55,0x8b,0xec,0x51,0x53};
    CHECK(unicode_detour(base+0x56450,width_bytes,5,unicode_width_entry,&unicode_width_original));
    CHECK(((float (__thiscall *)(void *,const WCHAR *))(base+0x56450))(font,L"?\x6708")==39.f);
    BYTE *blocks=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);CHECK(blocks);
    unsigned position_size=0x55887-0x557fb,pen_size=0x55949-0x558ec;
    memcpy(blocks,base+0x557fb,position_size);blocks[position_size]=0xc3;
    memcpy(blocks+512,base+0x558ec,pen_size);blocks[512+pen_size]=0xc3;
    BYTE latin[44];memcpy(latin,glyphs,44);*(float *)(latin+0x1c)=2;*(float *)(latin+0x20)=4;*(float *)(latin+0x24)=10;
    BYTE node[64]={0};float x=0,y=0,width=0;
    glyph_blocks(blocks,blocks+512,node,latin,0,&x,&y,&width);
    CHECK(*(float *)(node+0x14)==2&&*(float *)(node+0x18)==4&&x==8&&y==-4&&width==10);
    float old_x=x,old_y=y,old_width=width;
    BYTE original_node[64];memcpy(original_node,node,64);
    glyph_blocks(blocks,blocks+512,original_node,glyphs+44,1,&old_x,&old_y,&old_width);
    static const BYTE y_bytes[]={0x8b,0x41,0x20,0x89,0x47,0x18},pen_bytes[]={0x80,0x7c,0x24,0x0e,0x00};
    cjk_y_after=blocks+position_size;cjk_pen_after=blocks+512+5;
    CHECK(unicode_detour(blocks+(0x55881-0x557fb),y_bytes,6,cjk_y_entry,&cjk_y_original));
    CHECK(unicode_detour(blocks+512,pen_bytes,5,cjk_pen_entry,&cjk_pen_original));
    /* Foreign fallback glyphs must retain every original output. */
    glyph_blocks(blocks,blocks+512,node,glyphs+44,1,&x,&y,&width);
    CHECK(!memcmp(node,original_node,64)&&x==old_x&&y==old_y&&width==old_width);
    cjk_font_count=1;cjk_fonts[0].font=font;
    x=0;y=0;width=0;float world_x=0,world_y=0;
    void *sequence[]={latin,glyphs+44,glyphs};
    const float expected_x[]={2,10,47},expected_y[]={4,4,6};
    for(unsigned i=0;i<3;i++){
        glyph_blocks(blocks,blocks+512,node,sequence[i],i!=0,&x,&y,&width);
        world_x+=*(float *)(node+0x14);world_y+=*(float *)(node+0x18);
        CHECK(world_x==expected_x[i]&&world_y==expected_y[i]);
        if(i)CHECK(*(float *)(node+4)>=2.f); /* Preserve fallback texture tag. */
    }
    CHECK(width==49.f);cjk_font_count=0;
    cjk_font_count=1;x=0;y=0;width=0;world_x=0;world_y=0;
    float advance=0;
    for(unsigned i=0;i<60;i++){
        BYTE *g=sequence[i%3];
        glyph_blocks(blocks,blocks+512,node,g,i%3!=0,&x,&y,&width);
        world_x+=*(float *)(node+0x14);world_y+=*(float *)(node+0x18);
        CHECK(world_x==advance+*(float *)(g+0x1c)&&world_y==*(float *)(g+0x20));
        advance+=*(float *)(g+0x24);CHECK(width==advance);
    }
    cjk_font_count=0;
    puts("PASS: original chained glyph blocks keep owned CJK baselines/advances and preserve foreign fallback outputs");
    static const BYTE stop_bytes[]={0x8b,0x55,0x10,0x8b,0x45,0x20};void *stop_original;
    CHECK(unicode_detour(base+0x56232,stop_bytes,6,split_return,&stop_original));
    /* The split-return detour overlaps the first vertical instruction.
       Enter that original instruction in its unchanged trampoline. */
    vertical_code=(BYTE *)stop_original+3;
    cjk_font_count=1;*(int *)(font+8)=123;*(int *)(font+0xc)=456;
    CHECK(split_fixture(base,blocks,latin,glyphs,FALSE,0)); /* Reproduce missed composition stage. */
    static const BYTE split_bytes[]={0xf3,0x0f,0x10,0x46,0x04};
    CHECK(unicode_detour(base+0x55fdf,split_bytes,5,cjk_split_entry,&cjk_split_original));
    for(unsigned pattern=0;pattern<5;pattern++)CHECK(split_fixture(base,blocks,latin,glyphs,TRUE,pattern));
    cjk_font_count=0;*(int *)(font+8)=-1;*(int *)(font+0xc)=-1;
    puts("PASS: original native visual splitting reproduces mixed-script regression and preserves corrected positions in both font lists");
    static const BYTE alignment_stop[]={0x8b,0x55,0x10,0x8b,0x4d,0x18};void *alignment_stop_original;
    CHECK(unicode_detour(base+0x55ed4,alignment_stop,6,split_return,&alignment_stop_original));
    static const BYTE vertical_stop[]={0x8b,0x75,0x08,0xf3,0x0f,0x10,0x44,0x24,0x24};void *vertical_stop_original;
    CHECK(unicode_detour(base+0x563d9,vertical_stop,9,split_return,&vertical_stop_original));
    BYTE primary_font[0x70]={0};
    float foreign[2][4],reference[2][4],reference_positions[2][4][6];
    for(unsigned mode=1;mode<=2;mode++)for(unsigned pattern=0;pattern<4;pattern++){
        foreign[mode-1][pattern]=alignment_fixture(base,primary_font,font,mode,pattern,FALSE,NULL);
        reference[mode-1][pattern]=alignment_fixture(base,primary_font,font,mode,pattern,TRUE,reference_positions[mode-1][pattern]);
    }
    CHECK(foreign[0][0]!=-44&&foreign[1][0]!=-88); /* Reproduce the centred pause-title regression. */
    BYTE alignment_bytes[]={0xf3,0x0f,0x10,0x05,0,0,0,0};
    DWORD alignment_constant=(DWORD)(uintptr_t)(base+0x606618);memcpy(alignment_bytes+4,&alignment_constant,4);
    static const BYTE alignment_width[]={0xf3,0x0f,0x59,0x40,0x10};
    cjk_align_after=base+0x55d98;cjk_align_width_after=base+0x55ddb;
    CHECK(unicode_detour(base+0x55d90,alignment_bytes,8,cjk_align_entry,&cjk_align_original));
    CHECK(unicode_detour(base+0x55dd6,alignment_width,5,cjk_align_width_entry,&cjk_align_width_original));
    for(unsigned mode=1;mode<=2;mode++)for(unsigned pattern=0;pattern<4;pattern++)
        CHECK(alignment_fixture(base,primary_font,font,mode,pattern,FALSE,NULL)==foreign[mode-1][pattern]);
    cjk_font_count=1;*(DWORD *)(font+8)=123;*(DWORD *)(font+0xc)=456;
    for(alignment_test_page=0;alignment_test_page<7;alignment_test_page++)for(unsigned mode=1;mode<=2;mode++)for(unsigned pattern=0;pattern<4;pattern++){
        /* Match the original single-atlas algorithm, including its native
           first-node/bearing and rounding rules; do not invent centring. */
        float expected=reference[mode-1][pattern];
        float positions[6];
        float actual=alignment_fixture(base,primary_font,font,mode,pattern,FALSE,positions);
        if(actual!=expected)fprintf(stderr,"alignment mode %u pattern %u actual %.1f expected %.1f\n",mode,pattern,actual,expected);
        CHECK(actual==expected);
        CHECK(!memcmp(positions,reference_positions[mode-1][pattern],sizeof(positions)));
    }
    alignment_test_page=0;cjk_font_count=0;*(int *)(font+8)=-1;*(int *)(font+0xc)=-1;
    puts("PASS: original centre/right alignment reproduces skipped CJK advances and wrong atlas width; owned-only extension restores mixed/all-CJK alignment while preserving Latin, foreign fallbacks and texture tags");
    CHECK(hd_fixture(base,font));
    puts("PASS: paged glyph routing uses original native child ownership with exact positions/UVs, repeated reuse, empty redraw clearing, pivot/rotation propagation and recursive visual release");
    BYTE text[0x1f8]={0},primary_colour[0x180]={0},alternate_colour[0x180]={0};
    *(BYTE **)(text+0x1c4)=primary_colour;*(BYTE **)(text+0x1c8)=alternate_colour;
    const float white[4]={1,1,1,1},cyan[4]={0,.75f,.8f,1},dim[4]={.3f,.3f,.3f,.5f};
    memcpy(alternate_colour+0x90,white,16);
    *(int *)(alternate_colour+0x18)=123;*(int *)(alternate_colour+0x1c)=456;
    typedef void (__thiscall *TextColour)(void *,const float *);
    TextColour colour=(TextColour)(base+0x50c40);
    colour(text,cyan);
    CHECK(!memcmp(primary_colour+0x90,cyan,16)&&!memcmp(alternate_colour+0x90,white,16));
    cjk_font_count=1;*(int *)(font+8)=123;*(int *)(font+0xc)=456;
    cjk_visual_colour=(TextColour)(base+0x42f50);
    static const BYTE colour_bytes[]={0x55,0x8b,0xec,0x56,0xff,0x75,0x08};
    CHECK(unicode_detour(base+0x50c40,colour_bytes,7,cjk_colour_entry,&cjk_colour_original));
    const float *colours[]={cyan,white,dim};
    for(unsigned i=0;i<3;i++){
        *(unsigned *)(alternate_colour+0x170)=0;
        colour(text,colours[i]);
        CHECK(!memcmp(text+0x54,colours[i],16)&&!memcmp(primary_colour+0x90,colours[i],16)
            &&!memcmp(alternate_colour+0x90,colours[i],16));
        CHECK(*(unsigned *)(alternate_colour+0x170)&0x100);
    }
    *(int *)(alternate_colour+0x1c)=999;memcpy(alternate_colour+0x90,white,16);
    *(unsigned *)(alternate_colour+0x170)=0;colour(text,cyan);
    CHECK(!memcmp(alternate_colour+0x90,white,16)&&!*(unsigned *)(alternate_colour+0x170));
    *(int *)(alternate_colour+0x1c)=456;*(DWORD *)(text+0x10)=3;
    memcpy(primary_colour+0x90,white,16);colour(text,dim);
    CHECK(!memcmp(text+0x54,dim,16)&&!memcmp(primary_colour+0x90,white,16)&&!memcmp(alternate_colour+0x90,white,16));
    *(DWORD *)(text+0x10)=0;*(BYTE **)(text+0x1c8)=NULL;colour(text,cyan);
    CHECK(!memcmp(primary_colour+0x90,cyan,16));
    cjk_font_count=0;*(int *)(font+8)=-1;*(int *)(font+0xc)=-1;
    puts("PASS: original colour setter reproduces white fallback, then native highlight/reset/RGBA propagation preserves foreign artwork and state-3 behavior");
    CHECK(GetTempPathW(MAX_PATH,config_path));
    WCHAR temporary[MAX_PATH];CHECK(GetTempFileNameW(config_path,L"rrd",0,temporary));wcscpy(config_path,temporary);
    CHECK(WritePrivateProfileStringW(L"languages",L"cjk_enabled",L"true",config_path));
    cjk_render_enabled=1;cjk_install_direct(base);CHECK(cjk_direct_enabled);
    CHECK(cjk_add_original&&cjk_remove_original&&cjk_destroy_original&&cjk_dirty_original&&cjk_visible_original&&cjk_direct_colour_original);
    DeleteFileW(config_path);
    puts("PASS: all six direct-visual hook boundaries match relocated original instructions; native pool/constructor/child APIs match guards");
    ((void (__thiscall *)(void *))(base+0x55090))(font);
    *(void **)(base+0x80d408)=previous;
    puts("PASS: original font construction, native UV/metric conversion, default glyph and destruction");return 0;
}
