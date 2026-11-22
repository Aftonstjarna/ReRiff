/* Optional track culling disable.
 *
 * The race manager keeps every gaTriggerCullingVolume that was created from
 * the track. Each volume owns the props/triggers for a section and the game
 * hides the whole section when its per-view visibility counters reach zero.
 * Writing the counters non-zero keeps those sections eligible on the next
 * tick. While the race is paused the trigger manager does not tick, so the
 * paused path also forces the volume's existing onVisible callbacks.
 *
 * Opt-in: [graphics] disable_culling=1 in server.ini. Restart to apply.
 */
typedef void (__thiscall *RaceCullingVisible)(void *,int,BYTE);

static BOOL race_culling_enabled,race_culling_trace,race_culling_trace_announced;
static unsigned race_culling_frames,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count;
static RaceCullingVisible race_culling_force_visible,race_culling_on_visible;

static BOOL install_race_culling(BYTE *base){
    WCHAR value[16];
    GetPrivateProfileStringW(L"graphics",L"disable_culling",L"false",value,16,config_path);
    race_culling_enabled=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    WCHAR client[MAX_PATH];wcsncpy(client,config_path,MAX_PATH-1);client[MAX_PATH-1]=0;
    WCHAR *slash=wcsrchr(client,L'\\');if(slash)wcscpy(slash+1,L"client.ini");
    GetPrivateProfileStringW(L"camera",L"race_orbit_trace",L"false",value,16,client);
    race_culling_trace=!_wcsicmp(value,L"true")||!_wcsicmp(value,L"yes")||!wcscmp(value,L"1");
    if(!race_culling_enabled)return TRUE;
    if(!readable(base+0x278ea0,8)||!readable(base+0x27959c,8))return FALSE;
    race_culling_force_visible=(RaceCullingVisible)(base+0x278ea0);
    race_culling_on_visible=(RaceCullingVisible)(base+0x27959c);
    char detail[128];snprintf(detail,sizeof(detail),"Track culling disable enabled; trace=%d.",race_culling_trace?1:0);log_line(detail);
    return TRUE;
}

static BOOL race_culling_paused(BYTE *base){
    if(!readable(base+0x7d6668,4))return FALSE;
    void *race=*(void **)(base+0x7d6668);
    return readable(race,13)&&*((BYTE *)race+12)!=0;
}

static void race_culling_apply(void *manager,BOOL paused){
    race_culling_volume_count=race_culling_valid_count=race_culling_forced_count=0;
    if(!readable(manager,36))return;
    void **begin=*(void ***)((BYTE *)manager+28),**end=*(void ***)((BYTE *)manager+32);
    if(!begin||!end||end<begin||(BYTE *)end-(BYTE *)begin>32768)return;
    race_culling_volume_count=(unsigned)(end-begin);
    for(void **item=begin;item<end;item++){
        void *volume=*item;
        if(!readable(volume,316))continue;
        race_culling_valid_count++;
        /* All twelve viewport counters, not just the four the tick helper
         * samples, so split-screen and replay views stay eligible too. */
        for(int view=0;view<12;view++)*((WORD *)((BYTE *)volume+124+view*2))=1;
        if(!paused)continue;
        race_culling_forced_count++;
        for(int view=0;view<4;view++){
            if(race_culling_force_visible)race_culling_force_visible(volume,view,1);
            if(race_culling_on_visible)race_culling_on_visible(volume,view,1);
        }
    }
}

static void race_culling_frame(void){
    if(!race_culling_enabled)return;
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    void *manager=readable(base+0x7df324,4)?*(void **)(base+0x7df324):NULL;
    BOOL paused=race_culling_paused(base);
    race_culling_apply(manager,paused);
    if(!race_culling_trace)return;
    if(!race_culling_trace_announced){
        race_culling_trace_announced=TRUE;
        log_line("Culling trace enabled.");
    }
    if((++race_culling_frames%120)!=0)return;
    char line[220];
    snprintf(line,sizeof(line),"Culling trace frame %u manager=%p volumes=%u valid=%u forced=%u paused=%d",race_culling_frames,manager,race_culling_volume_count,race_culling_valid_count,race_culling_forced_count,paused?1:0);
    log_line(line);
}
