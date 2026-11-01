/* Four original startup singleton helpers. Each section is linked at its
 * original VA. Constructor/exception-runtime internals remain external.
 */
.intel_syntax noprefix
.code32
.equ constructor_49fd30, 0x0049FD30
.equ constructor_55e530, 0x0055E530
.equ constructor_535710, 0x00535710

.section .object_5cfb10,"ax"
.globl object_5cfb10
object_5cfb10:
    cmp DWORD PTR [0x00BFCCFC], 0
    jne .Lsmall_done
    push 0x14
    call DWORD PTR [0x0092D4B4]
    add esp, 4
    test eax, eax
    je .Lsmall_null
    mov DWORD PTR [eax+4], 1
    mov DWORD PTR [eax+8], 0
    mov DWORD PTR [eax+0x0c], 0
    mov DWORD PTR [eax], 0x009FE634
    mov DWORD PTR [eax+0x10], 0x009998BC
    mov DWORD PTR [0x00BFCCFC], eax
    ret
.Lsmall_null:
    {load} xor eax, eax
    mov DWORD PTR [0x00BFCCFC], eax
.Lsmall_done:
    ret

/* Both allocation wrappers have the identical original exception-frame shape. */
.macro guarded_object symbol, global, bytes, constructor
.globl \symbol
\symbol:
    push ebp
    {load} mov ebp, esp
    push -1
    push 0x008FDFCC
    mov eax, DWORD PTR fs:[0]
    push eax
    push ecx
    mov eax, DWORD PTR [0x00BAABC0]
    {load} xor eax, ebp
    push eax
    lea eax, [ebp-0x0c]
    mov DWORD PTR fs:[0], eax
    cmp DWORD PTR [\global], 0
    jne .Lguard_done\@
    push \bytes
    call DWORD PTR [0x0092D4B4]
    add esp, 4
    mov DWORD PTR [ebp-0x10], eax
    mov DWORD PTR [ebp-4], 0
    test eax, eax
    je .Lguard_null\@
    {load} mov ecx, eax
    call \constructor
    mov DWORD PTR [\global], eax
    mov ecx, DWORD PTR [ebp-0x0c]
    mov DWORD PTR fs:[0], ecx
    pop ecx
    {load} mov esp, ebp
    pop ebp
    ret
.Lguard_null\@:
    {load} xor eax, eax
    mov DWORD PTR [\global], eax
.Lguard_done\@:
    mov ecx, DWORD PTR [ebp-0x0c]
    mov DWORD PTR fs:[0], ecx
    pop ecx
    {load} mov esp, ebp
    pop ebp
    ret
.endm
.section .object_5cfb60,"ax"
guarded_object object_5cfb60, 0x00BFCC44, 0x0bbc, constructor_49fd30
.section .object_5cfbe0,"ax"
guarded_object object_5cfbe0, 0x00BFCD2C, 0x00f4, constructor_55e530

.section .object_5cfc60,"ax"
.globl object_5cfc60
object_5cfc60:
    cmp DWORD PTR [0x00BFCD08], 0
    jne .Llast_done
    push 0x44
    call DWORD PTR [0x0092D4B4]
    add esp, 4
    test eax, eax
    je .Llast_null
    {load} mov ecx, eax
    call constructor_535710
    mov DWORD PTR [0x00BFCD08], eax
    ret
.Llast_null:
    {load} xor eax, eax
    mov DWORD PTR [0x00BFCD08], eax
.Llast_done:
    ret
