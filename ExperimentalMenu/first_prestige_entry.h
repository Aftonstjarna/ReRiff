/* Intentional SP-only extension of original Profile.setupAchievementsList.
 * Original body 060013EA/RVA325BBC; exact skip at body+1B3. Native reel,
 * row construction, ownership, locked appearance and actions remain original.
 * This process selects its isolated save domain before Profile is JITted. */
static BOOL first_prestige_entry_installed;
static const BYTE first_prestige_original_skip[]={0x07,0x1f,0x12,0x3b,0x1d,0x05,0,0};
static BOOL first_prestige_skip_valid(BYTE *base){
    BYTE bytes[8];if(!progression_probe_read(base+0x325d7b,bytes,sizeof(bytes)))return FALSE;
    if(!memcmp(bytes,first_prestige_original_skip,8))return TRUE;
    static const BYTE nops[8]={0};
    return first_prestige_entry_installed&&!memcmp(bytes,nops,8);
}
static BOOL first_prestige_entry_install(BYTE *base){
    if(first_prestige_entry_installed)return first_prestige_skip_valid(base);
    BYTE bytes[8];if(!progression_probe_read(base+0x325d7b,bytes,8)||memcmp(bytes,first_prestige_original_skip,8))return FALSE;
    DWORD old,ignored;if(!VirtualProtect(base+0x325d7b,8,PAGE_READWRITE,&old))return FALSE;
    /* Eight CIL nops preserve every branch target and EH offset. This is not
     * x86 instruction patching. No original executable is edited on disk. */
    memset(base+0x325d7b,0,8);first_prestige_entry_installed=TRUE;
    if(!VirtualProtect(base+0x325d7b,8,old,&ignored))return FALSE;
    log_line("First Prestige native achievement row enabled for isolated Singleplayer; original 38-achievement eligibility retained.");return TRUE;
}
