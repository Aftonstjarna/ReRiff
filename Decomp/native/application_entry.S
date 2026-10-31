/* Original application entry VA 0x005CFC90..0x005D002A.
 * Calls retain original addresses. Helper internals remain separate recovery.
 * {load}/{disp32} preserve the original encoding choices, including loop padding.
 */
.intel_syntax noprefix
.code32
.section .text
.globl original_application_entry
.type original_application_entry, @function
.equ check_cookie, 0x008F8958
.equ frequency_to_double, 0x008F92EF
.equ fn_452d30, 0x00452D30
.equ fn_415b00, 0x00415B00
.equ fn_5c2740, 0x005C2740
.equ fn_44ebe0, 0x0044EBE0
.equ fn_48ce90, 0x0048CE90
.equ fn_5cfbe0, 0x005CFBE0
.equ fn_560310, 0x00560310
.equ fn_44f9b0, 0x0044F9B0
.equ fn_44fb30, 0x0044FB30
.equ fn_6d86bb, 0x006D86BB
.equ fn_44f490, 0x0044F490
.equ fn_5cfc60, 0x005CFC60
.equ fn_535ec0, 0x00535EC0
.equ fn_510f30, 0x00510F30
.equ fn_5cfb60, 0x005CFB60
.equ fn_4a0480, 0x004A0480
.equ fn_5cfb10, 0x005CFB10
.equ fn_48ce10, 0x0048CE10
.equ fn_4a0af0, 0x004A0AF0
.equ fn_4a08e0, 0x004A08E0
original_application_entry:
    push ebp
    {load} mov ebp, esp
    and esp, -8
    sub esp, 0x54
    mov eax, DWORD PTR [0x00BAABC0]
    {load} xor eax, esp
    mov DWORD PTR [esp+0x50], eax
    push ebx
    mov ebx, DWORD PTR [ebp+8]
    push esi
    mov esi, DWORD PTR [ebp+0x10]
    push edi
    push 0x00A062FC
    push 1
    push 0
    call DWORD PTR [0x0092D034] /* CreateMutexA: OCTANE */
    mov DWORD PTR [0x00C0D610], eax
    call DWORD PTR [0x0092D108] /* GetLastError */
    cmp eax, 0xb7
    jne .Lfirst_instance
    push DWORD PTR [0x00C0D610]
    call DWORD PTR [0x0092D0D4] /* CloseHandle */
    mov eax, -2
    pop edi
    pop esi
    pop ebx
    mov ecx, DWORD PTR [esp+0x50]
    {load} xor ecx, esp
    call check_cookie
    {load} mov esp, ebp
    pop ebp
    ret 0x10
.Lfirst_instance:
    push 0x00C00B48
    call DWORD PTR [0x0092D0F0] /* QueryPerformanceFrequency */
    test eax, eax
    je .Ltimer_failure
    mov edx, DWORD PTR [0x00C00B4C]
    mov ecx, DWORD PTR [0x00C00B48]
    call frequency_to_double
    divsd xmm0, QWORD PTR [0x00A06760]
    movsd QWORD PTR [0x00C00B50], xmm0
    call DWORD PTR [0x0092D0EC] /* GetCurrentThreadId */
    push 0
    push 0
    mov DWORD PTR [0x00BFCC3C], eax
    call DWORD PTR [0x0092D698] /* CoInitializeEx */
    cmp BYTE PTR [0x00C0D428], 0
    jne .Lcommand_string
    mov BYTE PTR [0x00C0D428], 1
    call fn_452d30
.Lcommand_string:
    push esi
    lea ecx, [esp+0x18]
    call fn_415b00
    mov edx, DWORD PTR [esp+0x28]
    lea ecx, [esp+0x14]
    mov esi, DWORD PTR [esp+0x14]
    lea edi, [esp+0x14]
    mov eax, DWORD PTR [esp+0x24]
    cmp edx, 0x10
    cmovae ecx, esi
    cmovae edi, esi
    {load} add eax, ecx
    cmp edx, 0x10
    lea ecx, [esp+0x14]
    cmovae ecx, esi
    {load} cmp ecx, eax
    je .Lempty_string
    push DWORD PTR [0x0092D4D4]
    {load} mov edx, ecx
    lea ecx, [esp+0x10]
    push edi
    push eax
    call fn_5c2740
    add esp, 0x0c
    jmp .Lservices
.Lempty_string:
    mov DWORD PTR [esp+0x0c], edi
.Lservices:
    call fn_44ebe0
    cmp DWORD PTR [0x00BFCC2C], 0
    jne .Lcheck_stage
    push 0x148
    call DWORD PTR [0x0092D4B4]
    add esp, 4
    test eax, eax
    je .Lallocation_failed
    {load} mov ecx, eax
    call fn_48ce90
    jmp .Lstore_service
.Lallocation_failed:
    {load} xor eax, eax
.Lstore_service:
    mov DWORD PTR [0x00BFCC2C], eax
.Lcheck_stage:
    call fn_5cfbe0
    mov esi, DWORD PTR [0x00BFCD2C]
    {load} mov ecx, esi
    call fn_560310
    mov edi, DWORD PTR [0x0092D4B0]
    test al, al
    jne .Lstage_ready
    push 0x1d
    push 0x00A008CC
    lea ecx, [esi+0x30]
    call fn_44f9b0
    mov eax, DWORD PTR [0x00BFCD2C]
    {load} xor ecx, ecx
    push -1
    push ecx
    add eax, 0x30
    mov WORD PTR [esp+0x4c], cx
    push eax
    lea ecx, [esp+0x50]
    mov DWORD PTR [esp+0x64], 7
    mov DWORD PTR [esp+0x60], 0
    call fn_44fb30
    call fn_6d86bb
    push 1
    {load} mov ecx, eax
    mov edx, DWORD PTR [eax]
    call DWORD PTR [edx+0x0c]
    {load} mov edx, eax
    lea ecx, [esp+0x2c]
    call fn_44f490
    cmp DWORD PTR [esp+0x40], 8
    lea ecx, [esp+0x2c]
    push 0x10
    cmovae ecx, DWORD PTR [esp+0x30]
    lea eax, [esp+0x48]
    cmp DWORD PTR [esp+0x5c], 8
    push ecx
    cmovae eax, DWORD PTR [esp+0x4c]
    push eax
    push 0
    call DWORD PTR [0x0092D594] /* MessageBoxW */
    cmp DWORD PTR [esp+0x40], 8
    jb .Lfree_second
    push DWORD PTR [esp+0x2c]
    call edi
    add esp, 4
.Lfree_second:
    {load} xor eax, eax
    mov DWORD PTR [esp+0x40], 7
    cmp DWORD PTR [esp+0x58], 8
    mov DWORD PTR [esp+0x3c], 0
    mov WORD PTR [esp+0x2c], ax
    jb .Lfailed
    push DWORD PTR [esp+0x44]
    call edi
    add esp, 4
.Lfailed:
    or esi, -1
.Lcleanup_string:
    cmp DWORD PTR [esp+0x28], 0x10
    jb .Lreturn
    push DWORD PTR [esp+0x14]
    call edi
    add esp, 4
.Lreturn:
    {load} mov eax, esi
    pop edi
    pop esi
    pop ebx
    mov ecx, DWORD PTR [esp+0x50]
    {load} xor ecx, esp
    call check_cookie
    {load} mov esp, ebp
    pop ebp
    ret 0x10
.Lstage_ready:
    cmp DWORD PTR [esi+0x2c], 8
    lea eax, [esi+0x18]
    jb .Ldll_directory
    mov eax, DWORD PTR [eax]
.Ldll_directory:
    push eax
    call DWORD PTR [0x0092D074] /* SetDllDirectoryW */
    call fn_5cfc60
    mov ecx, DWORD PTR [0x00BFCD08]
    call fn_535ec0
    call fn_510f30
    test al, al
    je .Lfailed
    call fn_6d86bb
    {load} mov ecx, eax
    mov edx, DWORD PTR [eax]
    call DWORD PTR [edx]
    call fn_5cfb60
    cmp DWORD PTR [esp+0x28], 0x10
    lea eax, [esp+0x14]
    push DWORD PTR [ebp+0x14]
    cmovae eax, DWORD PTR [esp+0x18]
    mov esi, DWORD PTR [0x00BFCC44]
    {load} mov ecx, esi
    push eax
    push ebx
    mov DWORD PTR [esp+0x1c], esi
    call fn_4a0480
    test al, al
    jne .Lstart_game
    mov ecx, DWORD PTR [0x00BFCBBC]
    mov eax, DWORD PTR [ecx]
    call DWORD PTR [eax+0x40]
    {load} mov ecx, eax
    mov edx, DWORD PTR [eax]
    call DWORD PTR [edx]
    jmp .Lfailed
.Lstart_game:
    call fn_5cfb10
    mov ebx, DWORD PTR [0x00BFCCFC]
    lea eax, [esp+0x14]
    cmp DWORD PTR [esp+0x28], 0x10
    {load} mov ecx, ebx
    push 0
    cmovae eax, DWORD PTR [esp+0x18]
    mov DWORD PTR [ebx+0x10], eax
    mov eax, DWORD PTR [ebx]
    mov eax, DWORD PTR [eax+4]
    call eax
    test al, al
    je .Lfailed
    mov eax, DWORD PTR [0x00BFCBBC]
    mov edi, DWORD PTR [0x0092D124]
    cmp BYTE PTR [eax+0x2d], 0
    jne .Lshutdown
    {disp32} lea esp, [esp+0]
.Lloop:
    call fn_48ce10
    mov eax, DWORD PTR [0x00BFCBBC]
    cmp BYTE PTR [eax+0x2d], 0
    jne .Lshutdown
    push 0
    push DWORD PTR [ebx+8]
    call edi /* WaitForSingleObject */
    test eax, eax
    je .Lshutdown
    {load} mov ecx, esi
    call fn_4a0af0
    mov eax, DWORD PTR [0x00BFCBBC]
    cmp BYTE PTR [eax+0x2d], 0
    je .Lloop
.Lshutdown:
    mov BYTE PTR [esi+0xba0], 1
    mov edi, 2
    add esi, 0x460
    nop
.Lsignal:
    push DWORD PTR [esi]
    call DWORD PTR [0x0092D120] /* SetEvent */
    lea esi, [esi+4]
    dec edi
    jne .Lsignal
    push -1
    push DWORD PTR [ebx+8]
    call DWORD PTR [0x0092D124]
    mov ecx, DWORD PTR [esp+0x10]
    call fn_4a08e0
    mov edi, DWORD PTR [0x0092D4B0]
    {load} xor esi, esi
    jmp .Lcleanup_string
.Ltimer_failure:
    push 0x10
    push 0x00A06304
    push 0x00A06310
    push 0
    call DWORD PTR [0x0092D598] /* MessageBoxA */
    mov ecx, DWORD PTR [esp+0x5c]
    or eax, -1
    pop edi
    pop esi
    pop ebx
    {load} xor ecx, esp
    call check_cookie
    {load} mov esp, ebp
    pop ebp
    ret 0x10
.size original_application_entry, .-original_application_entry
