/* Original PE entrypoint, RVA 0x004FD6D0, supported Game.exe only.
 * The import table resolves VA 0x0092D690 to mscoree!_CorExeMain.
 * Preserve the original fixed address for byte comparison; this is not
 * a standalone loader or a replacement implementation of the CLR startup.
 */
.code32
.section .text
.globl original_exe_entry
.type original_exe_entry, @function
.equ original_cor_exe_main_iat, 0x0092D690
original_exe_entry:
    jmp *original_cor_exe_main_iat
.size original_exe_entry, .-original_exe_entry
