/* Scoped startup CIL patch: retain Last.fm JSON as UTF-8 using the game's
   existing std::string copy. Original disk image and shared codec unchanged.
   Evidence, tokens and startup/JIT constraints: UNICODE-METADATA-TRANSPORT.md. */
static BOOL lastfm_utf8_installed;
static const BYTE lastfm_utf8_header[]={0x0b,0x30,0x08,0x00,0x22,0x06,0,0,0x4a,0x14,0,0x11};
static const BYTE lastfm_utf8_original[]={
    0x1f,0x10,0x12,0x30,0x1f,0x14,0x58,0x4a,0x36,0x04,0x12,0x30,0x2b,0x03,0x12,0x30,
    0x4a,0x12,0x30,0x1f,0x10,0x58,0x4a,0x12,0x2f,0x28,0x02,0x18,0,0x06,0x26
};
static const BYTE lastfm_utf8_copy[]={0x12,0x2f,0x12,0x30,0x28,0x31,0x06,0,0x06,0x26};
static BOOL install_lastfm_utf8(BYTE *base){
    if(lastfm_utf8_installed)return TRUE;
    if(!cjk_component_enabled())return TRUE;
    BYTE *header=base+0x34fe9c,*site=header+12+0xa6;
    if(!readable(header,sizeof(lastfm_utf8_header))||memcmp(header,lastfm_utf8_header,sizeof(lastfm_utf8_header))
        ||!readable(site,sizeof(lastfm_utf8_original))||memcmp(site,lastfm_utf8_original,sizeof(lastfm_utf8_original)))return FALSE;
    /* Same body size, local signatures, branch targets and EH ranges.
       IL_A6: ldloca.s 47; ldloca.s 48; call 06000631; pop; nops to C5.
       Both strings are already constructed; original cleanup still owns both. */
    DWORD protection,ignored;if(!VirtualProtect(site,sizeof(lastfm_utf8_original),PAGE_EXECUTE_READWRITE,&protection))return FALSE;
    memset(site,0,sizeof(lastfm_utf8_original));memcpy(site,lastfm_utf8_copy,sizeof(lastfm_utf8_copy));
    FlushInstructionCache(GetCurrentProcess(),site,sizeof(lastfm_utf8_original));
    VirtualProtect(site,sizeof(lastfm_utf8_original),protection,&ignored);
    lastfm_utf8_installed=TRUE;log_line("Last.fm UTF-8 response copy installed before track-screen JIT (original string copy/cleanup).");return TRUE;
}
