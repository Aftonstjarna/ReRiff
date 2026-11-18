#include "engine_menu.c"
#include <math.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE badge_number[512],badge_star[512],badge_suffix[512],badge_menu[512],badge_ui[512],badge_parent[512],badge_font[112],badge_glyph[44];
static void *badge_children[3],*badge_table[49],*badge_ui_pointer=badge_ui;
static int stars,text_items,cleanup_calls;
static float text_height=30;
static void __thiscall visible_mock(void *item,BYTE visible){((BYTE *)item)[372]=visible;}
static void __thiscall colour_mock(void *item,const float *colour){(void)item;if(colour[0]!=1||colour[3]!=1)abort();}
static void __thiscall scale_mock(void *item,const float *size){((float *)item)[43]=size[0];((float *)item)[44]=size[1];}
static void __thiscall position_mock(void *item,const float *point){((float *)item)[8]=point[0];((float *)item)[9]=point[1];}
static void __thiscall enable_mock(void *item,BYTE enabled){(void)item;if(enabled)abort();}
static unsigned __thiscall font_mock(void *number){if(number!=badge_number)abort();return 9;}
static void __thiscall raw_mock(void *item,const WCHAR *text){
    if(item==badge_number){if(!*text||wcsspn(text,L"0123456789")!=wcslen(text))abort();((float *)item)[12]=20*wcslen(text)*text_height/30;}
    else if(item==badge_suffix){if(*text&&wcscmp(text,L"x2"))abort();((float *)item)[12]=*text?20*text_height/30:0;}
    else abort();((float *)item)[13]=text_height;
    if(item==badge_number)*(WORD *)badge_glyph=(WORD)text[0];
    *(float *)(badge_glyph+0x18)=(20*text_height/30)/1000;*(float *)(badge_glyph+0x20)=5*text_height/30;
}
static void *__thiscall sprite_mock(void *ui,int id,const float *point,int origin,int screen){
    if(ui!=badge_ui||id!=89||origin!=4||screen!=-1||point[0]||point[1])abort();stars++;
    *(void ***)badge_star=badge_table;((float *)badge_star)[12]=((float *)badge_star)[13]=20;return badge_star;
}
static void *__cdecl text_mock(const WCHAR *text,float x,float y,unsigned font,int align,int vertical,float width,BOOL wrap,float height,BOOL menu,BOOL enabled){
    (void)width;(void)height;(void)enabled;if(*text||x||y||font!=9||align||vertical!=1||wrap||menu)abort();
    text_items++;*(void ***)badge_suffix=badge_table;return badge_suffix;
}
static void __thiscall add_mock(void *parent,void *child){
    if(parent!=badge_parent)abort();unsigned index=child==badge_star?1:2;badge_children[index]=child;
    *(void ***)(badge_parent+376)=badge_children;*(void ***)(badge_parent+380)=badge_children+index+1;
}
static void __thiscall cleanup_mock(void *menu,void *screen,void *lower){(void)screen;(void)lower;if(menu!=badge_menu)abort();cleanup_calls++;}
int main(void){
    badge_table[3]=(void *)visible_mock;badge_table[4]=(void *)colour_mock;badge_table[9]=(void *)scale_mock;
    *(void ***)badge_number=badge_table;((float *)badge_number)[8]=100;((float *)badge_number)[9]=80;
    *(void **)(badge_menu+384)=badge_parent;badge_children[0]=badge_number;
    *(void ***)(badge_parent+376)=badge_children;*(void ***)(badge_parent+380)=badge_children+1;
    *(void **)(badge_number+0x1cc)=badge_font;*(float *)(badge_font+0x14)=1000;
    *(void **)(badge_font+0x28)=badge_glyph;*(void **)(badge_font+0x2c)=badge_glyph+44;
    *(void **)(badge_menu+276)=badge_number;progression_font=font_mock;raw_text=raw_mock;create_sprite=sprite_mock;create_text=text_mock;add_item=add_mock;menu_item=enable_mock;enable_input=enable_mock;sprite_position=position_mock;ui_instance=&badge_ui_pointer;
    CHECK(progression_badge_render(badge_parent,badge_number,L"33 *")&&stars==1&&text_items==1);
    CHECK(badge_star[372]&&!badge_suffix[372]&&((float *)badge_number)[8]<100);
    CHECK(fabsf(((float *)badge_star)[43]-1.f)<.001f);
    CHECK(fabsf(((float *)badge_star)[9]-((float *)badge_number)[9])<.01f);
    CHECK(progression_badge_child(badge_parent,badge_star)&&!progression_badge_child(badge_number,badge_star));
    progression_badge_restore(badge_menu);CHECK(((float *)badge_number)[8]==100&&!badge_star[372]);
    CHECK(progression_badge_render(badge_parent,badge_number,L"33 *x2")&&stars==1&&text_items==1&&badge_star[372]&&badge_suffix[372]);
    CHECK(fabsf(((float *)badge_number)[8]-76.8f)<.01f); /* Whole badge remains centred. */
    progression_badge_restore(badge_menu);CHECK(progression_badge_render(badge_parent,badge_number,L"33")&&!badge_star[372]&&!badge_suffix[372]);
    const float heights[]={15,30,45,60,90};
    for(unsigned i=0;i<5;i++){
        progression_badge_restore(badge_menu);text_height=heights[i];
        CHECK(progression_badge_render(badge_parent,badge_number,L"33 *x2"));
        float extra=text_height*(.16f*20/30+20.f/30+.16f*20/30+20.f/30);
        CHECK(fabsf(((float *)badge_number)[8]-(100-extra*.5f))<.01f);
        CHECK(stars==1&&text_items==1);
        CHECK(fabsf(((float *)badge_star)[9]-((float *)badge_number)[9])<.01f);
        CHECK(fabsf(((float *)badge_star)[43]*20-text_height*20/30)<.01f);
    }
    const WCHAR *extended[]={L"41 *",L"100 *x2",L"1000 *x2",L"2147483647 *x2"};
    for(unsigned i=0;i<5;i++)for(unsigned j=0;j<4;j++){
        progression_badge_restore(badge_menu);text_height=heights[i];
        CHECK(progression_badge_render(badge_parent,badge_number,extended[j]));
        float star_left=((float *)badge_star)[8]-((float *)badge_star)[43]*20*.5f;
        float text_right=((float *)badge_number)[8]+((float *)badge_number)[12]*.5f;
        CHECK(star_left>text_right&&stars==1&&text_items==1);
        CHECK(fabsf(((float *)badge_star)[9]-((float *)badge_number)[9])<.01f);
    }
    progression_cleanup_original=cleanup_mock;progression_profile_cleanup(badge_menu,NULL,NULL);CHECK(cleanup_calls==1&&!progression_badge_number);
    puts("PASS: native difficulty sprite89, star-only first prestige, x2 suffix, native digit ink height/baseline and sibling-panel transform, centred layout, reuse, hiding and original cleanup forwarding");return 0;
}
