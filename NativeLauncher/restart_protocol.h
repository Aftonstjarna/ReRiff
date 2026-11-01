/* Named events are private to the current Windows session and game PID. */
static void restart_event_name(WCHAR *name,size_t capacity,DWORD pid,BOOL ready) {
    _snwprintf(name,capacity,L"Local\\ReRiffGameRestart%ls-%lu",ready?L"Ready":L"",(unsigned long)pid);
    name[capacity-1]=0;
}
