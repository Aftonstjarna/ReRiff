/* Verifies that retained props stop being forced once they are broken and
 * that the barricade/ramp release window is widened to the retained behind
 * window. gaPropBase.forceBreak sets byte +493 and hides the prop visual, so
 * forcing that prop visible kept smashed walls on screen; the barricade tick
 * releases its pooled prop at -30, which culled the walls beside ramps. */
#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned visual_calls;
static void __cdecl force_visual(void *visual,BYTE visible){(void)visual;if(visible)visual_calls++;}
int main(void){
    enum{PROPS=2};
    BYTE *base=VirtualAlloc(NULL,0x900000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    BYTE *manager=VirtualAlloc(NULL,484,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(base&&manager);
    BYTE *prop[PROPS],*visual[PROPS];
    void *prop_vector[PROPS];
    for(int i=0;i<PROPS;i++){
        prop[i]=VirtualAlloc(NULL,572,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        visual[i]=VirtualAlloc(NULL,132,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
        CHECK(prop[i]&&visual[i]);
        *((void **)prop[i])=(void *)(size_t)(0x1000+i*256);
        *((void **)(prop[i]+288))=visual[i];
        prop_vector[i]=prop[i];
    }
    /* Prop 0 is intact, prop 1 has been broken. */
    *((BYTE *)prop[1]+493)=1;
    *((void ***)(manager+448))=NULL;
    *((void ***)(manager+452))=NULL;
    *((void ***)(manager+456))=NULL;
    *((void ***)(manager+460))=prop_vector;
    *((void ***)(manager+464))=prop_vector+PROPS;
    *((void ***)(manager+468))=prop_vector+PROPS;
    *((void ***)(manager+472))=NULL;
    *((void ***)(manager+476))=NULL;
    *((void ***)(manager+480))=NULL;
    *((void **)(base+0x7e3080))=manager;
    CHECK(ReRiffConfigureCullingVisible(force_visual));
    race_culling_props(base);
    CHECK(*((BYTE *)visual[0]+20)==1);
    CHECK(*((BYTE *)visual[1]+20)==0);
    CHECK(visual_calls==1);

    /* A broken prop is never added to the snapshot or re-appended. */
    void *vec[3]={prop_vector,prop_vector+1,prop_vector+PROPS};
    void *snapshot[4],*vtable[4];
    unsigned count=0;
    prop_vector[0]=prop[1];
    vec[1]=prop_vector+1;
    CHECK(race_culling_readd(vec,572,snapshot,vtable,&count,493)==0);
    CHECK(count==0);

    /* An intact prop is snapshotted and re-appended after the game drops it. */
    prop_vector[0]=prop[0];
    vec[1]=prop_vector+1;
    CHECK(race_culling_readd(vec,572,snapshot,vtable,&count,493)==0);
    CHECK(count==1&&snapshot[0]==prop[0]);
    vec[1]=prop_vector;
    CHECK(race_culling_readd(vec,572,snapshot,vtable,&count,493)==1);
    CHECK(vec[1]==prop_vector+1&&prop_vector[0]==prop[0]);

    /* Once intact prop 0 breaks it leaves the snapshot and is not re-appended. */
    *((BYTE *)prop[0]+493)=1;
    vec[1]=prop_vector;
    CHECK(race_culling_readd(vec,572,snapshot,vtable,&count,493)==0);
    CHECK(count==0);

    /* The tier window is expressed in strip blocks and converted to world
     * units. A prop behind that window leaves the snapshot and is not forced;
     * a prop ahead of it remains retained. */
    float *pose=(float *)(base+0x7a4998);
    memset(pose,0,48);pose[0]=1;pose[4]=1;pose[8]=1;
    race_retention_tier=10;race_window_behind=100;race_culling_block_distance=1.0f;
    *((BYTE *)prop[0]+493)=0;
    float *translation=(float *)(visual[0]+76);
    translation[0]=0;translation[1]=0;translation[2]=-200;
    prop_vector[0]=prop[0];prop_vector[1]=prop[1];
    *((void ***)(manager+460))=prop_vector;*((void ***)(manager+464))=prop_vector+2;*((void ***)(manager+468))=prop_vector+2;
    *((BYTE *)visual[0]+20)=0;
    race_culling_props(base);
    CHECK(*((BYTE *)visual[0]+20)==0);
    translation[2]=200;
    race_culling_props(base);
    CHECK(*((BYTE *)visual[0]+20)==1);
    race_retention_tier=0;

    /* The -30 release literal becomes the retained behind window. */
    double *barricade=(double *)(base+0x23afd7),*ramp=(double *)(base+0x23c940);
    *barricade=-30.0;*ramp=-30.0;
    CHECK(race_culling_widen_behind(base,512.0));
    CHECK(*barricade==-512.0&&*ramp==-512.0);
    CHECK(race_culling_widen_behind(base,512.0));
    CHECK(*barricade==-512.0&&*ramp==-512.0);
    puts("PASS: broken props clear and the release window follows retention");
    return 0;
}
