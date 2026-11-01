.intel_syntax noprefix
.code32
.text
.global original_cookie_check
.equ failure,0x8f956b
original_cookie_check:
 cmp ecx,DWORD PTR [0xbaabc0]
 jne .failure
 repz ret
.failure:
 jmp failure
