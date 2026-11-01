/* Game.exe VA 0x008F9B78..0x008F9C14: complete cookie initializer.
 * Fixed data/IAT addresses belong to the supported original image.
 * {load} preserves the original register-register instruction encoding.
 */
.intel_syntax noprefix
.code32
.section .text
.globl original_security_cookie_init
.type original_security_cookie_init, @function
.equ security_cookie, 0x00BAABC0
.equ security_cookie_complement, 0x00BAABC4
.equ iat_GetSystemTimeAsFileTime, 0x0092D040
.equ iat_GetCurrentThreadId, 0x0092D0EC
.equ iat_GetCurrentProcessId, 0x0092D044
.equ iat_QueryPerformanceCounter, 0x0092D0F4
original_security_cookie_init:
    push ebp
    {load} mov ebp, esp
    sub esp, 0x14
    and DWORD PTR [ebp-0x0c], 0
    and DWORD PTR [ebp-0x08], 0
    mov eax, DWORD PTR [security_cookie]
    push esi
    push edi
    mov edi, 0xbb40e64e
    mov esi, 0xffff0000
    {load} cmp eax, edi
    je .Lgenerate
    test esi, eax
    je .Lgenerate
    not eax
    mov DWORD PTR [security_cookie_complement], eax
    jmp .Ldone
.Lgenerate:
    lea eax, [ebp-0x0c]
    push eax
    call DWORD PTR [iat_GetSystemTimeAsFileTime]
    mov eax, DWORD PTR [ebp-0x08]
    xor eax, DWORD PTR [ebp-0x0c]
    mov DWORD PTR [ebp-0x04], eax
    call DWORD PTR [iat_GetCurrentThreadId]
    xor DWORD PTR [ebp-0x04], eax
    call DWORD PTR [iat_GetCurrentProcessId]
    xor DWORD PTR [ebp-0x04], eax
    lea eax, [ebp-0x14]
    push eax
    call DWORD PTR [iat_QueryPerformanceCounter]
    mov ecx, DWORD PTR [ebp-0x10]
    lea eax, [ebp-0x04]
    xor ecx, DWORD PTR [ebp-0x14]
    xor ecx, DWORD PTR [ebp-0x04]
    {load} xor ecx, eax
    {load} cmp ecx, edi
    jne .Lcheck_high_word
    mov ecx, 0xbb40e64f
    jmp .Lstore
.Lcheck_high_word:
    test esi, ecx
    jne .Lstore
    {load} mov eax, ecx
    or eax, 0x4711
    shl eax, 0x10
    {load} or ecx, eax
.Lstore:
    mov DWORD PTR [security_cookie], ecx
    not ecx
    mov DWORD PTR [security_cookie_complement], ecx
.Ldone:
    pop edi
    pop esi
    {load} mov esp, ebp
    pop ebp
    ret
.size original_security_cookie_init, .-original_security_cookie_init
