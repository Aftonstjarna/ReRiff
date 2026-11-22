/* Experimental race free-look camera.
 *
 * Runs from the already-installed race-tick vtable wrapper. It keeps the
 * normal camera active and uses its already-ticked matrix as the free-look
 * source. R3 toggles free look; right stick and (RMB-held) mouse drive yaw
 * and pitch around the car. The engine's own finish/results camera requests
 * take precedence: a pending game switch or the results entry disables free
 * look and restores the normal camera.
 *
 * Opt-in: [camera] race_orbit=1 in client.ini. Optional free_fov_offset,
 * free_zoom, invert_x and invert_y settings live in the same section.
 */
#include <xinput.h>
#include <math.h>
#include <stdlib.h>

static BOOL race_camera_installed,race_camera_announced,race_camera_frame_logged;static unsigned race_camera_frames;
static BOOL race_orbit_enabled;
static BOOL race_photo_enabled,race_photo_active,race_photo_paused_by_us,race_photo_hud_hidden,race_photo_hide_hud=TRUE,race_photo_previous,race_photo_chord,race_photo_music_paused;
static WORD race_photo_buttons[4];static int race_photo_button_count;
static int race_photo_key=VK_F9;
static float race_photo_speed=8.0f;
static float race_photo_yaw,race_photo_pitch;
static float race_photo_position[3],race_photo_base_right[3],race_photo_base_up[3],race_photo_base_forward[3];
static int race_camera_mode=3;
static DWORD race_camera_last_tick;
static float race_camera_yaw,race_camera_idle;
static BOOL race_camera_reset=TRUE,race_camera_free,race_camera_restored=TRUE,race_camera_trace;static BOOL race_r3_previous;static WORD race_free_button=XINPUT_GAMEPAD_RIGHT_THUMB,race_hud_button=XINPUT_GAMEPAD_BACK;static int race_free_key,race_hud_key,race_free_mouse=VK_RBUTTON;static BOOL race_camera_invert_x,race_camera_invert_y;static float race_camera_pitch,race_camera_fov_offset,race_camera_zoom=1.0f;static int race_normal_mode[12];
static BOOL race_mouse_latched;
static POINT race_mouse_last;
typedef DWORD (WINAPI *RaceXInputGetState)(DWORD,XINPUT_STATE *);
static RaceXInputGetState race_xinput_get;

static void race_camera_input_init(void){
    if(race_xinput_get)return;
    HMODULE module=LoadLibraryW(L"xinput1_4.dll");
    if(!module)module=LoadLibraryW(L"xinput1_3.dll");
    if(!module)module=LoadLibraryW(L"xinput9_1_0.dll");
    if(module)race_xinput_get=(RaceXInputGetState)GetProcAddress(module,"XInputGetState");
}

static WORD race_button_value(const WCHAR *value){
    if(!value||!*value||!_wcsicmp(value,L"none"))return 0;
    WCHAR *end=NULL;unsigned long numeric=wcstoul(value,&end,0);
    if(end&&*end==0&&numeric<=0xffffu)return (WORD)numeric;
    static const struct {const WCHAR *name;WORD mask;} table[]={
        {L"dpad_up",XINPUT_GAMEPAD_DPAD_UP},{L"dpad_down",XINPUT_GAMEPAD_DPAD_DOWN},
        {L"dpad_left",XINPUT_GAMEPAD_DPAD_LEFT},{L"dpad_right",XINPUT_GAMEPAD_DPAD_RIGHT},
        {L"start",XINPUT_GAMEPAD_START},{L"back",XINPUT_GAMEPAD_BACK},{L"select",XINPUT_GAMEPAD_BACK},{L"view",XINPUT_GAMEPAD_BACK},
        {L"left_thumb",XINPUT_GAMEPAD_LEFT_THUMB},{L"l3",XINPUT_GAMEPAD_LEFT_THUMB},
        {L"right_thumb",XINPUT_GAMEPAD_RIGHT_THUMB},{L"r3",XINPUT_GAMEPAD_RIGHT_THUMB},
        {L"left_shoulder",XINPUT_GAMEPAD_LEFT_SHOULDER},{L"lb",XINPUT_GAMEPAD_LEFT_SHOULDER},
        {L"right_shoulder",XINPUT_GAMEPAD_RIGHT_SHOULDER},{L"rb",XINPUT_GAMEPAD_RIGHT_SHOULDER},
        {L"a",XINPUT_GAMEPAD_A},{L"b",XINPUT_GAMEPAD_B},{L"x",XINPUT_GAMEPAD_X},{L"y",XINPUT_GAMEPAD_Y}};
    for(unsigned i=0;i<sizeof(table)/sizeof(table[0]);i++)if(!_wcsicmp(value,table[i].name))return table[i].mask;
    return 0;
}

static int race_key_value(const WCHAR *value){
    if(!value||!*value||!_wcsicmp(value,L"none"))return 0;
    WCHAR *end=NULL;unsigned long numeric=wcstoul(value,&end,0);
    if(end&&*end==0&&numeric<=255u)return (int)numeric;
    if(value[0]&&!value[1]){
        if(value[0]>='a'&&value[0]<='z')return (int)(value[0]-32);
        if((value[0]>='A'&&value[0]<='Z')||(value[0]>='0'&&value[0]<='9'))return (int)value[0];
    }
    if((value[0]=='F'||value[0]=='f')&&value[1]){int n=_wtoi(value+1);if(n>=1&&n<=24)return VK_F1+n-1;}
    static const struct {const WCHAR *name;int key;} table[]={
        {L"space",VK_SPACE},{L"enter",VK_RETURN},{L"return",VK_RETURN},{L"tab",VK_TAB},
        {L"escape",VK_ESCAPE},{L"esc",VK_ESCAPE},{L"backspace",VK_BACK},{L"insert",VK_INSERT},
        {L"delete",VK_DELETE},{L"del",VK_DELETE},{L"home",VK_HOME},{L"end",VK_END},
        {L"pageup",VK_PRIOR},{L"pgup",VK_PRIOR},{L"pagedown",VK_NEXT},{L"pgdn",VK_NEXT},
        {L"up",VK_UP},{L"down",VK_DOWN},{L"left",VK_LEFT},{L"right",VK_RIGHT},
        {L"shift",VK_SHIFT},{L"lshift",VK_LSHIFT},{L"rshift",VK_RSHIFT},
        {L"ctrl",VK_CONTROL},{L"control",VK_CONTROL},{L"lctrl",VK_LCONTROL},{L"rctrl",VK_RCONTROL},
        {L"alt",VK_MENU},{L"lalt",VK_LMENU},{L"ralt",VK_RMENU},
        {L"numpad0",VK_NUMPAD0},{L"numpad1",VK_NUMPAD1},{L"numpad2",VK_NUMPAD2},{L"numpad3",VK_NUMPAD3},
        {L"numpad4",VK_NUMPAD4},{L"numpad5",VK_NUMPAD5},{L"numpad6",VK_NUMPAD6},{L"numpad7",VK_NUMPAD7},
        {L"numpad8",VK_NUMPAD8},{L"numpad9",VK_NUMPAD9},{L"multiply",VK_MULTIPLY},{L"add",VK_ADD},
        {L"subtract",VK_SUBTRACT},{L"decimal",VK_DECIMAL},{L"divide",VK_DIVIDE}};
    for(unsigned i=0;i<sizeof(table)/sizeof(table[0]);i++)if(!_wcsicmp(value,table[i].name))return table[i].key;
    return 0;
}

static int race_mouse_value(const WCHAR *value){
    if(!value||!_wcsicmp(value,L"none"))return 0;
    if(!_wcsicmp(value,L"left")||!_wcsicmp(value,L"lmb"))return VK_LBUTTON;
    if(!_wcsicmp(value,L"middle")||!_wcsicmp(value,L"mmb"))return VK_MBUTTON;
    if(!_wcsicmp(value,L"right")||!_wcsicmp(value,L"rmb"))return VK_RBUTTON;
    return VK_RBUTTON;
}

static BOOL race_button_down(WORD mask);
static void race_photo_parse_buttons(const WCHAR *value){
    race_photo_button_count=0;
    WCHAR text[64];wcsncpy(text,value?value:L"",63);text[63]=0;
    WCHAR *p=text;
    while(*p&&race_photo_button_count<4){
        while(*p==L' '||*p==L'\t'||*p==L'+'||*p==L',')p++;
        if(!*p)break;
        WCHAR *start=p;
        while(*p&&*p!=L'+'&&*p!=L','&&*p!=L' '&&*p!=L'\t')p++;
        WCHAR saved=*p;*p=0;
        WORD mask=race_button_value(start);
        if(mask)race_photo_buttons[race_photo_button_count++]=mask;
        *p=saved;
    }
}

static BOOL race_photo_chord_down(void){
    if(!race_photo_button_count)return FALSE;
    for(int i=0;i<race_photo_button_count;i++)if(!race_button_down(race_photo_buttons[i]))return FALSE;
    return TRUE;
}

static BOOL race_button_down(WORD mask){
    if(!mask||!race_xinput_get)return FALSE;
    XINPUT_STATE state;
    return race_xinput_get(0,&state)==ERROR_SUCCESS&&(state.Gamepad.wButtons&mask)!=0;
}

static BOOL race_key_down(int key){return key&&(GetAsyncKeyState(key)&0x8000)!=0;}

static void race_read_controls(const WCHAR *client){
    WCHAR value[64];
    GetPrivateProfileStringW(L"controls",L"free_look_button",L"right_thumb",value,64,client);race_free_button=race_button_value(value);
    GetPrivateProfileStringW(L"controls",L"hud_button",L"back",value,64,client);race_hud_button=race_button_value(value);
    GetPrivateProfileStringW(L"controls",L"free_look_key",L"",value,64,client);race_free_key=race_key_value(value);
    GetPrivateProfileStringW(L"controls",L"hud_key",L"",value,64,client);race_hud_key=race_key_value(value);
    GetPrivateProfileStringW(L"controls",L"free_look_mouse",L"right",value,64,client);race_free_mouse=race_mouse_value(value);
    GetPrivateProfileStringW(L"controls",L"photo_mode_button",L"left_thumb+right_thumb",value,64,client);race_photo_parse_buttons(value);
    GetPrivateProfileStringW(L"controls",L"photo_mode_key",L"F9",value,64,client);race_photo_key=race_key_value(value);
}

static void race_camera_weight(void *camera,int active,float weight){
    if(!readable(camera,24))return;
    *((BYTE *)camera+12)=(BYTE)(active?1:0);
    *((float *)((BYTE *)camera+16))=weight;
    *((float *)((BYTE *)camera+20))=0.0f;
}

static void race_camera_rotate(float *v,const float axis[3],float angle){
    float c=(float)cos(angle),s=(float)sin(angle);
    float x=v[0],y=v[1],z=v[2];
    float dot=axis[0]*x+axis[1]*y+axis[2]*z;
    float cx=axis[1]*z-axis[2]*y,cy=axis[2]*x-axis[0]*z,cz=axis[0]*y-axis[1]*x;
    v[0]=x*c+cx*s+axis[0]*dot*(1.0f-c);
    v[1]=y*c+cy*s+axis[1]*dot*(1.0f-c);
    v[2]=z*c+cz*s+axis[2]*dot*(1.0f-c);
}

static void race_camera_disable(void){
    if(!race_camera_free)return;
    race_camera_free=FALSE;race_camera_restored=FALSE;race_camera_reset=TRUE;race_camera_pitch=0;
    log_line("Race free-look camera disabled for a game-controlled camera transition.");
}

static void race_camera_axes(float *stickX,float *stickY,float *dx,float *dy,BOOL *right){
    *stickX=*stickY=*dx=*dy=0.0f;*right=FALSE;
    if(race_xinput_get){
        XINPUT_STATE state;
        if(race_xinput_get(0,&state)==ERROR_SUCCESS){
            int x=state.Gamepad.sThumbRX,y=state.Gamepad.sThumbRY;
            if(x>7849)*stickX=(x-7849)/24766.0f;else if(x<-7849)*stickX=(x+7849)/24766.0f;
            if(y>7849)*stickY=(y-7849)/24766.0f;else if(y<-7849)*stickY=(y+7849)/24766.0f;
        }
    }
    HWND window=GetForegroundWindow();
    BOOL rmb=race_free_mouse&&(GetAsyncKeyState(race_free_mouse)&0x8000)!=0;
    if(rmb&&window){
        RECT client;POINT center;
        if(GetClientRect(window,&client)&&client.right>0&&client.bottom>0){
            center.x=client.right/2;center.y=client.bottom/2;ClientToScreen(window,&center);
            POINT cursor;
            if(GetCursorPos(&cursor)){
                if(race_mouse_latched){*dx=(float)(cursor.x-race_mouse_last.x);*dy=(float)(cursor.y-race_mouse_last.y);}
                else{*dx=0;*dy=0;race_mouse_latched=TRUE;}
                race_mouse_last=center;SetCursorPos(center.x,center.y);*right=TRUE;
            }
        }
    }else race_mouse_latched=FALSE;
}

static void *race_camera_find(void *set,int mode){
    if(!readable(set,24))return NULL;
    void **begin=*(void ***)((BYTE *)set+12),**end=*(void ***)((BYTE *)set+16);
    if(!begin||!end||end<begin||(BYTE *)end-(BYTE *)begin>4096)return NULL;
    for(void **item=begin;item<end;item++){
        void *camera=*item;if(!readable(camera,24))continue;
        if(*(int *)((BYTE *)camera+8)==mode)return camera;
    }
    return NULL;
}

typedef void *(__thiscall *RacePlayerMatrix)(void *);
static BOOL race_camera_player(void *player,float *position,float *forward){
    if(!readable(player,76))return FALSE;
    /* player+72 stores a handle; dereference it to the gaVehiclePhys object,
     * exactly as gaCameraChase.tick does before reading body+8/body+104. */
    void *handle=*(void **)((BYTE *)player+72);
    if(!readable(handle,4))return FALSE;
    void *body=*(void **)handle;
    if(!readable(body,116))return FALSE;
    float *m=(float *)((BYTE *)body+8),*l=(float *)((BYTE *)body+104);
    position[0]=m[0]*l[0]+m[3]*l[1]+m[6]*l[2]+m[9];
    position[1]=m[1]*l[0]+m[4]*l[1]+m[7]*l[2]+m[10];
    position[2]=m[2]*l[0]+m[5]*l[1]+m[8]*l[2]+m[11];
    void **vtable=*(void ***)player;
    if(readable(vtable,8)&&vtable[1]){
        void *matrix=((RacePlayerMatrix)vtable[1])(player);
        if(readable(matrix,48)){
            float *pm=(float *)matrix;
            forward[0]=pm[6];forward[1]=pm[7];forward[2]=pm[8];
            return TRUE;
        }
    }
    forward[0]=m[6];forward[1]=m[7];forward[2]=m[8];
    return TRUE;
}

static float race_angle_difference(float to,float from){
    float difference=to-from;
    while(difference>3.14159265f)difference-=6.28318531f;
    while(difference<-3.14159265f)difference+=6.28318531f;
    return difference;
}

static void race_camera_update(void *camera,void *player,float dt){
    if(!readable(camera,236))return;
    float position[3],forward[3];
    if(!race_camera_player(player,position,forward))return;
    float *look=(float *)((BYTE *)camera+200);
    look[0]=position[0];look[1]=position[1]+1.75f;look[2]=position[2];
    *((float *)((BYTE *)camera+212))=0.0f;
    *((float *)((BYTE *)camera+216))=8.0f;
    float behind=(float)atan2(-forward[2],-forward[0]);
    if(race_camera_reset){race_camera_yaw=behind;race_camera_pitch=0;race_camera_idle=0;race_camera_reset=FALSE;}
    float stickX,stickY,dx,dy;BOOL right;
    race_camera_axes(&stickX,&stickY,&dx,&dy,&right);
    BOOL input=(stickX!=0)||(stickY!=0)||(right&&(dx!=0||dy!=0));
    if(input){
        float yawDelta=stickX*2.5f*dt+dx*0.0045f;
        float pitchDelta=(-stickY*2.0f*dt)-dy*0.0045f;
        race_camera_yaw+=(race_camera_invert_x?yawDelta:-yawDelta);
        race_camera_pitch+=(race_camera_invert_y?-pitchDelta:pitchDelta);
        race_camera_idle=0;
    }else{
        race_camera_idle+=dt;
        if(right)race_camera_idle=0;
        if(race_camera_idle>0.65f){
            race_camera_yaw+=race_angle_difference(behind,race_camera_yaw)*(dt*3.5f>1.0f?1.0f:dt*3.5f);
            race_camera_pitch+=-race_camera_pitch*(dt*3.5f>1.0f?1.0f:dt*3.5f);
        }
    }
    while(race_camera_yaw>3.14159265f)race_camera_yaw-=6.28318531f;
    while(race_camera_yaw<-3.14159265f)race_camera_yaw+=6.28318531f;
    if(race_camera_pitch>1.1f)race_camera_pitch=1.1f;
    if(race_camera_pitch<-1.1f)race_camera_pitch=-1.1f;
    *((float *)((BYTE *)camera+220))=race_camera_yaw;
}

static void race_camera_activate(void *set,float dt,int index){
    if(!readable(set,88))return;
    void *player=*(void **)((BYTE *)set+4);
    void *orbit=race_camera_find(set,12);
    if(!player||!orbit)return;
    if(index<0||index>=12)return;
    if(!race_normal_mode[index]){
        int mode=readable(set,112)?*((int *)((BYTE *)set+44)):0;
        if(mode==12||mode==0)mode=3;
        race_normal_mode[index]=mode;
    }
    void *normal=race_camera_find(set,race_normal_mode[index]);
    if(!normal)normal=race_camera_find(set,3);
    if(!race_camera_free){
        if(readable(set,112)){
            int mode=*((int *)((BYTE *)set+44));
            if(mode!=12&&mode!=0)race_normal_mode[index]=mode;
        }
        *((BYTE *)set+80)=1;
        if(normal)race_camera_weight(normal,1,1.0f);
        race_camera_weight(orbit,0,0.0f);
        return;
    }
    *((BYTE *)set+80)=0;
    if(readable(set,112)){
        /* A pending switch is a game request (finish, scripted, pause).
         * Yield instead of clearing it, so the end camera can run. */
        if(*((int *)((BYTE *)set+100))||*((int *)((BYTE *)set+108))){race_camera_disable();return;}
    }
    void **begin=*(void ***)((BYTE *)set+12),**end=*(void ***)((BYTE *)set+16);
    if(begin&&end&&end>=begin&&(BYTE *)end-(BYTE *)begin<=4096){
        for(void **item=begin;item<end;item++){
            void *other=*item;if(!readable(other,24))continue;
            if(other==orbit)race_camera_weight(other,1,1.0f);
            else if(other==normal)race_camera_weight(other,1,0.0f);
            else race_camera_weight(other,0,0.0f);
        }
    }
    if(!race_camera_announced){
        race_camera_announced=TRUE;log_line("Race free-look camera active; right stick/mouse x/y attached to the normal camera.");
    }
    race_camera_update(orbit,player,dt);
    void *source=normal&&readable(normal,236)?normal:(race_camera_find(set,3));
    if(source&&readable(source,236)&&readable(orbit,236)){
        float position[3],forward[3];
        if(race_camera_player(player,position,forward)){
            /* Keep the normal camera's field of view; the orbit camera's
             * default FOV is lower and looked like an unwanted zoom. */
            float fov=*((float *)((BYTE *)source+48))+race_camera_fov_offset;
            if(fov<20.0f)fov=20.0f;if(fov>120.0f)fov=120.0f;
            *((float *)((BYTE *)orbit+48))=fov;
            float *src=(float *)((BYTE *)source+56),*dst=(float *)((BYTE *)orbit+56);
            memcpy(dst,src,48);
            float behind=(float)atan2(-forward[2],-forward[0]);
            float yaw=race_camera_yaw-behind;
            const float up[3]={0,1,0};
            float offset[3]={dst[9]-position[0],dst[10]-position[1],dst[11]-position[2]};
            for(int row=0;row<3;row++)race_camera_rotate(dst+row*3,up,yaw);
            race_camera_rotate(offset,up,yaw);
            float axis[3]={dst[0],dst[1],dst[2]};
            float length=(float)sqrt(axis[0]*axis[0]+axis[1]*axis[1]+axis[2]*axis[2]);
            if(length>0.0001f){axis[0]/=length;axis[1]/=length;axis[2]/=length;
                for(int row=0;row<3;row++)race_camera_rotate(dst+row*3,axis,race_camera_pitch);
                race_camera_rotate(offset,axis,race_camera_pitch);
            }
            dst[9]=position[0]+offset[0]*race_camera_zoom;
            dst[10]=position[1]+offset[1]*race_camera_zoom;
            dst[11]=position[2]+offset[2]*race_camera_zoom;
        }
    }
}

typedef void (__cdecl *RaceHudToggle)(void);
static RaceHudToggle race_hud_toggle;

static void race_photo_normalize(float *v){
    float length=(float)sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
    if(length>0.00001f){v[0]/=length;v[1]/=length;v[2]/=length;}
}

static void *race_photo_final_camera(BYTE *base,int *set_index){
    if(!readable(base+0x7cb408,4))return NULL;
    void *manager=*(void **)(base+0x7cb408);
    if(!readable(manager,4))return NULL;
    for(int index=0;index<12;index++){
        void *set=((void **)manager)[index];
        if(!set||!readable(set,12))continue;
        void *viewport=*(void **)((BYTE *)set+8);
        if(!readable(viewport,432))continue;
        void *camera=*(void **)((BYTE *)viewport+428);
        if(!readable(camera,116))continue;
        if(set_index)*set_index=index;
        return camera;
    }
    return NULL;
}

static void race_photo_restore_hud(BYTE *base){
    if(race_photo_hud_hidden&&readable(base+0x7a2f48,1)&&!*((BYTE *)base+0x7a2f48)&&race_hud_toggle)race_hud_toggle();
    race_photo_hud_hidden=FALSE;
}

static void *race_photo_audio(BYTE *base){
    if(!readable(base+0x7e750c,4))return NULL;
    void *audio=*(void **)(base+0x7e750c);
    return readable(audio,164)?audio:NULL;
}

static void race_photo_pause_music(BYTE *base){
    void *audio=race_photo_audio(base);
    if(!audio||*((BYTE *)audio+109))return;
    void *source=*(void **)((BYTE *)audio+100);
    if(!readable(source,4))return;
    void **vtable=*(void ***)source;
    if(!readable(vtable,8)||!vtable[1])return;
    ((void (__thiscall *)(void *))vtable[1])(source);
    *((BYTE *)audio+109)=1;
    race_photo_music_paused=TRUE;
    log_line("Photo mode: music paused.");
}

static void race_photo_resume_music(BYTE *base){
    if(!race_photo_music_paused)return;
    void *audio=race_photo_audio(base);
    if(audio&&*((BYTE *)audio+109)){
        void *source=*(void **)((BYTE *)audio+100);
        if(readable(source,4)){
            void **vtable=*(void ***)source;
            if(readable(vtable,4)&&vtable[0])((void (__thiscall *)(void *))vtable[0])(source);
        }
        *((BYTE *)audio+109)=0;
        log_line("Photo mode: music resumed.");
    }
    race_photo_music_paused=FALSE;
}

static BOOL race_photo_begin(BYTE *base){
    if(!readable(base+0x7d6668,4))return FALSE;
    void *race=*(void **)(base+0x7d6668);
    if(!readable(race,13)||*((BYTE *)race+12)){log_line("Photo mode refused: the race is already paused.");return FALSE;}
    int set_index=-1;void *camera=race_photo_final_camera(base,&set_index);
    if(!camera){log_line("Photo mode refused: no final race camera is available.");return FALSE;}
    char begin[160];snprintf(begin,sizeof(begin),"Photo mode begin; race=%p camera set %d camera=%p.",race,set_index,camera);log_line(begin);
    float *matrix=(float *)((BYTE *)camera+68);
    for(int row=0;row<3;row++){
        race_photo_base_right[row]=matrix[row];
        race_photo_base_up[row]=matrix[3+row];
        race_photo_base_forward[row]=matrix[6+row];
        race_photo_position[row]=matrix[9+row];
    }
    race_photo_yaw=0;race_photo_pitch=0;
    *((BYTE *)race+12)=1;
    race_photo_paused_by_us=TRUE;
    log_line("Photo mode: pause byte set.");
    if(race_photo_hide_hud&&readable(base+0x7a2f48,1)&&*((BYTE *)base+0x7a2f48)&&race_hud_toggle){
        race_hud_toggle();race_photo_hud_hidden=TRUE;
        log_line("Photo mode: HUD hidden.");
    }
    race_photo_pause_music(base);
    race_photo_active=TRUE;
    race_camera_free=FALSE;race_camera_restored=FALSE;race_camera_reset=TRUE;
    char line[128];snprintf(line,sizeof(line),"Photo mode enabled; race paused, camera set %d.",set_index);log_line(line);
    return TRUE;
}

static void race_photo_end(BYTE *base,BOOL resume){
    if(!race_photo_active)return;
    race_photo_active=FALSE;
    race_photo_restore_hud(base);
    if(resume&&race_photo_paused_by_us&&readable(base+0x7d6668,4)){
        void *race=*(void **)(base+0x7d6668);
        if(readable(race,13)&&*((BYTE *)race+12))*((BYTE *)race+12)=0;
    }
    race_photo_resume_music(base);
    race_photo_paused_by_us=FALSE;
    race_camera_free=FALSE;race_camera_restored=FALSE;race_camera_reset=TRUE;
    log_line("Photo mode disabled; normal camera restored.");
}

static void race_photo_race_enter(void){
    race_photo_active=FALSE;race_photo_paused_by_us=FALSE;race_photo_hud_hidden=FALSE;race_photo_music_paused=FALSE;
    race_photo_previous=FALSE;race_photo_chord=FALSE;
}

static void race_photo_disable(void){
    if(!race_photo_active)return;
    race_photo_end((BYTE *)GetModuleHandleW(NULL),TRUE);
}

static void race_photo_move(float *x,float *y,float *z,BOOL *boost){
    *x=*y=*z=0;*boost=FALSE;
    if(race_xinput_get){
        XINPUT_STATE state;
        if(race_xinput_get(0,&state)==ERROR_SUCCESS){
            int lx=state.Gamepad.sThumbLX,ly=state.Gamepad.sThumbLY;
            if(lx>7849)*x=(lx-7849)/24766.0f;else if(lx<-7849)*x=(lx+7849)/24766.0f;
            if(ly>7849)*y=(ly-7849)/24766.0f;else if(ly<-7849)*y=(ly+7849)/24766.0f;
            if(state.Gamepad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER)*z-=1.0f;
            if(state.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER)*z+=1.0f;
            if(state.Gamepad.bRightTrigger>30)*boost=TRUE;
        }
    }
    if(race_key_down('W'))*y+=1.0f;if(race_key_down('S'))*y-=1.0f;
    if(race_key_down('D'))*x+=1.0f;if(race_key_down('A'))*x-=1.0f;
    if(race_key_down('E'))*z+=1.0f;if(race_key_down('Q'))*z-=1.0f;
    if(race_key_down(VK_SPACE))*z+=1.0f;
    if(race_key_down(VK_LCONTROL)||race_key_down(VK_CONTROL))*z-=1.0f;
    if(race_key_down(VK_SHIFT))*boost=TRUE;
}

static void race_photo_update(BYTE *base,float dt){
    float stickX,stickY,dx,dy;BOOL right;
    race_camera_axes(&stickX,&stickY,&dx,&dy,&right);
    if((stickX!=0)||(stickY!=0)||(right&&(dx!=0||dy!=0))){
        float yawDelta=stickX*2.5f*dt+dx*0.0045f;
        float pitchDelta=(-stickY*2.0f*dt)-dy*0.0045f;
        race_photo_yaw+=(race_camera_invert_x?yawDelta:-yawDelta);
        race_photo_pitch+=(race_camera_invert_y?-pitchDelta:pitchDelta);
    }
    if(race_photo_pitch>1.45f)race_photo_pitch=1.45f;
    if(race_photo_pitch<-1.45f)race_photo_pitch=-1.45f;
    while(race_photo_yaw>3.14159265f)race_photo_yaw-=6.28318531f;
    while(race_photo_yaw<-3.14159265f)race_photo_yaw+=6.28318531f;
    float rightVector[3]={race_photo_base_right[0],race_photo_base_right[1],race_photo_base_right[2]};
    float upVector[3]={race_photo_base_up[0],race_photo_base_up[1],race_photo_base_up[2]};
    float forwardVector[3]={race_photo_base_forward[0],race_photo_base_forward[1],race_photo_base_forward[2]};
    const float worldUp[3]={0,1,0};
    race_camera_rotate(rightVector,worldUp,race_photo_yaw);
    race_camera_rotate(upVector,worldUp,race_photo_yaw);
    race_camera_rotate(forwardVector,worldUp,race_photo_yaw);
    float pitchAxis[3]={rightVector[0],rightVector[1],rightVector[2]};
    race_camera_rotate(rightVector,pitchAxis,race_photo_pitch);
    race_camera_rotate(upVector,pitchAxis,race_photo_pitch);
    race_camera_rotate(forwardVector,pitchAxis,race_photo_pitch);
    race_photo_normalize(rightVector);race_photo_normalize(upVector);race_photo_normalize(forwardVector);
    float moveX,moveY,moveZ;BOOL boost;
    race_photo_move(&moveX,&moveY,&moveZ,&boost);
    float movement[3];
    for(int axis=0;axis<3;axis++)movement[axis]=rightVector[axis]*moveX+forwardVector[axis]*moveY+worldUp[axis]*moveZ;
    float length=(float)sqrt(movement[0]*movement[0]+movement[1]*movement[1]+movement[2]*movement[2]);
    if(length>1.0f){movement[0]/=length;movement[1]/=length;movement[2]/=length;}
    float speed=race_photo_speed*(boost?3.0f:1.0f);
    for(int axis=0;axis<3;axis++)race_photo_position[axis]+=movement[axis]*speed*dt;
    int set_index=-1;void *camera=race_photo_final_camera(base,&set_index);
    if(!camera){race_photo_end(base,TRUE);return;}
    float *matrix=(float *)((BYTE *)camera+68);
    for(int row=0;row<3;row++){
        matrix[row]=rightVector[row];
        matrix[3+row]=upVector[row];
        matrix[6+row]=forwardVector[row];
        matrix[9+row]=race_photo_position[row];
    }
    if(race_camera_trace&&(++race_camera_frames%90)==0){
        char line[180];
        snprintf(line,sizeof(line),"Photo camera trace frame %u set=%d pos=%.2f,%.2f,%.2f yaw=%.2f pitch=%.2f",race_camera_frames,set_index,race_photo_position[0],race_photo_position[1],race_photo_position[2],race_photo_yaw,race_photo_pitch);
        log_line(line);
    }
}

static void race_photo_frame(BYTE *base,float dt){
    race_photo_chord=FALSE;
    if(!race_photo_enabled)return;
    BOOL chord=race_photo_chord_down()||race_key_down(race_photo_key);
    race_photo_chord=chord;
    if(chord&&!race_photo_previous){
        if(race_photo_active)race_photo_end(base,TRUE);
        else race_photo_begin(base);
    }
    race_photo_previous=chord;
    if(!race_photo_active)return;
    if(!readable(base+0x7d6668,4)){race_photo_end(base,FALSE);return;}
    void *race=*(void **)(base+0x7d6668);
    if(!readable(race,13)||!*((BYTE *)race+12)){race_photo_end(base,FALSE);return;}
    race_photo_update(base,dt);
}

/* Called from the proven race-tick vtable wrapper. The original
 * gaCameraManager.tick still runs and ticks the cameras; this pass only
 * activates the orbit camera and writes its fields before the next tick. */
static void race_camera_frame(void){
    if(!race_camera_installed)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!readable(base+0x7cb408,4))return;
    void *manager=*(void **)(base+0x7cb408);
    if(!readable(manager,4))return;
    if(!race_camera_frame_logged){
        race_camera_frame_logged=TRUE;char detail[160];
        snprintf(detail,sizeof(detail),"Race orbit camera frame: manager=%p set0=%p mode=%d",manager,readable(manager,4)?((void **)manager)[0]:NULL,race_camera_mode);
        log_line(detail);
    }
    DWORD now=GetTickCount();
    float dt=race_camera_last_tick?(now-race_camera_last_tick)/1000.0f:0.016f;
    if(dt<0.0f||dt>0.05f)dt=0.016f;
    race_camera_last_tick=now;
    race_photo_frame(base,dt);
    BOOL r3=race_button_down(race_free_button)||race_key_down(race_free_key);
    if(race_photo_active||race_photo_chord){race_r3_previous=r3;return;}
    if(!race_orbit_enabled)return;
    if(r3&&!race_r3_previous){
        race_camera_free=!race_camera_free;race_camera_reset=TRUE;race_camera_pitch=0;race_camera_restored=FALSE;
        if(race_camera_free)for(int index=0;index<12;index++)race_normal_mode[index]=0;
        log_line(race_camera_free?"Race free-look camera enabled.":"Race free-look camera disabled; normal camera restored.");
    }
    race_r3_previous=r3;
    if(!race_camera_free&&race_camera_restored)return;
    for(int index=0;index<12;index++){
        void *set=((void **)manager)[index];
        if(!set)continue;
        if(race_camera_mode>=2)race_camera_activate(set,dt,index);
    }
    if(!race_camera_free){race_camera_restored=TRUE;return;}
    if(race_camera_trace&&race_camera_mode>=2&&(++race_camera_frames%90)==0){
        char header[160];
        snprintf(header,sizeof(header),"Race camera trace frame %u manager=%p free=%d pitch=%.2f yaw=%.2f",race_camera_frames,manager,race_camera_free,race_camera_pitch,race_camera_yaw);
        log_line(header);
        for(int set_index=0;set_index<12;set_index++){
            void *set=((void **)manager)[set_index];
            if(!set||!readable(set,112))continue;
            void *player=*(void **)((BYTE *)set+4);
            int active=*((int *)((BYTE *)set+44));
            int pending=*((int *)((BYTE *)set+100));
            void **begin=*(void ***)((BYTE *)set+12),**end=*(void ***)((BYTE *)set+16);
            int count=(begin&&end&&end>=begin&&(BYTE *)end-(BYTE *)begin<=4096)?(int)(end-begin):-1;
            char line[220];
            snprintf(line,sizeof(line),"Race camera set %d set=%p player=%p active=%d pending=%d cams=%d",set_index,set,player,active,pending,count);
            log_line(line);
            if(count<=0)continue;
            for(int camera_index=0;camera_index<count;camera_index++){
                void *camera=begin[camera_index];
                if(!readable(camera,236)){snprintf(line,sizeof(line),"Race camera set %d cam %d invalid=%p",set_index,camera_index,camera);log_line(line);continue;}
                int mode=*((int *)((BYTE *)camera+8));
                int cam_active=*((BYTE *)camera+12);
                float weight=*((float *)((BYTE *)camera+16));
                float weight2=*((float *)((BYTE *)camera+20));
                float blend=*((float *)((BYTE *)camera+48));
                float *m=(float *)((BYTE *)camera+56);
                float px=m[9],py=m[10],pz=m[11];
                float lx=0,ly=0,lz=0;
                if(mode==12){float *look=(float *)((BYTE *)camera+200);lx=look[0];ly=look[1];lz=look[2];}
                snprintf(line,sizeof(line),"Race camera set %d cam %d ptr=%p mode=%d active=%d w=%.2f/%.2f/%.2f pos=%.2f,%.2f,%.2f look=%.2f,%.2f,%.2f",set_index,camera_index,camera,mode,cam_active,weight,weight2,blend,px,py,pz,lx,ly,lz);
                log_line(line);
            }
            void *viewport=*(void **)((BYTE *)set+8);
            void *elcamera=readable(viewport,432)?*(void **)((BYTE *)viewport+428):NULL;
            if(readable(elcamera,116)){
                float *fm=(float *)((BYTE *)elcamera+68);
                snprintf(line,sizeof(line),"Race camera set %d final viewport=%p pos=%.2f,%.2f,%.2f",set_index,elcamera,fm[9],fm[10],fm[11]);log_line(line);
            }
        }
    }
}

static BOOL race_hud_select_previous;
__declspec(dllexport) int __cdecl ReRiffConfigureHudToggle(void *callback){
    if(!callback)return 0;race_hud_toggle=(RaceHudToggle)callback;return 1;
}
static void race_hud_frame(void){
    if(!race_hud_toggle)return;
    if(!race_xinput_get)race_camera_input_init();
    BOOL select=race_button_down(race_hud_button)||race_key_down(race_hud_key);
    if(select&&!race_hud_select_previous)race_hud_toggle();
    race_hud_select_previous=select;
}

static BOOL install_race_camera(BYTE *base){
    WCHAR client[MAX_PATH];wcscpy(client,config_path);WCHAR *slash=wcsrchr(client,L'\\');if(slash)wcscpy(slash+1,L"client.ini");
    race_read_controls(client);
    BOOL orbit=GetPrivateProfileIntW(L"camera",L"race_orbit",0,client)!=0;
    BOOL photo=GetPrivateProfileIntW(L"camera",L"photo_mode",0,client)!=0;
    if(!orbit&&!photo)return TRUE;
    if(race_camera_installed)return TRUE;
    race_orbit_enabled=orbit;race_photo_enabled=photo;
    race_camera_mode=GetPrivateProfileIntW(L"camera",L"race_orbit_mode",3,client);race_camera_trace=GetPrivateProfileIntW(L"camera",L"race_orbit_trace",0,client);
    WCHAR value[64];
    GetPrivateProfileStringW(L"camera",L"free_fov_offset",L"0",value,64,client);race_camera_fov_offset=(float)wcstod(value,NULL);
    GetPrivateProfileStringW(L"camera",L"free_zoom",L"1",value,64,client);race_camera_zoom=(float)wcstod(value,NULL);
    if(race_camera_zoom<0.25f)race_camera_zoom=0.25f;if(race_camera_zoom>4.0f)race_camera_zoom=4.0f;
    race_camera_invert_x=GetPrivateProfileIntW(L"camera",L"invert_x",0,client)!=0;
    race_camera_invert_y=GetPrivateProfileIntW(L"camera",L"invert_y",0,client)!=0;
    if(race_photo_enabled){
        GetPrivateProfileStringW(L"camera",L"photo_speed",L"8.0",value,64,client);race_photo_speed=(float)wcstod(value,NULL);
        if(race_photo_speed<1.0f)race_photo_speed=1.0f;if(race_photo_speed>100.0f)race_photo_speed=100.0f;
        race_photo_hide_hud=GetPrivateProfileIntW(L"camera",L"photo_hide_hud",1,client)!=0;
        if(!readable(base+0x7d6668,4)){race_photo_enabled=FALSE;log_line("Photo mode refused: the race manager singleton differs from the supported game.");}
    }
    if(!race_orbit_enabled&&!race_photo_enabled)return TRUE;
    race_camera_input_init();
    race_camera_installed=TRUE;
    char detail[240];snprintf(detail,sizeof(detail),"Race camera hook ready; free_look=%d photo=%d mode=%d fov_offset=%.1f zoom=%.2f invert_x=%d invert_y=%d photo_speed=%.1f.",race_orbit_enabled?1:0,race_photo_enabled?1:0,race_camera_mode,race_camera_fov_offset,race_camera_zoom,race_camera_invert_x,race_camera_invert_y,race_photo_speed);log_line(detail);
    return TRUE;
}
