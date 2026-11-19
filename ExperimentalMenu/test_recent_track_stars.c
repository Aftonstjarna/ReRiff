#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE row[404],items[5][404];static void *children[9],*table[10],*ui;
static int made,added,disabled,definitions[5];static float positions[5][2];
static void __thiscall size_mock(void *item,const float *f){((float *)item)[12]*=f[0];((float *)item)[13]*=f[1];}
static void *__thiscall sprite_mock(void *instance,int definition,const float *pos,int origin,int id){
    if(instance!=ui||origin!=4||id!=-1||made>=5)abort();void *item=items[made];memset(item,0,404);*(void ***)item=table;((float *)item)[12]=((float *)item)[13]=64;definitions[made]=definition;memcpy(positions[made],pos,8);made++;return item;
}
static void __thiscall disable_mock(void *item,BYTE enabled){if(!item||enabled)abort();disabled++;}
static void __thiscall add_mock(void *parent,void *item){if(parent!=row||((int *)item)[1]!=1)abort();children[4+added++]=item;*(void ***)(row+380)=children+4+added;}
int main(void){
    ui=row;ui_instance=&ui;create_sprite=sprite_mock;add_item=add_mock;enable_input=menu_item=disable_mock;table[9]=(void *)size_mock;
    for(int scale=1;scale<=6;scale++)for(int rating=-1;rating<=6;rating++){
        memset(row,0,404);*(void ***)(row+376)=children;*(void ***)(row+380)=children+4;*(void ***)(row+384)=children+9;
        float factor=scale*0.5f;((float *)row)[12]=1400*factor;((float *)row)[13]=80*factor;made=added=disabled=0;
        CHECK(recent_stars_row(row,rating));CHECK(made==5&&added==5&&disabled==10);
        for(int i=0;i<5;i++){
            CHECK(definitions[i]==(rating>=1&&rating<=5&&i<rating?89:90));
            CHECK(positions[i][0]-16*factor>1100*factor&&positions[i][0]+16*factor<1400*factor);
            CHECK(positions[i][1]-16*factor>=0&&positions[i][1]+16*factor<=80*factor);
        }
        CHECK(!recent_stars_row(row,rating));CHECK(made==5); /* no duplicate decoration */
    }
    BYTE a[24]={0},b[24]={0};memcpy(a,"song",4);memcpy(b,"song",4);*(unsigned *)(a+16)=*(unsigned *)(b+16)=4;*(unsigned *)(a+20)=*(unsigned *)(b+20)=15;
    CHECK(recent_stars_equal(a,b));b[0]='x';CHECK(!recent_stars_equal(a,b));*(unsigned *)(a+16)=4097;CHECK(!recent_stars_equal(a,b));
    puts("PASS: native star definitions, unknown ratings, layout at six scales, input exclusion, SSP ownership and duplicate prevention");return 0;
}
