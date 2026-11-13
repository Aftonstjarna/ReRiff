#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
int main(int argc,char **argv){
    CHECK(argc==2);CHECK(MultiByteToWideChar(CP_UTF8,0,argv[1],-1,config_path,MAX_PATH));
    CHECK(cjk_pack_enabled()&&latin_pack_enabled());
    for(unsigned tier=0;tier<3;tier++){
        CjkAtlas *atlas=cjk_load_atlas(tier);CHECK(atlas&&atlas->count>44810&&atlas->pages<=8);
        const WCHAR polish[]=L"\x0104\x0105\x0106\x0107\x0118\x0119\x0141\x0142\x0143\x0144\x00d3\x00f3\x015a\x015b\x0179\x017a\x017b\x017c";
        for(unsigned i=0;polish[i];i++){CjkPageMetric key={0};key.code=polish[i];CjkPageMetric *m=bsearch(&key,atlas->metrics,atlas->count,sizeof(key),cjk_metric_compare);CHECK(m&&m->w>0&&m->h>0&&m->page<atlas->pages);}
        CjkPageMetric jp={0};jp.code=0x6708;CHECK(bsearch(&jp,atlas->metrics,atlas->count,sizeof(jp),cjk_metric_compare));
        if(tier){CjkAtlas *base=cjk_atlases;CHECK(base->count==atlas->count);for(unsigned i=0;i<atlas->count;i++){CHECK(base->metrics[i].code==atlas->metrics[i].code);CHECK(base->metrics[i].top==atlas->metrics[i].top&&base->metrics[i].advance==atlas->metrics[i].advance);}}
    }
    puts("PASS: native loader combines Polish and Japanese coverage with identical 32/64/96px layout and at most eight pages.");return 0;
}
