#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(int argc,char **argv){
    CHECK(argc==2);CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,config_path,MAX_PATH));
    CjkAtlas *base=cjk_load_atlas(0),*medium=cjk_load_atlas(1),*large=cjk_load_atlas(2);
    CHECK(base&&medium&&large&&base->pixels==32&&medium->pixels==64&&large->pixels==96);
    CHECK(base->count==medium->count&&base->count==large->count&&base->count==44810);
    CHECK(medium->pages>1&&large->pages>medium->pages&&large->pages<=CJK_MAX_PAGES);
    for(unsigned tier=1;tier<3;tier++){
        CjkAtlas *atlas=cjk_atlases+tier;unsigned sharper=0;
        CHECK(atlas->ascent==base->ascent);
        for(DWORD i=0;i<base->count;i++){
            CjkPageMetric *b=base->metrics+i,*h=atlas->metrics+i;
            CHECK(b->code==h->code&&h->page<atlas->pages);
            CHECK(b->left==h->left&&b->top==h->top&&b->advance==h->advance);
            CHECK(b->design_w==h->design_w&&b->design_h==h->design_h);
            if(h->w>b->w&&h->h>b->h)sharper++;
        }
        CHECK(sharper>40000);
    }
    cjk_hd_ready=FALSE;CHECK(cjk_select_atlas(90)==base);
    cjk_hd_ready=TRUE;
    CHECK(cjk_select_atlas(26)==base&&cjk_select_atlas(32)==base);
    CHECK(cjk_select_atlas(33)==medium&&cjk_select_atlas(64)==medium);
    CHECK(cjk_select_atlas(65)==large&&cjk_select_atlas(180)==large);
    CjkPageMetric *saved=large->metrics;large->metrics=NULL;
    CHECK(cjk_select_atlas(90)==base);large->metrics=saved;
    cjk_pair_count=CJK_PAIR_LIMIT-CJK_MAX_PAGES;CHECK(cjk_select_atlas(90)==base);
    puts("PASS: full 32/64/96 coverage, exact canonical layout metrics, higher-detail rasters, quality thresholds, missing-tier and native-visual-budget fallback");return 0;
}
