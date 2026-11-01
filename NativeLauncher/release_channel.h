/* Inspect GitHub's release flags at object depth, never inside notes/assets.
 * Only supported ReRiff version tags are eligible for installation. */
static const char *release_space(const char *p){while(isspace((unsigned char)*p))p++;return p;}
static const char *release_string(const char *p,char *out,size_t capacity){
    if(*p++!='"')return NULL;size_t n=0;
    while(*p&&*p!='"'){
        unsigned char c=(unsigned char)*p++;
        if(c<32)return NULL;
        if(c=='\\'){
            c=(unsigned char)*p++;if(!c)return NULL;
            if(c=='u'){for(int i=0;i<4;i++)if(!isxdigit((unsigned char)*p++))return NULL;c='?';}
            else if(!strchr("\"\\/bfnrt",c))return NULL;
        }
        if(out){if(n+1>=capacity)return NULL;out[n++]=(char)c;}
    }
    if(*p!='"')return NULL;if(out)out[n]=0;return p+1;
}
static const char *release_skip(const char *p,unsigned depth){
    if(depth>32)return NULL;p=release_space(p);
    if(*p=='"')return release_string(p,NULL,0);
    if(*p=='{'||*p=='['){
        char end=*p=='{'?'}':']';BOOL object=*p=='{';p=release_space(p+1);
        if(*p==end)return p+1;
        for(;;){
            if(object){p=release_string(p,NULL,0);if(!p)return NULL;p=release_space(p);if(*p++!=':')return NULL;}
            p=release_skip(p,depth+1);if(!p)return NULL;p=release_space(p);
            if(*p==end)return p+1;if(*p++!=',')return NULL;p=release_space(p);
        }
    }
    const char *start=p;while(*p&&!isspace((unsigned char)*p)&&!strchr(",]}",*p))p++;
    if(p==start)return NULL;return p;
}
typedef struct GithubRelease {char tag[80];BOOL experimental,draft,flags;} GithubRelease;
static const char *release_object(const char *p,GithubRelease *release){
    memset(release,0,sizeof(*release));p=release_space(p);if(*p++!='{')return NULL;
    BOOL have_pre=FALSE,have_draft=FALSE;char key[128];
    for(;;){
        p=release_space(p);if(*p=='}'){release->flags=have_pre&&have_draft;return p+1;}
        p=release_string(p,key,sizeof(key));if(!p)return NULL;p=release_space(p);if(*p++!=':')return NULL;p=release_space(p);
        if(!strcmp(key,"tag_name")){p=release_string(p,release->tag,sizeof(release->tag));if(!p)return NULL;}
        else if(!strcmp(key,"prerelease")||!strcmp(key,"draft")){
            BOOL value;if(!strncmp(p,"true",4)){value=TRUE;p+=4;}else if(!strncmp(p,"false",5)){value=FALSE;p+=5;}else return NULL;
            if(!strcmp(key,"prerelease")){release->experimental=value;have_pre=TRUE;}else{release->draft=value;have_draft=TRUE;}
        }else{p=release_skip(p,0);if(!p)return NULL;}
        p=release_space(p);if(*p=='}'){release->flags=have_pre&&have_draft;return p+1;}if(*p++!=',')return NULL;
    }
}
static BOOL select_github_release(const char *json,BOOL experimental,char version[64],char tag[80]){
    const char *p=release_space(json);BOOL list=*p=='[';if(list)p=release_space(p+1);
    BOOL found=FALSE;int best[4]={0};
    while(*p&&(!list||*p!=']')){
        GithubRelease release;p=release_object(p,&release);if(!p)return FALSE;
        const char *number=release.tag[0]=='v'?release.tag+1:release.tag;int parsed[4];
        if(release.flags&&!release.draft&&release.experimental==experimental&&strlen(number)<64&&parse_update_version(number,parsed)&&(!found||update_version_newer(parsed,best))){
            strcpy(version,number);strcpy(tag,release.tag);memcpy(best,parsed,sizeof(best));found=TRUE;
        }
        p=release_space(p);if(!list)break;if(*p==']')break;if(*p++!=',')return FALSE;p=release_space(p);
    }
    if(list){if(*p++!=']')return FALSE;p=release_space(p);}
    return found&&!*p;
}
