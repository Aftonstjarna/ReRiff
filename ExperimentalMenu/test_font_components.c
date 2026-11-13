#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
/* Each process uses an isolated config and actual packaged metrics. Mask bits:
 * 1 = expected CJK coverage, 2 = expected Latin-only additions. Missing packs
 * must be harmless even when their setting is stale and still enabled. */
int main(int argc,char **argv){
    CHECK(argc==3);CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,config_path,MAX_PATH));
    int expected=atoi(argv[2]);
    for(unsigned tier=0;tier<3;tier++){
        CjkAtlas *atlas=cjk_load_atlas(tier);
        if(!expected){CHECK(!atlas);continue;}
        CHECK(atlas&&atlas->count&&atlas->pages<=CJK_MAX_PAGES);
        CjkPageMetric key={0};key.code=0x6708;
        CHECK(!!bsearch(&key,atlas->metrics,atlas->count,sizeof(key),cjk_metric_compare)==!!(expected&1));
        key.code=0x0142;
        CHECK(!!bsearch(&key,atlas->metrics,atlas->count,sizeof(key),cjk_metric_compare)==!!(expected&2));
        for(unsigned page=0;page<atlas->pages;page++){
            if(expected==1)CHECK(!atlas->latin[page]);
            if(expected==2)CHECK(atlas->latin[page]);
        }
        CHECK(cjk_load_atlas(tier)==atlas);
    }
    puts("PASS: independent/missing language components retain expected coverage and atlas ownership.");return 0;
}
