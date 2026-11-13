/* Optional CJK font resources, using the original native texture loader. */
static void cjk_resource_probe(void){
    static LONG attempted;
    if(InterlockedCompareExchange(&attempted,1,0))return;
    UINT mode=GetPrivateProfileIntW(L"diagnostics",L"cjk_resource_probe",0,config_path);
    if(!mode || mode>2)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!readable(base+0x80d408,4)||!readable(base+0x18a90,5))return;
    void *graphics=*(void **)(base+0x80d408);
    if(!readable(graphics,4)||!readable(*(void ***)graphics,31*4))return;
    if((*(void ***)graphics)[30]!=base+0x94d60)return;
    static const BYTE token_bytes[]={0x55,0x8b,0xec,0x83,0xe4,0xf8},dimension_bytes[]={0x55,0x8b,0xec,0x6a,0xff};
    if(!readable(base+0x18a90,sizeof(token_bytes))||!readable(base+0x94d60,sizeof(dimension_bytes))
        ||memcmp(base+0x18a90,token_bytes,sizeof(token_bytes))||memcmp(base+0x94d60,dimension_bytes,sizeof(dimension_bytes)))return;
    typedef void *(__thiscall *TokenCtor)(void *,const char *);
    typedef BYTE (__thiscall *Dimensions)(void *,const int *,const char *,float *,float *);
    static int resource=-1;float w=0,h=0;
    const char *path=mode==2?"fonts/reriff/cjk/jp":"fonts/reriff/probe";
    const char *texture=mode==2?"reriff_cjk_jp":"reriff_probe";
    ((TokenCtor)(base+0x18a90))(&resource,path);
    BYTE found=((Dimensions)(*(void ***)graphics)[30])(graphics,&resource,texture,&w,&h);
    char line[240];snprintf(line,sizeof(line),"CJK native resource probe: found=%u width=%.0f height=%.0f resource=%s.",found,w,h,path);log_line(line);
}

/* The Languages component enables these native fallback metrics and visuals.
 * Diagnostic resource probes remain separately opt-in. */
typedef struct {DWORD code;float x,y,w,h,left,top,advance;} CjkMetric;
typedef struct {DWORD code;float x,y,w,h,left,top,advance;BYTE flag,pad[3];} CjkRawGlyph;
_Static_assert(sizeof(CjkMetric)==32,"RRCF record size");
_Static_assert(sizeof(CjkRawGlyph)==36,"original raw font record size");
#define CJK_FONT_LIMIT 64
#define CJK_MAX_PAGES 8
typedef struct {DWORD code,page;float x,y,w,h,left,top,advance,design_w,design_h;} CjkPageMetric;
_Static_assert(sizeof(CjkPageMetric)==44,"paged font metric size");
typedef struct {
    CjkPageMetric *metrics;DWORD count,ascent,pixels,pages;
    BOOL attempted,tokens_ready[CJK_MAX_PAGES],latin[CJK_MAX_PAGES];int resource[CJK_MAX_PAGES],texture[CJK_MAX_PAGES];
} CjkAtlas;
typedef struct {float scale,baseline;BYTE *font;CjkAtlas *atlas;} CjkFontCache;
static CjkFontCache cjk_fonts[CJK_FONT_LIMIT];
static CjkAtlas cjk_atlases[3];
static unsigned cjk_font_count;
static BOOL cjk_hd_ready;
static BOOL cjk_hd_capacity(void);
static CjkFontCache *cjk_visual_font(BYTE *visual){
    if(!readable(visual,0x20))return NULL;
    for(unsigned i=0;i<cjk_font_count;i++){
        BYTE *font=cjk_fonts[i].font;
        if(*(int *)(font+8)>=0&&*(int *)(visual+0x18)==*(int *)(font+8)
            &&*(int *)(visual+0x1c)==*(int *)(font+0xc))return cjk_fonts+i;
    }
    return NULL;
}
static int cjk_render_enabled=-1;
static void *cjk_y_original,*cjk_y_after,*cjk_pen_original,*cjk_pen_after;
static void *cjk_split_original;
static void *cjk_align_original,*cjk_align_after,*cjk_align_width_original,*cjk_align_width_after;
__attribute__((used)) static const float cjk_alignment_tag_limit=19.f;
/* Alternate-atlas tags stay intact for native extraction. During horizontal
   alignment, our chained language glyphs need the normal pen accumulation
   and the last glyph's own atlas dimensions instead of the primary atlas. */
__attribute__((used,noinline)) static BYTE *__cdecl cjk_alignment_font(BYTE *font,BYTE *node){
    if(!cjk_font_count||!(*(float *)(node+4)>=2.f)||!readable(font,0x70))return NULL;
    BYTE *fallback=*(BYTE **)(font+0x68);
    for(unsigned i=0;i<cjk_font_count;i++)if(cjk_fonts[i].font==fallback)return fallback;
    return NULL;
}
__attribute__((used,noinline)) static void __cdecl cjk_alignment_multiply(BYTE *font,BYTE *node,float *width){
    DWORD error=GetLastError();BYTE *fallback=cjk_alignment_font(font,node);
    *width*=*(float *)((fallback?fallback:font)+0x10);SetLastError(error);
}
__attribute__((naked,noinline)) static void cjk_align_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 4(%ebx)\npush 56(%ebx)\ncall _cjk_alignment_font\nadd $8,%esp\ntest %eax,%eax\njnz 1f\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_align_original\n"
        "1: fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\nmovss _cjk_alignment_tag_limit,%xmm0\njmp *_cjk_align_after\n");
}
__attribute__((naked,noinline)) static void cjk_align_width_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "lea 160(%esp),%eax\npush %eax\npush 4(%ebx)\npush 28(%ebx)\n"
        "call _cjk_alignment_multiply\nadd $12,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_align_width_after\n");
}
static void *cjk_colour_original;
static void (__thiscall *cjk_visual_colour)(void *,const float *);
static void __cdecl cjk_direct_colour(BYTE *,const float *);
__attribute__((used,noinline)) static void __cdecl cjk_colour_after(BYTE *text,const float *colour){
    DWORD error=GetLastError();
    if(cjk_font_count&&cjk_visual_colour&&readable(text,0x1cc)&&*(DWORD *)(text+0x10)!=3){
        BYTE *alternate=*(BYTE **)(text+0x1c8);
        if(cjk_visual_font(alternate)){cjk_visual_colour(alternate,colour);cjk_direct_colour(alternate,colour);}
    }
    SetLastError(error);
}
__attribute__((naked,noinline)) static void cjk_colour_entry(void){
    __asm__("push %ebp\nmov %esp,%ebp\npush %ecx\npush 8(%ebp)\ncall *_cjk_colour_original\n"
        "pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 8(%ebp)\npush -4(%ebp)\ncall _cjk_colour_after\nadd $8,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\nmov %ebp,%esp\npop %ebp\nret $4\n");
}
__attribute__((used,noinline)) static int __cdecl cjk_prepare_split(BYTE *node,BYTE *alternate,float x,float y){
    if(!cjk_font_count||!readable(alternate,0x20)||!(*(float *)(node+4)>=2.f))return -1;
    if(!cjk_visual_font(alternate))return -1;
    /* Native splitting removes fallback nodes from the primary chain.
       Canonicalize this node before native removal, and propagate its world
       position only when the next node still has relative coordinates. */
    *(float *)(node+0x14)=x;*(float *)(node+0x18)=y;node[0x34]=1;
    BYTE *next=*(BYTE **)(node+0x24);
    return next&&!next[0x34];
}
__attribute__((naked,noinline)) static void cjk_split_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\nmov %esp,%edi\n"
        "push 208(%edi)\npush 224(%edi)\npush 20(%ebp)\npush 4(%ebx)\ncall _cjk_prepare_split\nadd $16,%esp\n"
        "test %eax,%eax\njs 1f\nmov %al,20(%ebx)\n"
        "1: fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_split_original\n");
}
__attribute__((used,noinline)) static BOOL __cdecl cjk_owned_glyph(const void *glyph){
    uintptr_t address=(uintptr_t)glyph;
    for(unsigned i=0;i<cjk_font_count;i++){
        BYTE *font=cjk_fonts[i].font;
        uintptr_t begin=(uintptr_t)*(BYTE **)(font+0x28),end=(uintptr_t)*(BYTE **)(font+0x2c);
        if(address>=begin&&address<end&&(address-begin)%44==0)return TRUE;
    }
    return FALSE;
}
/* Keep native fallback texture selection, but use normal chained-glyph pen
 * rules for owned language glyphs. Original fallback artwork keeps its rules. */
__attribute__((naked,noinline)) static void cjk_y_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 24(%ebx)\ncall _cjk_owned_glyph\nadd $4,%esp\ntest %eax,%eax\njnz 1f\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_y_original\n"
        "1: fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\nmov 32(%ecx),%eax\njmp *_cjk_y_after\n");
}
__attribute__((naked,noinline)) static void cjk_pen_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 68(%ebx)\ncall _cjk_owned_glyph\nadd $4,%esp\ntest %eax,%eax\njnz 1f\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_pen_original\n"
        "1: fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\ncmp %eax,%eax\n"
        "jmp *_cjk_pen_after\n");
}

static CjkAtlas *cjk_read_atlas(CjkAtlas *atlas,unsigned tier,BOOL latin){
    atlas->attempted=TRUE;atlas->pixels=tier==0?32:tier==1?64:96;
    WCHAR path[MAX_PATH],suffix[64];wcscpy(path,config_path);WCHAR *slash=wcsrchr(path,L'\\');
    if(latin)_snwprintf(suffix,64,L"\\fonts\\noto-latin\\latin%u.rrcf",(unsigned)atlas->pixels);
    else if(tier)_snwprintf(suffix,64,L"\\fonts\\noto-cjk\\jp%u.rrcf",(unsigned)atlas->pixels);
    else wcscpy(suffix,L"\\fonts\\noto-cjk\\jp.rrcf");
    if(!slash||(size_t)(slash-path)+wcslen(suffix)>=MAX_PATH)return NULL;wcscpy(slash,suffix);
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE)return NULL;
    DWORD header[12]={0},count,size=GetFileSize(file,NULL),header_size=tier?48:32,record_size=tier?44:32;
    BOOL ok=ReadFile(file,header,header_size,&count,NULL)&&count==header_size;
    ok=ok&&header[0]==0x46435252&&header[1]==(tier?2u:1u)&&header[2]>0&&header[2]<=65536
        &&header[3]==8192&&header[4]==8192&&header[5]==atlas->pixels&&header[6]>0&&header[6]<128
        &&size==header_size+header[2]*record_size;
    DWORD pages=tier?header[8]:1;
    ok=ok&&pages>0&&pages<=CJK_MAX_PAGES&&(!tier||(header[9]==44&&header[10]==32&&header[11]==0));
    BYTE *raw=ok?HeapAlloc(GetProcessHeap(),0,header[2]*record_size):NULL;
    ok=raw&&ReadFile(file,raw,header[2]*record_size,&count,NULL)&&count==header[2]*record_size;CloseHandle(file);
    CjkPageMetric *records=ok?HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,header[2]*sizeof(*records)):NULL;
    ok=ok&&records;
    if(ok)for(DWORD i=0;i<header[2];i++){
        CjkPageMetric *m=records+i;
        if(tier)memcpy(m,raw+i*44,44);
        else{CjkMetric *old=(CjkMetric *)(raw+i*32);m->code=old->code;memcpy(&m->x,&old->x,28);m->design_w=m->w;m->design_h=m->h;}
        if(m->code>0x10ffff||(m->code>=0xd800&&m->code<=0xdfff)||(i&&m->code<=records[i-1].code)||m->page>=pages
            ||!(m->x>=0&&m->y>=0&&m->w>=0&&m->h>=0&&m->x+m->w<=8192&&m->y+m->h<=8192)
            ||!(m->left>=-128&&m->left<=128&&m->top>=-128&&m->top<=128&&m->advance>=0&&m->advance<=128)
            ||!(m->design_w>=0&&m->design_w<=128&&m->design_h>=0&&m->design_h<=128)){ok=FALSE;break;}
    }
    if(raw)HeapFree(GetProcessHeap(),0,raw);
    if(!ok){if(records)HeapFree(GetProcessHeap(),0,records);log_line("CJK font metrics invalid; quality tier refused.");return NULL;}
    atlas->metrics=records;atlas->count=header[2];atlas->ascent=header[6];atlas->pages=pages;for(unsigned i=0;i<pages;i++)atlas->latin[i]=latin;return atlas;
}
/* Both independent downloads feed the same native fallback and owned VFS. */
static int cjk_metric_compare(const void *a,const void *b){
    DWORD x=((const CjkPageMetric *)a)->code,y=((const CjkPageMetric *)b)->code;
    return x<y?-1:x>y;
}
static CjkAtlas *cjk_load_atlas(unsigned tier){
    CjkAtlas *atlas=cjk_atlases+tier;if(atlas->attempted)return atlas->metrics?atlas:NULL;
    CjkAtlas latin={0};
    if(cjk_pack_enabled())cjk_read_atlas(atlas,tier,FALSE);
    atlas->attempted=TRUE;
    if(latin_pack_enabled()&&cjk_read_atlas(&latin,tier,TRUE)){
        if(!atlas->metrics){*atlas=latin;return atlas;}
        if(atlas->pages+latin.pages>CJK_MAX_PAGES){HeapFree(GetProcessHeap(),0,latin.metrics);return atlas;}
        CjkPageMetric *merged=HeapAlloc(GetProcessHeap(),0,(atlas->count+latin.count)*sizeof(*merged));
        if(!merged){HeapFree(GetProcessHeap(),0,latin.metrics);return atlas;}
        memcpy(merged,atlas->metrics,atlas->count*sizeof(*merged));unsigned n=atlas->count;
        for(unsigned i=0;i<latin.count;i++){
            CjkPageMetric key=latin.metrics[i];
            if(bsearch(&key,atlas->metrics,atlas->count,sizeof(key),cjk_metric_compare))continue;
            key.page+=atlas->pages;key.top+=((float)atlas->ascent-(float)latin.ascent);merged[n++]=key;
        }
        qsort(merged,n,sizeof(*merged),cjk_metric_compare);
        for(unsigned i=0;i<latin.pages;i++)atlas->latin[atlas->pages+i]=TRUE;
        atlas->pages+=latin.pages;atlas->count=n;
        HeapFree(GetProcessHeap(),0,atlas->metrics);HeapFree(GetProcessHeap(),0,latin.metrics);atlas->metrics=merged;
    }
    return atlas->metrics?atlas:NULL;
}
static unsigned cjk_quality_tier(float cap){return cap>64?2:cap>32?1:0;}
static CjkAtlas *cjk_select_atlas(float cap){
    unsigned tier=cjk_hd_ready&&cjk_hd_capacity()?cjk_quality_tier(cap):0;
    CjkAtlas *atlas=cjk_load_atlas(tier);return atlas?atlas:tier?cjk_load_atlas(0):NULL;
}
static BOOL cjk_page_tokens(CjkAtlas *atlas,unsigned page){
    if(page>=atlas->pages)return FALSE;if(atlas->tokens_ready[page])return TRUE;
    typedef void *(__thiscall *TokenCtor)(void *,const char *);
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);char resource[96],texture[96];
    if(atlas->latin[page]){snprintf(resource,sizeof(resource),"fonts/reriff/latin/p%u",(unsigned)atlas->pixels);snprintf(texture,sizeof(texture),"reriff_latin_%u",(unsigned)atlas->pixels);}
    else if(atlas->pixels==32){strcpy(resource,"fonts/reriff/cjk/jp");strcpy(texture,"reriff_cjk_jp");}
    else{snprintf(resource,sizeof(resource),"fonts/reriff/cjk/jp%u/p%03u",(unsigned)atlas->pixels,page);
        snprintf(texture,sizeof(texture),"reriff_cjk_jp%u_p%03u",(unsigned)atlas->pixels,page);}
    ((TokenCtor)(base+0x18a90))(&atlas->resource[page],resource);
    ((TokenCtor)(base+0x18a90))(&atlas->texture[page],texture);
    atlas->tokens_ready[page]=atlas->resource[page]>=0&&atlas->texture[page]>=0;return atlas->tokens_ready[page];
}
static void cjk_prepare_fallback(void *object,const WCHAR *text){
    if(cjk_render_enabled<0)cjk_render_enabled=cjk_component_enabled();
    if(!cjk_render_enabled)return;
    BYTE *font=object,*base=(BYTE *)GetModuleHandleW(NULL);
    if(!readable(font,0x70)||*(void **)font!=base+0x5fa5ac)return;
    BYTE *previous=*(BYTE **)(font+0x68);BOOL owned=FALSE;
    for(unsigned i=0;i<cjk_font_count;i++)if(cjk_fonts[i].font==previous)owned=TRUE;
    if(previous&&!owned)return;
    BOOL needed=FALSE;unsigned units=0;
    for(;units<8192&&readable(text+units,2)&&text[units];units++)if(text[units]>127)needed=TRUE;
    if(!needed)return;
    BYTE *begin=*(BYTE **)(font+0x28),*end=*(BYTE **)(font+0x2c);
    uintptr_t length=(uintptr_t)end-(uintptr_t)begin;
    if(length>65536*44||length%44||!readable(begin,length))return;
    float texture_height=*(float *)(font+0x14),cap=0,baseline=0;
    for(BYTE *g=begin;g<end;g+=44)if(*(WORD *)g=='M'){
        cap=*(float *)(g+0x18)*texture_height;baseline=*(float *)(g+0x20)+cap;break;
    }
    if(!(cap>=4&&cap<=512&&baseline>=0&&baseline<=1024))return;
    CjkAtlas *atlas=units>=8192?cjk_load_atlas(0):cjk_select_atlas(cap);if(!atlas)return;
    float scale=cap/32.f;
    for(unsigned i=0;i<cjk_font_count;i++)if(cjk_fonts[i].scale==scale&&cjk_fonts[i].baseline==baseline&&cjk_fonts[i].atlas==atlas){
        *(BYTE **)(font+0x68)=cjk_fonts[i].font;return;
    }
    if(cjk_font_count>=CJK_FONT_LIMIT)return;
    static const BYTE ctor_bytes[]={0x55,0x8b,0xec,0x6a,0xff,0x68};
    if(!readable(base+0x551b0,sizeof(ctor_bytes))||memcmp(base+0x551b0,ctor_bytes,sizeof(ctor_bytes)))return;
    typedef void *(__cdecl *GameAllocate)(size_t);
    typedef void *(__thiscall *FontCtor)(void *,const int *,const int *,void *,BYTE,unsigned);
    if(!cjk_page_tokens(atlas,0))return;
    unsigned n=0;for(DWORD i=0;i<atlas->count;i++)if(atlas->metrics[i].code<=65535)n++;
    CjkRawGlyph *raw=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,n*sizeof(*raw));if(!raw)return;
    float canonical_min=*(float *)(base+0x60686c),canonical_max=*(float *)(base+0x6063f8);
    unsigned j=0;for(DWORD i=0;i<atlas->count;i++)if(atlas->metrics[i].code<=65535){
        CjkPageMetric *m=atlas->metrics+i;CjkRawGlyph *g=raw+j++;
        g->code=m->code;g->x=m->x;g->y=m->y;g->w=m->w;g->h=m->h;
        g->left=m->left*scale;g->top=baseline+(m->top-atlas->ascent)*scale;g->advance=m->advance*scale;
        if(g->top<canonical_min)canonical_min=g->top;
        if(g->top+m->design_h>canonical_max)canonical_max=g->top+m->design_h;
    }
    struct {CjkRawGlyph *begin,*end,*capacity;} vector={raw,raw+n,raw+n};
    GameAllocate allocate=*(GameAllocate *)(base+0x52d4b4);BYTE *fallback=allocate(0x70);
    if(!fallback){HeapFree(GetProcessHeap(),0,raw);return;}
    ((FontCtor)(base+0x551b0))(fallback,&atlas->resource[0],&atlas->texture[0],&vector,0,*(unsigned *)(font+0x64));
    HeapFree(GetProcessHeap(),0,raw);
    /* Original 455428..4554DF computes max(top+height)-min(top).
       Preserve the legacy 32px height here as well as in each glyph quad. */
    *(float *)(fallback+0x20)=canonical_max-canonical_min;
    if(*(float *)(fallback+0x10)!=8192||*(float *)(fallback+0x14)!=8192){
        ((void (__thiscall *)(void *))(base+0x55090))(fallback);(*(void (__cdecl **)(void *))(base+0x52d4b0))(fallback);
        log_line("CJK render probe: unexpected texture dimensions; fallback refused.");return;
    }
    BYTE *glyphs=*(BYTE **)(fallback+0x28);j=0;
    for(DWORD i=0;i<atlas->count;i++)if(atlas->metrics[i].code<=65535){
        CjkPageMetric *m=atlas->metrics+i;BYTE *g=glyphs+j++*44;
        *(float *)(g+0x14)=m->design_w*scale/8192.f;*(float *)(g+0x18)=m->design_h*scale/8192.f;
        *(float *)(g+4)+=m->page*2.f; /* retained until the native split; Umax stays untagged */
    }
    cjk_fonts[cjk_font_count]=(CjkFontCache){scale,baseline,fallback,atlas};cjk_font_count++;*(BYTE **)(font+0x68)=fallback;
    char line[200];snprintf(line,sizeof(line),"CJK font: %upx, %u pages, %u BMP glyphs; cap=%.1f baseline=%.1f.",
        (unsigned)atlas->pixels,(unsigned)atlas->pages,n,cap,baseline);log_line(line);
}
