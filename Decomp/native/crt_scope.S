/* Original startup SEH scope table, VA 0xB9E5C0..0xB9E5DC.
 * Four cookie-layout fields followed by one enclosing/filter/handler record.
 * Filter and handler addresses are inside the rebuilt crt_startup.S block.
 */
.section .rodata
.globl original_crt_scope
.type original_crt_scope, @object
original_crt_scope:
    .long -2, 0, -52, 0
    .long -2
    .long 0x008F8BF2
    .long 0x008F8C06
.size original_crt_scope, .-original_crt_scope
