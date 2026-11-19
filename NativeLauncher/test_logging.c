#define wWinMain launcher_main
#include "launcher.c"
#undef wWinMain
#include <assert.h>
#undef assert
#define assert(c) do { if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);exit(1);} } while(0)
int main(void){
 WCHAR previous[CAP],temp[CAP],root[CAP],logs[CAP],file[CAP],marker[CAP];GetCurrentDirectoryW(CAP,previous);GetTempPathW(CAP,temp);GetTempFileNameW(temp,L"rrl",0,root);DeleteFileW(root);assert(CreateDirectoryW(root,NULL));assert(SetCurrentDirectoryW(root));
 wcscpy(folder,root);path(ini,L"server.ini");path(logs,L"logs");path(file,L"logs\\ReRiff-launcher.log");path(marker,L"localserver\\client-channel.ini");CreateDirectoryW(L"localserver",NULL);WritePrivateProfileStringW(L"client",L"channel",L"stable",marker);load_channel();
 log_message(L"disabled");assert(GetFileAttributesW(logs)==INVALID_FILE_ATTRIBUTES);
 WNDCLASSW cls={0};cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleW(NULL);cls.lpszClassName=L"LoggingTest";RegisterClassW(&cls);busy=TRUE;
 HWND hwnd=CreateWindowW(cls.lpszClassName,L"",WS_OVERLAPPED,0,0,660,615,NULL,NULL,cls.hInstance,NULL);assert(hwnd);
 assert(SendMessageW(logging_checkbox,BM_GETCHECK,0,0)==BST_UNCHECKED);assert(SendMessageW(launcher_tabs,TCM_GETITEMCOUNT,0,0)==PAGE_COUNT);
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);assert(rr_logging_ini(ini));log_message(L"enabled");assert(GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES);
 WIN32_FILE_ATTRIBUTE_DATA before,after;assert(GetFileAttributesExW(file,GetFileExInfoStandard,&before));
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);assert(!rr_logging_ini(ini));log_message(L"must not append");assert(GetFileAttributesExW(file,GetFileExInfoStandard,&after));assert(before.nFileSizeLow==after.nFileSizeLow&&GetFileAttributesW(logs)!=INVALID_FILE_ATTRIBUTES);
 assert(IsWindowEnabled(log_checkboxes[0])==FALSE);
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);
 assert(IsWindowEnabled(log_checkboxes[0]));
 SendMessageW(log_checkboxes[0],BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOG_CATEGORY_BASE,0);
 assert(!rr_log_enabled(folder,L"launcher",FALSE));assert(rr_log_enabled(folder,L"unicode",FALSE));
 log_message(L"category disabled");assert(GetFileAttributesExW(file,GetFileExInfoStandard,&after));assert(before.nFileSizeLow==after.nFileSizeLow);
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);
 SendMessageW(logging_checkbox,BM_SETCHECK,BST_CHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOGGING,0);
 assert(SendMessageW(log_checkboxes[0],BM_GETCHECK,0,0)==BST_UNCHECKED);
 assert(LOG_CATEGORY_COUNT==9&&diagnostic_count==LOG_CATEGORY_COUNT+4);
 HWND uninstall=GetDlgItem(hwnd,UNINSTALL);assert(uninstall);
 for(int page=0;page<PAGE_COUNT;page++){
  SendMessageW(launcher_tabs,TCM_SETCURSEL,page,0);select_launcher_page();
  assert(((GetWindowLongW(uninstall,GWL_STYLE)&WS_VISIBLE)!=0)==(page==PAGE_DIAGNOSTICS));
 }
 RECT uninstall_rect,details_rect;GetWindowRect(uninstall,&uninstall_rect);GetWindowRect(GetDlgItem(hwnd,UPDATE_DETAILS),&details_rect);
 assert(uninstall_rect.left>=details_rect.right&&uninstall_rect.top==details_rect.top);
 SendMessageW(launcher_tabs,TCM_SETCURSEL,PAGE_DIAGNOSTICS,0);select_launcher_page();
 WCHAR help[1024]={0};TOOLINFOW tool={0};tool.cbSize=TTTOOLINFOW_V2_SIZE;tool.lpszText=help;tool.hwnd=hwnd;tool.uId=(UINT_PTR)logging_checkbox;
 assert(SendMessageW(checkbox_tooltips,TTM_GETTOOLINFOW,0,(LPARAM)&tool));
 assert((tool.uFlags&(TTF_IDISHWND|TTF_SUBCLASS))==(TTF_IDISHWND|TTF_SUBCLASS));
 assert(wcsstr(tool.lpszText,L"Existing logs")||wcsstr(tool.lpszText,L"existing logs"));
 assert(WritePrivateProfileStringW(L"diagnostics",L"log_network",L"false",ini));
 assert(!rr_log_selected(folder,L"network")&&rr_log_selected(folder,L"connections"));
 SendMessageW(log_checkboxes[1],BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOG_CATEGORY_BASE+1,0);
 assert(!rr_log_selected(folder,L"client")&&!rr_log_selected(folder,L"network")&&!rr_log_selected(folder,L"news"));
 SendMessageW(log_checkboxes[1],BM_SETCHECK,BST_CHECKED,0);SendMessageW(hwnd,WM_COMMAND,LOG_CATEGORY_BASE+1,0);
 assert(rr_log_selected(folder,L"network"));

 DestroyWindow(hwnd);DeleteFileW(file);RemoveDirectoryW(logs);DeleteFileW(ini);DeleteFileW(marker);RemoveDirectoryW(L"localserver");SetCurrentDirectoryW(previous);RemoveDirectoryW(root);
 puts("PASS: native Diagnostics toggle, no disabled log directory, enabled writes and preserved disabled logs");return 0;
}
