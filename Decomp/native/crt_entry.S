/* _WinMainCRTStartup, original VA 0x008F8A84: call initialization,
 * then tail-transfer to the CRT startup implementation at 0x008F8A8E.
 * External symbols are resolved at original addresses by the linker.
 */
.code32
.section .text
.globl original_crt_entry
.type original_crt_entry, @function
.extern original_security_cookie_init
.extern original_crt_startup_common
original_crt_entry:
    call original_security_cookie_init
    jmp original_crt_startup_common
.size original_crt_entry, .-original_crt_entry
