/* Original CRT common startup VA 0x008F8A8E..0x008F8C42.
 * Contains its local SEH filter and handler. External helpers, scope table,
 * imports and application target retain original addresses, not replacements.
 */
.intel_syntax noprefix
.code32
.section .text
.globl original_crt_startup_common
.type original_crt_startup_common, @function
.equ seh_prolog, 0x008F9C90
.equ seh_epilog, 0x008F9CD5
.equ runtime_error, 0x008F9A1C
.equ initialize_c, 0x008F9C7C
.equ initialize_cpp, 0x008F9C82
.equ validate_callback, 0x008F9A80
.equ exception_filter, 0x008F9A16
.equ application_entry, 0x005CFC90
.equ startup_lock, 0x00C0E618
.equ startup_state, 0x00C0E61C
.equ already_initialized, 0x00BFB138
.equ application_result, 0x00BFB13C
.equ return_to_runtime, 0x00BFB140
original_crt_startup_common:
    push 0x14
    push 0x00B9E5C0
    call seh_prolog
    {load} xor esi, esi
    mov DWORD PTR [ebp-0x1c], esi
    mov DWORD PTR [ebp-0x20], esi
    call DWORD PTR [0x0092D2B0]
    movzx ebx, ax
    mov DWORD PTR [ebp-4], esi
    mov eax, DWORD PTR fs:[0x18]
    mov edx, DWORD PTR [eax+4]
    {load} mov edi, esi
    mov esi, startup_lock
.Lacquire:
    {load} mov ecx, edx
    {load} xor eax, eax
    lock cmpxchg DWORD PTR [esi], ecx
    test eax, eax
    je .Lacquired
    {load} cmp eax, edx
    jne .Lacquire
    {load} xor esi, esi
    inc esi
    {load} mov edi, esi
    jmp .Llocked
.Lacquired:
    {load} xor esi, esi
    inc esi
.Llocked:
    cmp DWORD PTR [startup_state], esi
    jne .Lcheck_state
    push 0x1f
    call runtime_error
    pop ecx
    jmp .Lcpp
.Lcheck_state:
    cmp DWORD PTR [startup_state], 0
    jne .Lwas_initialized
    mov DWORD PTR [startup_state], esi
    push 0x0092D82C
    push 0x0092D818
    call initialize_c
    pop ecx
    pop ecx
    test eax, eax
    je .Lcpp
    mov DWORD PTR [ebp-4], -2
    mov eax, 0xff
    jmp .Lreturn
.Lwas_initialized:
    mov DWORD PTR [already_initialized], esi
.Lcpp:
    cmp DWORD PTR [startup_state], esi
    jne .Lunlock
    push 0x0092D814
    push 0x0092D6A4
    call initialize_cpp
    pop ecx
    pop ecx
    mov DWORD PTR [startup_state], 2
.Lunlock:
    test edi, edi
    jne .Lcallback
    {load} xor eax, eax
    mov ecx, startup_lock
    xchg DWORD PTR [ecx], eax
.Lcallback:
    cmp DWORD PTR [0x00C0E628], 0
    je .Lcommand_line
    push 0x00C0E628
    call validate_callback
    pop ecx
    test eax, eax
    je .Lcommand_line
    push 0
    push 2
    push 0
    call DWORD PTR [0x00C0E628]
.Lcommand_line:
    mov eax, DWORD PTR [0x0092D2D8]
    mov esi, DWORD PTR [eax]
    mov DWORD PTR [ebp-0x1c], esi
    mov edi, DWORD PTR [ebp-0x20]
.Lscan:
    mov cl, BYTE PTR [esi]
    cmp cl, 0x20
    ja .Ladvance
    test cl, cl
    je .Lwhitespace
    test edi, edi
    jne .Ladvance
.Lwhitespace:
    mov al, BYTE PTR [esi]
    test al, al
    je .Ldispatch
    cmp al, 0x20
    ja .Ldispatch
    inc esi
    mov DWORD PTR [ebp-0x1c], esi
    jmp .Lwhitespace
.Ldispatch:
    push ebx
    push esi
    push 0
    push 0x00400000
    call application_entry
    mov DWORD PTR [application_result], eax
    cmp DWORD PTR [return_to_runtime], 0
    jne .Lcleanup
    push eax
    call DWORD PTR [0x0092D4AC]
.Ladvance:
    cmp cl, 0x22
    jne .Lmultibyte
    {load} xor eax, eax
    test edi, edi
    sete al
    {load} mov edi, eax
    mov DWORD PTR [ebp-0x20], edi
.Lmultibyte:
    movzx eax, cl
    push eax
    call DWORD PTR [0x0092D2C8]
    pop ecx
    test eax, eax
    je .Lnext
    inc esi
    mov DWORD PTR [ebp-0x1c], esi
.Lnext:
    inc esi
    mov DWORD PTR [ebp-0x1c], esi
    jmp .Lscan
.Lfilter:
    mov ecx, DWORD PTR [ebp-0x14]
    mov eax, DWORD PTR [ecx]
    mov eax, DWORD PTR [eax]
    mov DWORD PTR [ebp-0x24], eax
    push ecx
    push eax
    call exception_filter
    pop ecx
    pop ecx
    ret
.Lhandler:
    mov esp, DWORD PTR [ebp-0x18]
    mov eax, DWORD PTR [ebp-0x24]
    mov DWORD PTR [application_result], eax
    cmp DWORD PTR [return_to_runtime], 0
    jne .Lcleanup
    push eax
    call DWORD PTR [0x0092D2C0]
.Lcleanup:
    cmp DWORD PTR [already_initialized], 0
    jne .Lfinish
    call DWORD PTR [0x0092D2C4]
    mov eax, DWORD PTR [application_result]
.Lfinish:
    mov DWORD PTR [ebp-4], -2
.Lreturn:
    call seh_epilog
    ret
.size original_crt_startup_common, .-original_crt_startup_common
