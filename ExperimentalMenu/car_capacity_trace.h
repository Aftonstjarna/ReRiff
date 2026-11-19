/* Read-only car capacity and save-state observations. No native calls or writes.
 * Verified layouts: reports/CAR-CAPACITY-AND-SAVE-TRACE.md. */
#include <stdarg.h>
static LONG car_trace_started;
static BOOL car_trace_append(char *out,size_t size,size_t *used,const char *format,...){
    if(*used>=size)return FALSE;va_list args;va_start(args,format);int n=vsnprintf(out+*used,size-*used,format,args);va_end(args);
    if(n<0||(size_t)n>=size-*used)return FALSE;*used+=(size_t)n;return TRUE;
}
static BOOL car_trace_capacity(const void *manager,char *out,size_t size){
    BYTE copy[796];if(!progression_probe_read(manager,copy,sizeof(copy)))return FALSE;
    int count;memcpy(&count,copy+792,4);size_t used=0;
    if(!car_trace_append(out,size,&used,"Car capacity: configured=%d fixed_slots=22 unlock_packs_per_car=9",count))return FALSE;
    if(count<0||count>22)return car_trace_append(out,size,&used," UNSAFE_COUNT; indexed reads skipped");
    for(int i=0;i<22;i++){
        DWORD pointers[3],length,capacity;memcpy(pointers,copy+i*12,12);memcpy(&length,copy+264+i*24+16,4);memcpy(&capacity,copy+264+i*24+20,4);
        BOOL valid=pointers[1]>=pointers[0]&&pointers[2]>=pointers[1]&&(pointers[1]-pointers[0])%176==0&&(pointers[2]-pointers[0])%176==0;
        unsigned packs=valid?(pointers[1]-pointers[0])/176:0,allocated=valid?(pointers[2]-pointers[0])/176:0;
        valid=valid&&allocated<=4096&&(pointers[0]||(!pointers[1]&&!pointers[2]));
        char name[65]="<unreadable>";
        if(length<=64&&capacity>=length&&(capacity>=16||length<=15)){
            const void *text=copy+264+i*24;DWORD external;if(capacity>=16){memcpy(&external,text,4);text=(const void *)(uintptr_t)external;}
            if(!length)name[0]=0;else if(progression_probe_read(text,name,length)){name[length]=0;for(unsigned j=0;j<length;j++)if((unsigned char)name[j]<32||(unsigned char)name[j]>126||name[j]=='"'||name[j]=='\\')name[j]='?';}
        }
        if(!car_trace_append(out,size,&used," slot[%d]={active:%u,name:\"%s\",vector_valid:%u,packs:%u,allocated:%u%s}",i,i<count,name,valid,packs,allocated,valid&&packs>9?",UNSAVABLE_PACKS":""))return FALSE;
    }
    return TRUE;
}
static BOOL car_trace_save(const void *save,char *out,size_t size){
    BYTE settings[328],progression[296],flags[4];void *settings_ptr=NULL,*progression_ptr=NULL;
    if(!progression_probe_read((const BYTE *)save+16,&settings_ptr,4)||!progression_probe_read((const BYTE *)save+32,&progression_ptr,4)
        ||!progression_probe_read((const BYTE *)save+12,flags,4)||!progression_probe_read(settings_ptr,settings,sizeof(settings))
        ||!progression_probe_read(progression_ptr,progression,sizeof(progression)))return FALSE;
    int selected;memcpy(&selected,settings+324,4);size_t used=0;
    if(!car_trace_append(out,size,&used,"Car save state: selected=%d valid_index=%u settings_dirty=%u settings_enabled=%u progression_dirty=%u progression_enabled=%u save_skip=%u slots=22",
        selected,selected>=0&&selected<22,settings[4],settings[5],progression[4],progression[5],flags[1]))return FALSE;
    for(int i=0;i<22;i++){
        int paint,driven;unsigned short unlocks;memcpy(&paint,settings+236+4*i,4);memcpy(&driven,progression+188+4*i,4);memcpy(&unlocks,progression+56+2*i,2);
        if(!car_trace_append(out,size,&used," slot[%d]={paint:%d,paint_in_save_range:%u,unlocked:%u,pack_mask:%03x,driven:%d}",i,paint,paint>=0&&paint<9,(progression[53+i/8]>>(i%8))&1,unlocks&0x1ff,driven))return FALSE;
    }
    return TRUE;
}
static DWORD WINAPI car_trace_worker(LPVOID argument){
    BYTE *base=argument;char previous_capacity[8192]="",previous_save[8192]="",current[8192];BOOL names=FALSE;
    for(unsigned tick=0;tick<3600;tick++){
        if(!rr_log_ini(config_path,L"engine_menu"))break;
        BOOL capacity=GetPrivateProfileIntW(L"diagnostics",L"car_capacity_trace",0,config_path)!=0;
        BOOL saves=GetPrivateProfileIntW(L"diagnostics",L"car_save_trace",0,config_path)!=0;
        if(!capacity&&!saves)break;
        void *manager=NULL,*save=NULL;
        if(capacity&&progression_probe_read(base+0x7f3d2c,&manager,4)&&car_trace_capacity(manager,current,sizeof(current))&&strcmp(current,previous_capacity)){
            log_line(current);strcpy(previous_capacity,current);
        }
        if(saves&&progression_probe_read(base+0x7ed7b8,&save,4)&&save&&car_trace_save(save,current,sizeof(current))&&strcmp(current,previous_save)){
            log_line(current);strcpy(previous_save,current);
            if(!names){progression_probe_save_names(base);names=TRUE;}
        }
        Sleep(500);
    }
    log_line("Car trace finished: read-only sampled memory; dirty flags do not prove disk persistence.");return 0;
}
static void start_car_trace(BYTE *base){
    if(!rr_log_ini(config_path,L"engine_menu")||(!GetPrivateProfileIntW(L"diagnostics",L"car_capacity_trace",0,config_path)&&!GetPrivateProfileIntW(L"diagnostics",L"car_save_trace",0,config_path))
        ||InterlockedCompareExchange(&car_trace_started,1,0))return;
    log_line("Car trace started: 500 ms samples for up to 30 minutes; no car, paint, unlock or save writes.");
    HANDLE thread=CreateThread(NULL,0,car_trace_worker,base,0,NULL);if(thread)CloseHandle(thread);else log_line("Car trace could not start.");
}
