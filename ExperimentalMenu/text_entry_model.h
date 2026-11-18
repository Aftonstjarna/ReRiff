/* Single-line UTF-16 editor state. UI, clipboard and game input live outside it. */
#define TEXT_ENTRY_CAPACITY 2048
typedef struct TextEntryModel {WCHAR value[TEXT_ENTRY_CAPACITY];unsigned cursor,anchor,limit;} TextEntryModel;
static BOOL text_high(WCHAR c){return c>=0xd800&&c<=0xdbff;}
static BOOL text_low(WCHAR c){return c>=0xdc00&&c<=0xdfff;}
static unsigned text_before(const WCHAR *s,unsigned at){if(!at)return 0;at--;if(at&&text_low(s[at])&&text_high(s[at-1]))at--;return at;}
static unsigned text_after(const WCHAR *s,unsigned at){if(!s[at])return at;return at+(text_high(s[at])&&text_low(s[at+1])?2:1);}
static void text_model_init(TextEntryModel *m,const WCHAR *value,unsigned limit){memset(m,0,sizeof(*m));m->limit=limit<TEXT_ENTRY_CAPACITY?limit:TEXT_ENTRY_CAPACITY-1;unsigned n=0;while(value[n]&&n<m->limit)n++;if(n&&text_high(value[n-1]))n--;memcpy(m->value,value,n*2);m->cursor=m->anchor=n;}
static void text_model_remove(TextEntryModel *m,unsigned first,unsigned last){size_t length=wcslen(m->value);if(first>last||last>length)return;memmove(m->value+first,m->value+last,(length-last+1)*2);m->cursor=m->anchor=first;}
static void text_model_insert(TextEntryModel *m,const WCHAR *value,unsigned count){
    unsigned a=m->anchor<m->cursor?m->anchor:m->cursor,b=m->anchor>m->cursor?m->anchor:m->cursor;
    unsigned length=(unsigned)wcslen(m->value),room=m->limit-(length-(b-a)),n=0;
    while(n<count&&value[n]&&n<room){WCHAR c=value[n];if(c<32||c==127)break;if(text_high(c)){if(n+1>=count||n+1>=room||!text_low(value[n+1]))break;n+=2;}else if(text_low(c))break;else n++;}
    if(!n)return;memmove(m->value+a+n,m->value+b,(length-b+1)*2);memcpy(m->value+a,value,n*2);m->cursor=m->anchor=a+n;
}
static void text_model_key(TextEntryModel *m,unsigned key,BOOL shift){
    if(key!=VK_BACK&&key!=VK_DELETE&&key!=VK_HOME&&key!=VK_END&&key!=VK_LEFT&&key!=VK_RIGHT)return;
    unsigned length=(unsigned)wcslen(m->value),a=m->anchor<m->cursor?m->anchor:m->cursor,b=m->anchor>m->cursor?m->anchor:m->cursor;
    if(key==VK_BACK){if(a!=b)text_model_remove(m,a,b);else text_model_remove(m,text_before(m->value,m->cursor),m->cursor);return;}
    if(key==VK_DELETE){if(a!=b)text_model_remove(m,a,b);else text_model_remove(m,m->cursor,text_after(m->value,m->cursor));return;}
    if(key==VK_HOME)m->cursor=0;else if(key==VK_END)m->cursor=length;
    else if(key==VK_LEFT)m->cursor=!shift&&a!=b?a:text_before(m->value,m->cursor);
    else if(key==VK_RIGHT)m->cursor=!shift&&a!=b?b:text_after(m->value,m->cursor);
    if(!shift)m->anchor=m->cursor;
}

/* Bounded per-edit history. Cursor moves alone do not create undo steps. */
#define TEXT_ENTRY_HISTORY 32
typedef struct TextEntryHistory {TextEntryModel states[TEXT_ENTRY_HISTORY];unsigned count,at;} TextEntryHistory;
static void text_history_init(TextEntryHistory *h,const TextEntryModel *m){h->count=1;h->at=0;h->states[0]=*m;}
static void text_history_record(TextEntryHistory *h,const TextEntryModel *before,const TextEntryModel *after,BOOL merge){
    if(!wcscmp(before->value,after->value))return;
    if(merge&&h->at>0&&h->at+1==h->count){h->states[h->at]=*after;return;}
    h->states[h->at]=*before;h->count=h->at+1;
    if(h->count==TEXT_ENTRY_HISTORY){memmove(h->states,h->states+1,(TEXT_ENTRY_HISTORY-1)*sizeof(*h->states));h->count--;h->at--;}
    h->states[h->count++]=*after;h->at=h->count-1;
}
static BOOL text_history_step(TextEntryHistory *h,TextEntryModel *m,BOOL redo){
    if((redo&&h->at+1>=h->count)||(!redo&&!h->at))return FALSE;
    if(redo)h->at++;else h->at--;*m=h->states[h->at];return TRUE;
}
