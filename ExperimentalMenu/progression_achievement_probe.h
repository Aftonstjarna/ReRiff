/* Read-only integration probe, not a First Prestige UI implementation.
 * Original setupAchievementsList 060013EA / RVA325BBC owns rows at +472.
 * Original elUI.createSprite RVA22030 indexes 100-byte descriptors at UI+12,
 * with an integer count at UI+16. No native objects or flags are modified. */
static void *achievement_probe_owner;
static unsigned achievement_probe_sections;
static BOOL achievement_probe_vector(const BYTE *owner,unsigned offset,void ***items,unsigned *count,unsigned maximum){
    uintptr_t vector[3];
    if(!owner||!progression_probe_read(owner+offset,vector,sizeof(vector)))return FALSE;
    if(!vector[0]||vector[1]<vector[0]||vector[2]<vector[1]||(vector[1]-vector[0])%sizeof(void *))return FALSE;
    uintptr_t length=(vector[1]-vector[0])/sizeof(void *);
    if(length>maximum||!readable((void *)vector[0],length*sizeof(void *)))return FALSE;
    *items=(void **)vector[0];*count=(unsigned)length;return TRUE;
}
static BOOL achievement_probe_snapshot(void *menu,void *ui,char *text,size_t capacity){
    void **rows;unsigned count;
    if(!text||capacity<256||!achievement_probe_vector(menu,472,&rows,&count,39)||(count!=38&&count!=39))return FALSE;
    void *definitions;unsigned sprites;
    if(!ui||!progression_probe_read((BYTE *)ui+12,&definitions,4)||!progression_probe_read((BYTE *)ui+16,&sprites,4)
        ||!definitions||sprites<188||sprites>4096)return FALSE;
    int used=snprintf(text,capacity,"First Prestige native-list probe: original_rows=%u sprite_count=%u",count,sprites);
    if(used<0||(size_t)used>=capacity)return FALSE;
    const unsigned samples[]={17,18,37};
    for(unsigned sample=0;sample<3;sample++){
        void *row;void **children;unsigned child_count;float rectangle[6];
        if(!progression_probe_read(rows+samples[sample],&row,4)
            ||!achievement_probe_vector(row,376,&children,&child_count,16)
            ||!progression_probe_read((BYTE *)row+32,rectangle,sizeof(rectangle)))return FALSE;
        int size=snprintf(text+used,capacity-(size_t)used," row[%u]={children:%u,x:%.2f,y:%.2f,w:%.2f,h:%.2f}",
            samples[sample],child_count,rectangle[0],rectangle[1],rectangle[4],rectangle[5]);
        if(size<0||(size_t)size>=capacity-(size_t)used)return FALSE;used+=size;
    }
    const unsigned ids[]={147,148,149,187};
    for(unsigned i=0;i<4;i++){
        BYTE definition[100];int resource,texture;float width,height;
        if(!progression_probe_read((BYTE *)definitions+ids[i]*100,definition,sizeof(definition)))return FALSE;
        memcpy(&resource,definition,4);memcpy(&texture,definition+4,4);
        memcpy(&width,definition+8,4);memcpy(&height,definition+12,4);
        int size=snprintf(text+used,capacity-(size_t)used," sprite[%u]={resource:%d,texture:%d,w:%.2f,h:%.2f,descriptor:",ids[i],resource,texture,width,height);
        if(size<0||(size_t)size>=capacity-(size_t)used)return FALSE;used+=size;
        for(unsigned j=0;j<100;j++){
            size=snprintf(text+used,capacity-(size_t)used,"%02x",definition[j]);
            if(size<0||(size_t)size>=capacity-(size_t)used)return FALSE;used+=size;
        }
        if((size_t)used+2>=capacity)return FALSE;text[used++]='}';text[used]=0;
    }
    return TRUE;
}
static void achievement_probe_tick(void *menu){
    if(menu==achievement_probe_owner||!rr_log_ini(config_path,L"engine_menu")
        ||!GetPrivateProfileIntW(L"progression",L"trace_enabled",0,config_path)
        ||!readable(progression_friend_length,4)||*progression_friend_length||!readable(ui_instance,4))return;
    char text[2048];
    if(achievement_probe_snapshot(menu,*ui_instance,text,sizeof(text))){achievement_probe_owner=menu;log_line(text);}
    else{
        unsigned section;uintptr_t vector[3];
        if(progression_probe_read((BYTE *)menu+404,&section,4)&&section<4&&!(achievement_probe_sections&(1u<<section))
            &&progression_probe_read((BYTE *)menu+472,vector,sizeof(vector))){
            achievement_probe_sections|=1u<<section;
            snprintf(text,sizeof(text),"First Prestige native-list probe pending: section=%u row_vector=%08lx,%08lx,%08lx",section,
                (unsigned long)vector[0],(unsigned long)vector[1],(unsigned long)vector[2]);log_line(text);
        }
    }
}
