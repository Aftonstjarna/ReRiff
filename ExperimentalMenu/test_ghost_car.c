#include "engine_menu.c"
#define CHECK(x) do{if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
static BOOL opened=TRUE,closed,write_ok=TRUE,close_ok=TRUE,invalid_patch;static unsigned opens,closes,reads,published;
static int before_status=1,after_status=1;
static int present(void){return 1;}
static BYTE __thiscall open_fixture(void *self,const char *name,DWORD mode){(void)self;(void)name;(void)mode;opens++;closed=FALSE;return opened;}
static BYTE __thiscall close_fixture(void *self){(void)self;closes++;closed=TRUE;return close_ok;}
static BYTE __thiscall read_fixture(void *self,DWORD at,void *out,DWORD size,DWORD *count){(void)self;(void)at;memset(out,0xa5,size);*count=size;return TRUE;}
static BYTE __thiscall write_fixture(void *self,DWORD at,const void *in,DWORD size){(void)self;(void)at;(void)in;(void)size;return write_ok;}
static int __cdecl process_fixture(const WCHAR *root,const char *name,int phase,int installed,void *patch){
    if(wcscmp(root,L"C:\\fixture\\")||strcmp(name,"abc-123.ghost")||installed!=1)return -1;
    if(phase==0){GhostCarPatch value={40,43,"dod",1};if(invalid_patch)value.pack=9;if(before_status==1)memcpy(patch,&value,sizeof(value));reads++;return before_status;}
    if(!closed)return -1;if(after_status==1)published++;return after_status;
}
int main(void){
    GhostCarPatch patch={40,43,"dod",1};BYTE original[100],expected[100];memset(original,0xa5,100);memcpy(expected,original,100);
    memcpy(expected+40,"dod",3);memcpy(expected+43,&patch.pack,4);
    for(unsigned chunk=1;chunk<=100;chunk++){
        BYTE data[100];memcpy(data,original,100);
        for(unsigned at=0;at<100;at+=chunk){unsigned n=100-at;if(n>chunk)n=chunk;ghost_car_overlay(at,data+at,n,&patch);}
        CHECK(!memcmp(data,expected,100));
    }
    BYTE guard[4]={1,2,3,4};ghost_car_overlay(0xffffffffu,guard,4,&patch);CHECK(guard[0]==1&&guard[3]==4);
    CHECK(ghost_car_filename("abc123-76561198000000000.ghost"));CHECK(!ghost_car_filename("../test.ghost"));CHECK(!ghost_car_filename("save.bin"));CHECK(!ghost_car_filename("x.ghost/r"));
    ghost_car_process=NULL;CHECK(ghost_car_before_open(NULL,"save.bin",1)&&!ghost_car_target&&!ghost_car_patch_ready);
    BYTE device[64]={0};WCHAR root[]=L"C:\\fixture\\";*(WCHAR **)(device+20)=root;*(DWORD *)(device+36)=(DWORD)wcslen(root);*(DWORD *)(device+40)=(DWORD)wcslen(root);
    progression_open=open_fixture;progression_close=close_fixture;progression_read=read_fixture;progression_write=write_fixture;
    ghost_car_process=process_fixture;ghost_car_has_mod=present;
    CHECK(progression_domain_open(device,"abc-123.ghost",1)&&reads==1&&ghost_car_patch_ready);
    BYTE data[100];DWORD count=0;CHECK(progression_domain_read(device,0,data,100,&count)&&count==100&&!memcmp(data,expected,100));
    CHECK(progression_domain_close(device)&&published==0&&!ghost_car_patch_ready);
    CHECK(progression_domain_open(device,"abc-123.ghost",2)&&reads==1);CHECK(progression_domain_write(device,0,data,100));CHECK(progression_domain_close(device)&&published==1);
    CHECK(progression_domain_open(device,"abc-123.ghost",2)&&reads==1);write_ok=FALSE;CHECK(!progression_domain_write(device,0,data,100));CHECK(progression_domain_close(device)&&published==1);
    unsigned old_opens=opens;before_status=-1;CHECK(!progression_domain_open(device,"abc-123.ghost",1)&&opens==old_opens&&!ghost_car_target&&!ghost_car_patch_ready);
    before_status=1;invalid_patch=TRUE;old_opens=opens;CHECK(!progression_domain_open(device,"abc-123.ghost",1)&&opens==old_opens&&!ghost_car_target&&!ghost_car_patch_ready);invalid_patch=FALSE;
    after_status=-1;CHECK(progression_domain_open(device,"abc-123.ghost",2));unsigned old_closes=closes;CHECK(!progression_domain_close(device)&&closed&&closes==old_closes+1);
    after_status=1;close_ok=FALSE;CHECK(progression_domain_open(device,"abc-123.ghost",2));old_closes=closes;CHECK(!progression_domain_close(device)&&closed&&closes==old_closes+1&&published==1);close_ok=TRUE;
    before_status=-1;CHECK(progression_domain_open(device,"save.bin",1));CHECK(progression_domain_close(device));
    before_status=1;
    BYTE no_root[64]={0};car_slots_active=FALSE;CHECK(progression_domain_open(no_root,"abc-123.ghost",1));CHECK(progression_domain_close(no_root));
    car_slots_active=TRUE;old_opens=opens;CHECK(!progression_domain_open(no_root,"abc-123.ghost",1)&&opens==old_opens);car_slots_active=FALSE;
    opened=FALSE;CHECK(!progression_domain_open(device,"abc-123.ghost",1)&&!ghost_car_patch_ready&&!ghost_car_target);
    puts("PASS ghost overlays, normalization/open/close failure propagation, native close preservation, ordinary-file pass-through and write-failure behavior");return 0;
}
