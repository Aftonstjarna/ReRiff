.intel_syntax noprefix
.code32
.text
.global original_request_dispatch
.equ fn_5367c0,0x5367c0
.equ fn_52f560,0x52f560
.equ fn_528d10,0x528d10
.equ fn_540a00,0x540a00
.equ fn_509be0,0x509be0
.equ fn_4a56c0,0x4a56c0
.equ fn_4a5850,0x4a5850
.equ fn_4a6330,0x4a6330
.equ fn_509c50,0x509c50
.equ fn_540bc0,0x540bc0
.equ fn_528e40,0x528e40
.equ fn_52f5b0,0x52f5b0
.equ fn_537530,0x537530
.equ fn_4a1bc0,0x4a1bc0
.equ fn_4a1cd0,0x4a1cd0
.equ fn_4a2130,0x4a2130
.equ fn_4adb10,0x4adb10
.equ fn_528f80,0x528f80
.equ fn_525560,0x525560
original_request_dispatch:
 push ebp
 {load} mov ebp,esp
 push ebx
 push esi
 push edi
 mov edi,DWORD PTR [ebp+8]
 {load} mov esi,ecx
 mov eax,DWORD PTR [edi]
 cmp eax,7
 ja .invalid
 jmp DWORD PTR [.requests+eax*4]
.case0:
 push DWORD PTR [edi+0x20]
 movzx eax,BYTE PTR [edi+0x19]
 push DWORD PTR [edi+0x1c]
 push eax
 movzx eax,BYTE PTR [edi+0x18]
 push eax
 movzx eax,BYTE PTR [edi+0x17]
 push eax
 movzx eax,BYTE PTR [edi+0x16]
 push ecx
 mov ecx,DWORD PTR [0xbfcd0c]
 push eax
 push DWORD PTR [esi+0x1bc]
 push DWORD PTR [esi+0x1b8]
 call fn_5367c0
 mov ecx,DWORD PTR [0xbfcd04]
 call fn_52f560
 push DWORD PTR [edi+0xc]
 mov ecx,DWORD PTR [0xbfcd00]
 call fn_528d10
 push DWORD PTR [edi+0x10]
 mov ecx,DWORD PTR [0xbfcd10]
 call fn_540a00
 mov ecx,DWORD PTR [0xbfcc70]
 call fn_509be0
 mov al,BYTE PTR [edi+8]
 mov BYTE PTR [esi+0xa5c],al
 mov al,BYTE PTR [edi+0x19]
 mov BYTE PTR [esi+0xa5e],al
 mov al,BYTE PTR [edi+0x1a]
 mov BYTE PTR [esi+0xa5f],al
 cmp BYTE PTR [esi+0xa5f],0
 mov al,BYTE PTR [edi+0x1b]
 mov BYTE PTR [esi+0xa60],al
 je .flags
 mov BYTE PTR [esi+0xa5e],1
.flags:
 cmp BYTE PTR [esi+0xa5e],0
 mov DWORD PTR [esi+0x4cc],0
 je .second
 {load} mov ecx,esi
 call fn_4a56c0
 test al,al
 je .optional
.second:
 cmp BYTE PTR [esi+0xa5f],0
 je .success
 {load} mov ecx,esi
 call fn_4a5850
 test al,al
 je .optional
.success:
 mov al,1
.optional:
 cmp BYTE PTR [esi+0xa74],0
 je .return
 {load} mov ecx,esi
 call fn_4a6330
 mov al,1
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case1:
 mov ecx,DWORD PTR [0xbfcc70]
 call fn_509c50
 cmp BYTE PTR [edi+6],0
 je .flag5
 mov ecx,DWORD PTR [0xbfcd10]
 call fn_540bc0
.flag5:
 cmp BYTE PTR [edi+5],0
 je .flag4
 mov ecx,DWORD PTR [0xbfcd00]
 call fn_528e40
.flag4:
 cmp BYTE PTR [edi+4],0
 je .cleanup
 mov ecx,DWORD PTR [0xbfcd04]
 call fn_52f5b0
.cleanup:
 mov ecx,DWORD PTR [0xbfcd0c]
 call fn_537530
 {load} mov ecx,esi
 mov BYTE PTR [esi+0xa5c],0
 mov WORD PTR [esi+0xa5e],0
 call fn_4a1bc0
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case2:
 push edi
 call fn_4a1cd0
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case3:
 push edi
 call fn_4a2130
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case4:
 mov eax,DWORD PTR [0xbfcd0c]
 mov ecx,DWORD PTR [edi+4]
 mov DWORD PTR [eax+0x260],ecx
 mov al,BYTE PTR [edi+8]
 mov BYTE PTR [esi+0xa5d],al
 mov ecx,DWORD PTR [edi+0xc]
 mov eax,DWORD PTR [esi+0xec]
 {load} cmp ecx,eax
 cmova ecx,eax
 mov DWORD PTR [esi+0x9a4],ecx
 mov al,BYTE PTR [edi+0x10]
 mov BYTE PTR [esi+0x9a8],al
 test al,al
 jne .case4done
 mov DWORD PTR [ebp+8],0
 {load} xor edi,edi
 mov ebx,DWORD PTR [ebp+8]
 mov BYTE PTR [esi+0x9a8],1
.clearloop:
 push ebx
 push ecx
 push edi
 {load} mov ecx,esi
 call fn_4adb10
 inc edi
 cmp edi,16
 jb .clearloop
 mov BYTE PTR [esi+0x9a8],0
.case4done:
 mov al,1
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case5:
 mov eax,DWORD PTR [edi+4]
 movss xmm1,DWORD PTR [0xa06558]
 cmp eax,4
 ja .scaleapply
 jmp DWORD PTR [.scales+eax*4]
.scale0:
 movss xmm1,DWORD PTR [0xa064e0]
 jmp .scaleapply
.scale1:
 movss xmm1,DWORD PTR [0xa06508]
 jmp .scaleapply
.scale3:
 movss xmm1,DWORD PTR [0xa06618]
 jmp .scaleapply
.scale4:
 movss xmm1,DWORD PTR [0xa066f8]
.scaleapply:
 pop edi
 pop esi
 pop ebx
 mov DWORD PTR [ebp+8],1
 mov ecx,DWORD PTR [0xbfcd00]
 pop ebp
 jmp fn_528f80
.case6:
 mov BYTE PTR [esi+0xba0],1
 mov al,1
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.case7:
 mov ecx,DWORD PTR [edi+4]
 mov bl,BYTE PTR [edi+0xa]
 mov dl,BYTE PTR [edi+9]
 mov al,BYTE PTR [edi+8]
 mov BYTE PTR [ecx+0x1e],al
 mov BYTE PTR [ecx+0x1f],dl
 mov BYTE PTR [ecx+0x20],bl
 mov ecx,DWORD PTR [edi+4]
 call fn_525560
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
.invalid:
 {load} xor al,al
.return:
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret 4
# Original two-byte padding before tables.
 {load} mov edi,edi
.requests:
 .long .case0,.case1,.case2,.case3,.case4,.case5,.case6,.case7
.scales:
 .long .scale0,.scale1,.scaleapply,.scale3,.scale4
