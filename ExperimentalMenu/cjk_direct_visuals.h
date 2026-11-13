/* Opt-in extension for original generateVisual callers which pass NULL for
 * the alternate atlas. Uses native pooled visuals and viewport ownership.
 * This is a ReRiff behavior change, not a matching reconstruction. */
typedef struct {BYTE *primary,*alternate;} CjkVisualPair;
#define CJK_PAIR_LIMIT 256
static CjkVisualPair cjk_pairs[CJK_PAIR_LIMIT];
static unsigned cjk_pair_count;
static BOOL cjk_direct_enabled;
static BYTE *cjk_direct_base;
static void *cjk_add_original,*cjk_remove_original,*cjk_destroy_original;
static void *cjk_dirty_original,*cjk_visible_original,*cjk_direct_colour_original;
static void *(__thiscall *cjk_visual_allocate)(void *,unsigned,unsigned);
static void *(__thiscall *cjk_visual_construct)(void *,int);
static void (__thiscall *cjk_visual_child)(void *,void *);
static void *(__thiscall *cjk_visual_construct_texture)(void *,const int *,const int *,int);
static void *cjk_pages_finish_original;
static void (__thiscall *cjk_page_resource)(void *,const int *);
static void (__thiscall *cjk_page_texture)(void *,const int *);
static void cjk_pair_add_tree(BYTE *,BYTE *,unsigned);
static void cjk_pair_remove_tree(BYTE *,BYTE *,unsigned);
static BOOL cjk_hd_capacity(void){return cjk_pair_count<CJK_PAIR_LIMIT-CJK_MAX_PAGES;}

static CjkVisualPair *cjk_pair(BYTE *primary){
    if(!cjk_direct_enabled||!cjk_pair_count)return NULL;
    for(unsigned i=0;i<CJK_PAIR_LIMIT;i++)if(cjk_pairs[i].primary==primary)return cjk_pairs+i;
    return NULL;
}
static void cjk_pair_sync(CjkVisualPair *pair){
    BYTE *p=pair->primary,*a=pair->alternate;
    /* Copy only verified standalone transform/flags. Keep atlas size, UVs,
       resource tokens, child list, cached geometry and native refs separate. */
    *(DWORD *)(a+8)=*(DWORD *)(p+8);
    memcpy(a+0x28,p+0x28,0x50); /* matrix, position and scale; ends before size */
    *(DWORD *)(a+0xc4)=*(DWORD *)(p+0xc4); /* original rotation */
    a[0xd4]=p[0xd4];memcpy(a+0xd8,p+0xd8,8); /* original pivot */
    a[0x14]=p[0x14];
    ((void (__thiscall *)(void *,const float *))cjk_direct_colour_original)(a,(float *)(p+0x90));
    *(DWORD *)(a+0x170)|=0x1b; /* original position/rotation/scale/pivot flags */
    ((void (__cdecl *)(void *,BOOL))cjk_visible_original)(a,p[0x14]!=0);
    ((void (__cdecl *)(void *))cjk_dirty_original)(a);
}
static void cjk_direct_refresh(void){
    if(!cjk_direct_enabled||!cjk_pair_count)return;
    /* Menu ticks also contain inlined managed position/colour assignments
       which cannot pass through the native setter detours. */
    DWORD error=GetLastError();
    for(unsigned i=0;i<CJK_PAIR_LIMIT;i++)if(cjk_pairs[i].primary)cjk_pair_sync(cjk_pairs+i);
    SetLastError(error);
}
static BOOL cjk_view_contains(BYTE *viewport,BYTE *visual){
    if(!readable(viewport,0x1bc))return FALSE;
    uintptr_t start=*(uintptr_t *)(viewport+0x1b0),end=*(uintptr_t *)(viewport+0x1b4);
    if(end<start||end-start>65536||(end-start)%4||!readable((void *)start,end-start))return FALSE;
    for(uintptr_t slot=start;slot<end;slot+=4)if(*(BYTE **)slot==visual)return TRUE;
    return FALSE;
}
static void cjk_pair_existing_viewports(CjkVisualPair *pair,BOOL add){
    /* gaGame owns twelve VIEW slots at +20 and its lower viewport at +68
       (decimal offsets). Read the current owners rather than caching weak
       viewport pointers across native viewport destruction/reset. */
    if(!readable(cjk_direct_base+0x7e9d24,4))return;
    BYTE *game=*(BYTE **)(cjk_direct_base+0x7e9d24);
    if(!readable(game,72))return;
    for(unsigned i=0;i<13;i++){
        BYTE *view=*(BYTE **)(game+20+i*4);
        if(!cjk_view_contains(view,add?pair->primary:pair->alternate))continue;
        if(add){cjk_pair_add_tree(view,pair->primary,0);}
        else{cjk_pair_remove_tree(view,pair->primary,0);}
    }
}
static void cjk_direct_layout(void *font,void **frame){
    if(!cjk_direct_enabled||!readable(frame,5*4))return;
    BYTE *primary=frame[3];CjkVisualPair *pair=cjk_pair(primary);
    if(!readable(primary,0x17c)||*(BYTE **)primary!=cjk_direct_base+0x599efc
        ||*(BYTE **)(primary+0x174)!=cjk_direct_base+0x599ef8)return;
    if(frame[4]){
        if(pair){cjk_pair_existing_viewports(pair,FALSE);cjk_visual_child(pair->alternate,NULL);}
        return; /* Never replace an alternate already supplied by the game. */
    }
    if(!pair){
        if(!readable(font,0x70)||!cjk_font_count||cjk_pair_count==CJK_PAIR_LIMIT)return;
        BYTE *fallback=*(BYTE **)((BYTE *)font+0x68);BOOL owned=FALSE;
        for(unsigned i=0;i<cjk_font_count;i++)if(cjk_fonts[i].font==fallback)owned=TRUE;
        if(!owned||*(BYTE **)(primary+0xc))return; /* standalone visuals only */
        /* Only allocate after an actual non-ASCII string needs this path. */
        const WCHAR *text=frame[2];BOOL needed=FALSE;
        for(unsigned i=0;i<256&&readable(text+i,2)&&text[i];i++)if(text[i]>127)needed=TRUE;
        if(!needed)return;
        BYTE *alternate=cjk_visual_allocate(cjk_direct_base+0x7bcf74,380,380);
        if(!alternate)return;
        cjk_visual_construct(alternate,1);
        ++*(DWORD *)(alternate+0x178); /* one native SSP reference owned here */
        for(unsigned i=0;i<CJK_PAIR_LIMIT;i++)if(!cjk_pairs[i].primary){
            pair=cjk_pairs+i;pair->primary=primary;pair->alternate=alternate;cjk_pair_count++;break;
        }
        log_line("CJK direct layout: created native alternate visual.");
    }
    cjk_pair_sync(pair);
    frame[4]=pair->alternate; /* original raw pointer argument, not an SSP */
    cjk_pair_existing_viewports(pair,TRUE);
}
static void cjk_pair_add_tree(BYTE *view,BYTE *primary,unsigned depth){
    CjkVisualPair *pair=cjk_pair(primary);if(!pair||depth>CJK_MAX_PAGES)return;
    cjk_pair_sync(pair);cjk_pair_add_tree(view,pair->alternate,depth+1);
    ((void (__thiscall *)(void *,void *))cjk_add_original)(view,pair->alternate);
}
static void cjk_pair_remove_tree(BYTE *view,BYTE *primary,unsigned depth){
    CjkVisualPair *pair=cjk_pair(primary);if(!pair||depth>CJK_MAX_PAGES)return;
    cjk_pair_remove_tree(view,pair->alternate,depth+1);
    ((void (__thiscall *)(void *,void *))cjk_remove_original)(view,pair->alternate);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_add(BYTE *view,BYTE *primary){
    DWORD error=GetLastError();cjk_pair_add_tree(view,primary,0);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_remove(BYTE *view,BYTE *primary){
    DWORD error=GetLastError();cjk_pair_remove_tree(view,primary,0);SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_destroy(BYTE *ref){
    DWORD error=GetLastError();CjkVisualPair *pair=cjk_pair(ref-0x174);
    if(pair){
        BYTE *alternate=pair->alternate;
        pair->primary=NULL;pair->alternate=NULL;cjk_pair_count--;
        /* Do not change viewport vectors while their native destructors may
           be iterating them. Their SSPs independently keep the visual alive. */
        if(!--*(DWORD *)(alternate+0x178))
            {cjk_direct_destroy(alternate+0x174);((void *(__thiscall *)(void *,unsigned))cjk_destroy_original)(alternate+0x174,1);}
    }
    SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_dirty(BYTE *primary){
    DWORD error=GetLastError();CjkVisualPair *pair=cjk_pair(primary);if(pair){cjk_pair_sync(pair);cjk_direct_dirty(pair->alternate);}SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_visible(BYTE *primary,BOOL visible){
    DWORD error=GetLastError();CjkVisualPair *pair=cjk_pair(primary);
    if(pair){pair->alternate[0x14]=visible!=0;((void (__cdecl *)(void *,BOOL))cjk_visible_original)(pair->alternate,visible);cjk_direct_visible(pair->alternate,visible);}
    SetLastError(error);
}
__attribute__((used,noinline)) static void __cdecl cjk_direct_colour(BYTE *primary,const float *colour){
    DWORD error=GetLastError();CjkVisualPair *pair=cjk_pair(primary);
    if(pair){((void (__thiscall *)(void *,const float *))cjk_direct_colour_original)(pair->alternate,colour);cjk_direct_colour(pair->alternate,colour);}
    SetLastError(error);
}
/* Preserve the original call's flags, registers and x87/SSE state. All hooks
   run before their original method and tail-forward its unchanged arguments. */
#define CJK_PAIR_ENTRY(name,args,helper,cleanup,original) \
__attribute__((naked,noinline)) static void name(void){ \
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n" \
        args "call _" #helper "\nadd $" #cleanup ",%esp\n" \
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_" #original "\n"); \
}
CJK_PAIR_ENTRY(cjk_add_entry,"push 40(%ebx)\npush 24(%ebx)\n",cjk_direct_add,8,cjk_add_original)
CJK_PAIR_ENTRY(cjk_remove_entry,"push 40(%ebx)\npush 24(%ebx)\n",cjk_direct_remove,8,cjk_remove_original)
CJK_PAIR_ENTRY(cjk_destroy_entry,"push 24(%ebx)\n",cjk_direct_destroy,4,cjk_destroy_original)
CJK_PAIR_ENTRY(cjk_dirty_entry,"push 40(%ebx)\n",cjk_direct_dirty,4,cjk_dirty_original)
CJK_PAIR_ENTRY(cjk_visible_entry,"push 44(%ebx)\npush 40(%ebx)\n",cjk_direct_visible,8,cjk_visible_original)
CJK_PAIR_ENTRY(cjk_direct_colour_entry,"push 40(%ebx)\npush 24(%ebx)\n",cjk_direct_colour,8,cjk_direct_colour_original)
#undef CJK_PAIR_ENTRY

/* The original generator extracts alternate glyphs into one native list.
   After its final vertical alignment, divide only owned HD lists into native
   visuals. Each node is already absolute; retain its position and native SSP
   ownership while changing list membership and clearing the page tag. */
static BYTE *cjk_page_visual(BYTE *parent,CjkAtlas *atlas,unsigned page){
    if(!cjk_page_tokens(atlas,page))return NULL;
    CjkVisualPair *pair=cjk_pair(parent);
    if(!pair){
        if(cjk_pair_count>=CJK_PAIR_LIMIT)return NULL;
        BYTE *visual=cjk_visual_allocate(cjk_direct_base+0x7bcf74,380,380);if(!visual)return NULL;
        cjk_visual_construct_texture(visual,&atlas->resource[page],&atlas->texture[page],1);
        ++*(DWORD *)(visual+0x178);
        for(unsigned i=0;i<CJK_PAIR_LIMIT;i++)if(!cjk_pairs[i].primary){
            pair=cjk_pairs+i;pair->primary=parent;pair->alternate=visual;cjk_pair_count++;break;
        }
    }else{
        cjk_page_resource(pair->alternate,&atlas->resource[page]);
        cjk_page_texture(pair->alternate,&atlas->texture[page]);
    }
    cjk_pair_sync(pair);return pair->alternate;
}
static void cjk_glyph_release(BYTE *node){
    if(!--*(DWORD *)(node+0x3c))((void *(__thiscall *)(void *,unsigned))**(void ***)(node+0x38))(node+0x38,1);
}
static BOOL cjk_route_pages(BYTE *alternate){
    CjkFontCache *cache=cjk_visual_font(alternate);
    if(!cache||!cache->atlas)return TRUE;
    CjkAtlas *atlas=cache->atlas;BYTE *visuals[CJK_MAX_PAGES]={alternate};
    BYTE *node=*(BYTE **)(alternate+0xbc);unsigned count=0,max_page=0,page_mask=0;
    for(BYTE *n=node;n;n=*(BYTE **)(n+0x24)){
        if(++count>8192||!readable(n,64))return FALSE;
        float u=*(float *)(n+4);if(!(u>=0&&u<atlas->pages*2.f)||!n[0x34])return FALSE;
        unsigned page=(unsigned)(u/2.f);page_mask|=1u<<page;if(page>max_page)max_page=page;
    }
    /* Old extra pages must be cleared even after an ASCII-only redraw or
       a switch back to the 32px atlas. Keep their owners for later reuse. */
    BYTE *parent=alternate;
    for(unsigned p=1;p<CJK_MAX_PAGES;p++){
        CjkVisualPair *pair=cjk_pair(parent);if(!pair)break;
        cjk_visual_child(pair->alternate,NULL);parent=pair->alternate;
    }
    if(!max_page)return TRUE;
    parent=alternate;
    for(unsigned p=1;p<=max_page;p++){
        if(!(page_mask&(1u<<p)))continue;
        visuals[p]=cjk_page_visual(parent,atlas,p);if(!visuals[p])return FALSE;parent=visuals[p];
        /* All pages share dimensions. Copy the original font visual's blank
           base quad and text mode, never another font's atlas UVs or size. */
        memcpy(visuals[p]+0x78,alternate+0x78,24);
        *(DWORD *)(visuals[p]+0x158)=*(DWORD *)(alternate+0x158);
        *(DWORD *)(visuals[p]+0x170)|=0xe0;
    }
    BYTE **nodes=HeapAlloc(GetProcessHeap(),0,count*sizeof(*nodes));if(!nodes)return FALSE;
    unsigned used=0;
    for(BYTE *n=node;n;n=*(BYTE **)(n+0x24)){nodes[used++]=n;++*(DWORD *)(n+0x3c);}
    cjk_visual_child(alternate,NULL);
    /* Every node has a temporary reference before any old chain is cut. */
    for(unsigned i=0;i<count;i++){
        BYTE *next=*(BYTE **)(nodes[i]+0x24);*(BYTE **)(nodes[i]+0x24)=NULL;
        if(next)cjk_glyph_release(next);
    }
    BYTE *heads[CJK_MAX_PAGES]={0},*tails[CJK_MAX_PAGES]={0};
    for(unsigned i=0;i<count;i++){
        BYTE *n=nodes[i];unsigned p=(unsigned)(*(float *)(n+4)/2.f);
        *(float *)(n+4)-=p*2.f; /* Native extraction already removed its +2. */
        if(tails[p]){*(BYTE **)(tails[p]+0x24)=n;++*(DWORD *)(n+0x3c);}else heads[p]=n;
        tails[p]=n;
    }
    for(unsigned p=0;p<=max_page;p++){
        if(!visuals[p])continue;
        cjk_visual_child(visuals[p],heads[p]);((void (__cdecl *)(void *))cjk_dirty_original)(visuals[p]);
    }
    for(unsigned i=0;i<count;i++)cjk_glyph_release(nodes[i]);HeapFree(GetProcessHeap(),0,nodes);
    CjkVisualPair *pair=cjk_pair(alternate);if(pair)cjk_pair_existing_viewports(pair,TRUE);
    return TRUE;
}
__attribute__((used,noinline)) static void __cdecl cjk_pages_finish(BYTE *alternate){
    if(!cjk_hd_ready||!alternate)return;
    DWORD error=GetLastError();if(!cjk_route_pages(alternate))log_line("CJK HD page routing refused incomplete glyph state.");SetLastError(error);
}
__attribute__((naked,noinline)) static void cjk_pages_finish_entry(void){
    __asm__("pushfl\npushal\nmov %esp,%ebx\nsub $528,%esp\nand $-16,%esp\nfxsave (%esp)\n"
        "push 20(%ebp)\ncall _cjk_pages_finish\nadd $4,%esp\n"
        "fxrstor (%esp)\nmov %ebx,%esp\npopal\npopfl\njmp *_cjk_pages_finish_original\n");
}
