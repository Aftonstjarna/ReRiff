/* Inspect GitHub metadata at object depth, never inside notes/assets. Tags and
 * package versions are opaque identifiers; publication time orders releases. */
static const char *release_space(const char *p){while(isspace((unsigned char)*p))p++;return p;}
static const char *release_hex4(const char *p,unsigned *value){
    *value=0;
    for(int i=0;i<4;i++){
        unsigned char c=(unsigned char)*p++;if(!isxdigit(c))return NULL;
        *value=*value*16+(c<='9'?c-'0':tolower(c)-'a'+10);
    }
    return p;
}
static const char *release_string(const char *p,char *out,size_t capacity){
    if(*p++!='"')return NULL;size_t n=0;
    while(*p&&*p!='"'){
        unsigned c=(unsigned char)*p++;char encoded[4];size_t bytes=1;
        if(c<32)return NULL;
        if(c=='\\'){
            c=(unsigned char)*p++;if(!c)return NULL;
            if(c=='u'){
                p=release_hex4(p,&c);if(!p)return NULL;
                if(c>=0xd800&&c<=0xdbff){
                    unsigned low;if(p[0]!='\\'||p[1]!='u')return NULL;
                    p=release_hex4(p+2,&low);if(!p||low<0xdc00||low>0xdfff)return NULL;
                    c=0x10000+((c-0xd800)<<10)+(low-0xdc00);
                }else if(c>=0xdc00&&c<=0xdfff)return NULL;
                if(c>=0x10000){encoded[0]=(char)(0xf0|(c>>18));encoded[1]=(char)(0x80|((c>>12)&63));encoded[2]=(char)(0x80|((c>>6)&63));encoded[3]=(char)(0x80|(c&63));bytes=4;}
                else if(c>=0x800){encoded[0]=(char)(0xe0|(c>>12));encoded[1]=(char)(0x80|((c>>6)&63));encoded[2]=(char)(0x80|(c&63));bytes=3;}
                else if(c>=0x80){encoded[0]=(char)(0xc0|(c>>6));encoded[1]=(char)(0x80|(c&63));bytes=2;}
            }else if(c=='b')c='\b';else if(c=='f')c='\f';else if(c=='n')c='\n';else if(c=='r')c='\r';else if(c=='t')c='\t';
            else if(!strchr("\"\\/",c))return NULL;
        }
        if(bytes==1)encoded[0]=(char)c;
        if(out){if(!c||n+bytes>=capacity)return NULL;memcpy(out+n,encoded,bytes);n+=bytes;}
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
    if(!strncmp(p,"true",4)||!strncmp(p,"null",4))return p+4;
    if(!strncmp(p,"false",5))return p+5;
    if(*p=='-')p++;
    if(*p=='0')p++;
    else{if(*p<'1'||*p>'9')return NULL;while(isdigit((unsigned char)*p))p++;}
    if(*p=='.'){p++;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))p++;}
    if(*p=='e'||*p=='E'){p++;if(*p=='+'||*p=='-')p++;if(!isdigit((unsigned char)*p))return NULL;while(isdigit((unsigned char)*p))p++;}
    return p;
}
typedef struct GithubRelease {char tag[256],published[21];BOOL experimental,draft,flags;} GithubRelease;
static BOOL release_timestamp(const char *text){
    if(strlen(text)!=20)return FALSE;
    for(int i=0;i<20;i++){
        char separator=i==4||i==7?'-':i==10?'T':i==13||i==16?':':i==19?'Z':0;
        if(separator?text[i]!=separator:!isdigit((unsigned char)text[i]))return FALSE;
    }
    return TRUE;
}
static const char *release_object(const char *p,GithubRelease *release){
    memset(release,0,sizeof(*release));p=release_space(p);if(*p++!='{')return NULL;
    BOOL have_pre=FALSE,have_draft=FALSE;char key[128];
    for(;;){
        p=release_space(p);if(*p=='}'){release->flags=have_pre&&have_draft;return p+1;}
        p=release_string(p,key,sizeof(key));if(!p)return NULL;p=release_space(p);if(*p++!=':')return NULL;p=release_space(p);
        if(!strcmp(key,"tag_name")){p=release_string(p,release->tag,sizeof(release->tag));if(!p)return NULL;}
        else if(!strcmp(key,"published_at")&&*p=='"'){p=release_string(p,release->published,sizeof(release->published));if(!p)return NULL;}
        else if(!strcmp(key,"prerelease")||!strcmp(key,"draft")){
            BOOL value;if(!strncmp(p,"true",4)){value=TRUE;p+=4;}else if(!strncmp(p,"false",5)){value=FALSE;p+=5;}else return NULL;
            if(!strcmp(key,"prerelease")){release->experimental=value;have_pre=TRUE;}else{release->draft=value;have_draft=TRUE;}
        }else{p=release_skip(p,0);if(!p)return NULL;}
        p=release_space(p);if(*p=='}'){release->flags=have_pre&&have_draft;return p+1;}if(*p++!=',')return NULL;
        if(*release_space(p)=='}')return NULL;
    }
}
/* Keep the best candidate across pages. FALSE means malformed JSON; a valid
 * page with no matching release leaves selected unchanged and returns TRUE. */
static BOOL select_github_release(const char *json,BOOL experimental,GithubRelease *selected,unsigned *count){
    const char *p=release_space(json);BOOL list=*p=='[';if(list)p=release_space(p+1);
    *count=0;
    while(*p&&(!list||*p!=']')){
        GithubRelease release;p=release_object(p,&release);if(!p)return FALSE;
        (*count)++;
        if(release.flags&&!release.draft&&release.experimental==experimental&&*release.tag&&release_timestamp(release.published)
           &&(!selected->flags||strcmp(release.published,selected->published)>0)){
            *selected=release;
        }
        p=release_space(p);if(!list)break;if(*p==']')break;if(*p++!=',')return FALSE;p=release_space(p);
        if(*p==']')return FALSE;
    }
    if(list){if(*p++!=']')return FALSE;p=release_space(p);}
    return !*p&&(*count||list);
}

/* Encode the entire tag as one URL component, including slash and percent.
 * It must never be interpreted as a path, query or fragment. */
static BOOL release_tag_path(const char *tag,char *out,size_t capacity){
    const char hex[]="0123456789ABCDEF";size_t used=0;
    if(!*tag||!strcmp(tag,".")||!strcmp(tag,".."))return FALSE;
    for(const unsigned char *p=(const unsigned char *)tag;*p;p++){
        BOOL plain=(*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||strchr("-._~",*p);
        size_t length=plain?1:3;if(used+length>=capacity)return FALSE;
        if(plain)out[used++]=(char)*p;
        else{out[used++]='%';out[used++]=hex[*p>>4];out[used++]=hex[*p&15];}
    }
    out[used]=0;return TRUE;
}
typedef struct ReleasePackage {char version[128],filename[256];} ReleasePackage;
static BOOL release_filename(const char *name){
    return *name&&name[0]!='.'&&strspn(name,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._")==strlen(name);
}
static BOOL release_package(const char *json,BOOL linux_package,ReleasePackage *package){
    memset(package,0,sizeof(*package));const char *p=release_space(json);if(*p++!='{')return FALSE;
    BOOL have_version=FALSE,have_artifacts=FALSE;char key[128];
    const char *suffix=linux_package?"-linux.run":"-windows.zip";
    for(;;){
        p=release_space(p);if(*p=='}')break;
        p=release_string(p,key,sizeof(key));if(!p)return FALSE;p=release_space(p);if(*p++!=':')return FALSE;p=release_space(p);
        if(!strcmp(key,"version")){
            if(have_version)return FALSE;have_version=TRUE;
            p=release_string(p,package->version,sizeof(package->version));if(!p)return FALSE;
            for(const unsigned char *v=(const unsigned char *)package->version;*v;v++)if(*v<32||*v==127)return FALSE;
        }else if(!strcmp(key,"artifacts")){
            if(have_artifacts||*p++!='[')return FALSE;have_artifacts=TRUE;p=release_space(p);
            while(*p!=']'){
                char name[256];p=release_string(p,name,sizeof(name));if(!p)return FALSE;
                size_t length=strlen(name),tail=strlen(suffix);
                if(length>tail&&!strcmp(name+length-tail,suffix)){
                    if(*package->filename||!release_filename(name))return FALSE;strcpy(package->filename,name);
                }
                p=release_space(p);if(*p==']')break;if(*p++!=',')return FALSE;p=release_space(p);
                if(*p==']')return FALSE;
            }
            p++;
        }else{p=release_skip(p,0);if(!p)return FALSE;}
        p=release_space(p);if(*p=='}')break;if(*p++!=',')return FALSE;
        if(*release_space(p)=='}')return FALSE;
    }
    return !*release_space(p+1)&&*package->version&&*package->filename;
}
static BOOL release_needs_install(const char *remote,const char *local,BOOL requested,BOOL installed,BOOL force){
    return force||requested!=installed||strcmp(remote,local)!=0;
}
