/* Shared original SEH frame prologue and epilogue, VA 0x8F9C90..0x8F9CE9.
 * The exception-dispatch routine at 0x8F9CE9 remains an external dependency.
 */
.intel_syntax noprefix
.code32
.section .text
.globl original_seh_prolog
.type original_seh_prolog, @function
original_seh_prolog:
    push 0x008F9CE9
    push DWORD PTR fs:[0]
    mov eax, DWORD PTR [esp+0x10]
    mov DWORD PTR [esp+0x10], ebp
    lea ebp, [esp+0x10]
    {load} sub esp, eax
    push ebx
    push esi
    push edi
    mov eax, DWORD PTR [0x00BAABC0]
    xor DWORD PTR [ebp-4], eax
    {load} xor eax, ebp
    push eax
    mov DWORD PTR [ebp-0x18], esp
    push DWORD PTR [ebp-8]
    mov eax, DWORD PTR [ebp-4]
    mov DWORD PTR [ebp-4], -2
    mov DWORD PTR [ebp-8], eax
    lea eax, [ebp-0x10]
    mov DWORD PTR fs:[0], eax
    ret
.size original_seh_prolog, .-original_seh_prolog
.globl original_seh_epilog
.type original_seh_epilog, @function
original_seh_epilog:
    mov ecx, DWORD PTR [ebp-0x10]
    mov DWORD PTR fs:[0], ecx
    pop ecx
    pop edi
    pop edi
    pop esi
    pop ebx
    {load} mov esp, ebp
    pop ebp
    push ecx
    ret
.size original_seh_epilog, .-original_seh_epilog
