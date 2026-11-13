/* Read-only update diagnostics. Viewing retained files never enables logging. */
#define UPDATE_DETAILS_CAP 65536
static WCHAR update_problem[512],update_work[CAP],update_failure[2048];
static HWND update_details_window;
static void update_details_append(WCHAR *out,const WCHAR *value){
    size_t used=wcslen(out),left=UPDATE_DETAILS_CAP-used-1;if(left)wcsncat(out,value,left);
}
static BOOL update_details_file(WCHAR *out,const WCHAR *name,const WCHAR *label){
    HANDLE file=CreateFileW(name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)return FALSE;
    LARGE_INTEGER size,offset;BYTE bytes[16386];DWORD count=0;
    BOOL ok=ReadFile(file,bytes,2,&count,NULL);BOOL wide=ok&&count==2&&bytes[0]==255&&bytes[1]==254;
    ok=GetFileSizeEx(file,&size);offset.QuadPart=ok&&size.QuadPart>16384?size.QuadPart-16384:0;
    if(offset.QuadPart&1)offset.QuadPart--;if(ok)ok=SetFilePointerEx(file,offset,NULL,FILE_BEGIN);
    ok=ok&&ReadFile(file,bytes,16384,&count,NULL);CloseHandle(file);if(!ok||!count)return FALSE;
    bytes[count]=bytes[count+1]=0;WCHAR text[16386];
    if(wide){unsigned skip=offset.QuadPart?0:2;memcpy(text,bytes+skip,(count-skip)&~1u);text[(count-skip)/2]=0;}
    else{unsigned skip=!offset.QuadPart&&count>=3&&!memcmp(bytes,"\xef\xbb\xbf",3)?3:0;
        int n=MultiByteToWideChar(CP_UTF8,0,(char *)bytes+skip,count-skip,text,16385);if(n<=0)return FALSE;text[n]=0;}
    update_details_append(out,L"\r\n\r\n");update_details_append(out,label);
    if(offset.QuadPart)update_details_append(out,L" (last 16 KiB)");update_details_append(out,L"\r\n");
    size_t used=wcslen(out);
    for(WCHAR *p=text;*p&&used+2<UPDATE_DETAILS_CAP;p++){
        if(*p==L'\n'&&(p==text||p[-1]!=L'\r'))out[used++]=L'\r';
        if(*p>=32||*p==L'\r'||*p==L'\n'||*p==L'\t')out[used++]=*p;
    }out[used]=0;
    return TRUE;
}
static void update_remember(const WCHAR *work){
    if(!*work)return;WCHAR file[CAP];path(file,L"localserver\\update-details.ini");
    WritePrivateProfileStringW(L"update",L"work",work,file);
}
static void update_details_text(WCHAR *out){
    if(!*update_work&&!*update_failure){WCHAR saved[CAP];path(saved,L"localserver\\update-details.ini");GetPrivateProfileStringW(L"update",L"work",L"",update_work,CAP,saved);}
    out[0]=0;update_details_append(out,*update_failure?update_failure:(*update_work?L"Latest retained update attempt.":L"No update failure has been recorded in this launcher session."));
    WCHAR file[CAP];
    if(*update_work){
        update_details_append(out,L"\r\n\r\nTemporary update folder: ");update_details_append(out,update_work);
        _snwprintf(file,CAP,L"%ls\\failure",update_work);file[CAP-1]=0;update_details_file(out,file,L"Installer error");
        _snwprintf(file,CAP,L"%ls\\update.log",update_work);file[CAP-1]=0;update_details_file(out,file,L"Temporary update log");
    }
    path(file,L"logs\\update.log");update_details_file(out,file,L"Saved update log (may include earlier attempts)");
}
static LRESULT CALLBACK update_details_proc(HWND hwnd,UINT event,WPARAM wp,LPARAM lp){
    if(event==WM_SIZE){MoveWindow(GetDlgItem(hwnd,1),12,12,LOWORD(lp)>24?LOWORD(lp)-24:1,HIWORD(lp)>24?HIWORD(lp)-24:1,TRUE);return 0;}
    if(event==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(event==WM_DESTROY){update_details_window=NULL;return 0;}
    return DefWindowProcW(hwnd,event,wp,lp);
}
static void show_update_details(void){
    if(!update_details_window){
        WNDCLASSW cls={0};cls.lpfnWndProc=update_details_proc;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"ReRiffUpdateDetails";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&cls);
        update_details_window=CreateWindowExW(0,cls.lpszClassName,L"ReRiff — Update details",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,760,520,window,NULL,cls.hInstance,NULL);
        if(!update_details_window)return;
        HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,12,12,710,440,update_details_window,(HMENU)1,cls.hInstance,NULL);
        SendMessageW(edit,EM_SETLIMITTEXT,UPDATE_DETAILS_CAP,0);SendMessageW(edit,WM_SETFONT,(WPARAM)font,TRUE);
    }
    WCHAR *text=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,UPDATE_DETAILS_CAP*sizeof(WCHAR));if(text){update_details_text(text);SetWindowTextW(GetDlgItem(update_details_window,1),text);HeapFree(GetProcessHeap(),0,text);}
    ShowWindow(update_details_window,SW_SHOWNORMAL);SetForegroundWindow(update_details_window);
}
