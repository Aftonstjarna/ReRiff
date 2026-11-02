#include "engine_menu.c"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 1;}}while(0)
static BYTE manager[64],dialog[512],owner[512];
static void *manager_pointer=manager;
static int pushed,popped,completed,choice,original_inputs;
static void __cdecl push_mock(void *self,int state){if(self!=manager||state!=9)abort();pushed++;((void **)manager)[11]=dialog;}
static void __cdecl pop_mock(void *self,int resume){if(self!=manager||resume!=TRUE)abort();popped++;((void **)manager)[11]=NULL;}
static void result_mock(void *self,int result){if(self!=owner)abort();completed++;choice=result;}
static void __thiscall original_mock(void *self,int selection){if(self!=dialog||selection!=2)abort();original_inputs++;}
int main(void){
    overlay_manager=&manager_pointer;overlay_push=push_mock;overlay_pop=pop_mock;prompt_installed=TRUE;prompt_input_original=original_mock;
    CHECK(save_prompt_begin(owner,result_mock)&&pushed==1);
    CHECK(!save_prompt_begin(owner,result_mock));
    save_prompt_tick(owner);CHECK(!completed);
    save_prompt_input(dialog,2);CHECK(popped==1&&!completed&&!original_inputs);
    save_prompt_tick(owner);CHECK(completed==1&&choice==1&&!save_prompt_owner);
    CHECK(save_prompt_begin(owner,result_mock));save_prompt_input(dialog,1);save_prompt_tick(owner);CHECK(completed==2&&choice==0);
    CHECK(save_prompt_begin(owner,result_mock));pop_mock(manager,TRUE);save_prompt_tick(owner);CHECK(completed==3&&choice==-1);
    save_prompt_input(dialog,2);CHECK(original_inputs==1);
    ((int *)owner)[75]=graphics_saved_resolution=2;*((BYTE *)owner+305)=FALSE;
    aa_pending=graphics_saved_aa=FALSE;borderless_pending=graphics_saved_borderless=FALSE;graphics_saved_full=FALSE;
    ui_slider_pending=graphics_saved_scale=0;CHECK(!graphics_dirty(owner));
    ((int *)owner)[75]=3;CHECK(graphics_dirty(owner));((int *)owner)[75]=2;
    aa_pending=TRUE;CHECK(graphics_dirty(owner));aa_pending=FALSE;
    borderless_pending=TRUE;CHECK(graphics_dirty(owner));borderless_pending=FALSE;
    *((BYTE *)owner+305)=TRUE;CHECK(graphics_dirty(owner));*((BYTE *)owner+305)=FALSE;
    ui_slider_pending=25;CHECK(graphics_dirty(owner));ui_slider_pending=0;CHECK(!graphics_dirty(owner));
    prompt_installed=FALSE;CHECK(!save_prompt_begin(owner,result_mock));
    puts("PASS: native save/discard/back results, deferred callbacks, no quit forwarding, duplicate prevention, graphics dirty tracking and unavailable-helper guard");return 0;
}
