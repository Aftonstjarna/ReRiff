# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a5530..0x4a56bd
.intel_syntax noprefix
.code32
.text
.global original_effect_target
original_effect_target:
.L4a5530:
 push   ebp
.L4a5531:
 {load} mov    ebp,esp
.L4a5533:
 push   0xffffffff
.L4a5535:
 push   0x902b50
.L4a553a:
 mov    eax,fs:0x0
.L4a5540:
 push   eax
.L4a5541:
 sub    esp,0x64
.L4a5544:
 mov    eax,[0xbaabc0]
.L4a5549:
 {load} xor    eax,ebp
.L4a554b:
 mov    DWORD PTR [ebp-0x10],eax
.L4a554e:
 push   ebx
.L4a554f:
 push   esi
.L4a5550:
 push   edi
.L4a5551:
 push   eax
.L4a5552:
 lea    eax,[ebp-0xc]
.L4a5555:
 mov    fs:0x0,eax
.L4a555b:
 {load} mov    esi,ecx
.L4a555d:
 cmp    BYTE PTR [esi+0x4bc],0x0
.L4a5564:
 je .L4a569f
.L4a556a:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a556d:
 lea    edi,[esi+0x434]
.L4a5573:
 push   0x0
.L4a5575:
 push   edi
.L4a5576:
 push   0x0
.L4a5578:
 push   DWORD PTR [esi+0x1fc]
.L4a557e:
 mov    eax,DWORD PTR [ecx]
.L4a5580:
 push   0x1
.L4a5582:
 push   0x1
.L4a5584:
 push   DWORD PTR [esi+0x240]
.L4a558a:
 push   DWORD PTR [esi+0x23c]
.L4a5590:
 push   ecx
.L4a5591:
 call   DWORD PTR [eax+0x5c]
.L4a5594:
 test   eax,eax
.L4a5596:
 jns .L4a5626
.L4a559c:
 {load} mov    edx,eax
.L4a559e:
 lea    ecx,[ebp-0x58]
.L4a55a1:
 call target_44f380
.L4a55a6:
 push   eax
.L4a55a7:
 lea    eax,[ebp-0x40]
.L4a55aa:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a55b1:
 push   0x9fc5f4
.L4a55b6:
 push   eax
.L4a55b7:
 call target_4b2d20
.L4a55bc:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a55c2:
 {load} mov    ebx,eax
.L4a55c4:
 add    esi,0x54
.L4a55c7:
 add    esp,0xc
.L4a55ca:
 {load} cmp    esi,ebx
.L4a55cc:
 je .L4a55f4
.L4a55ce:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a55d2:
 jb .L4a55db
.L4a55d4:
 push   DWORD PTR [esi]
.L4a55d6:
 call   edi
.L4a55d8:
 add    esp,0x4
.L4a55db:
 mov    DWORD PTR [esi+0x14],0xf
.L4a55e2:
 {load} mov    ecx,esi
.L4a55e4:
 mov    DWORD PTR [esi+0x10],0x0
.L4a55eb:
 push   ebx
.L4a55ec:
 mov    BYTE PTR [esi],0x0
.L4a55ef:
 call target_416fa0
.L4a55f4:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a55f8:
 jb .L4a5602
.L4a55fa:
 push   DWORD PTR [ebp-0x40]
.L4a55fd:
 call   edi
.L4a55ff:
 add    esp,0x4
.L4a5602:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a5606:
 mov    DWORD PTR [ebp-0x2c],0xf
.L4a560d:
 mov    DWORD PTR [ebp-0x30],0x0
.L4a5614:
 mov    BYTE PTR [ebp-0x40],0x0
.L4a5618:
 jb .L4a5622
.L4a561a:
 push   DWORD PTR [ebp-0x58]
.L4a561d:
 call   edi
.L4a561f:
 add    esp,0x4
.L4a5622:
 {load} xor    al,al
.L4a5624:
 jmp .L4a56a1
.L4a5626:
 mov    ecx,DWORD PTR [edi]
.L4a5628:
 lea    eax,[esi+0x438]
.L4a562e:
 push   eax
.L4a562f:
 push   0x0
.L4a5631:
 push   ecx
.L4a5632:
 mov    edx,DWORD PTR [ecx]
.L4a5634:
 call   DWORD PTR [edx+0x48]
.L4a5637:
 test   eax,eax
.L4a5639:
 jns .L4a569f
.L4a563b:
 {load} mov    edx,eax
.L4a563d:
 lea    ecx,[ebp-0x70]
.L4a5640:
 call target_44f380
.L4a5645:
 push   eax
.L4a5646:
 lea    eax,[ebp-0x28]
.L4a5649:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a5650:
 push   0x9fc740
.L4a5655:
 push   eax
.L4a5656:
 call target_4b2d20
.L4a565b:
 add    esp,0xc
.L4a565e:
 lea    ecx,[esi+0x54]
.L4a5661:
 push   eax
.L4a5662:
 call target_440a20
.L4a5667:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a566b:
 mov    esi,DWORD PTR [0x92d4b0]
.L4a5671:
 jb .L4a567b
.L4a5673:
 push   DWORD PTR [ebp-0x28]
.L4a5676:
 call   esi
.L4a5678:
 add    esp,0x4
.L4a567b:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a567f:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a5686:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a568d:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a5691:
 jb .L4a5622
.L4a5693:
 push   DWORD PTR [ebp-0x70]
.L4a5696:
 call   esi
.L4a5698:
 add    esp,0x4
.L4a569b:
 {load} xor    al,al
.L4a569d:
 jmp .L4a56a1
.L4a569f:
 mov    al,0x1
.L4a56a1:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a56a4:
 mov    DWORD PTR fs:0x0,ecx
.L4a56ab:
 pop    ecx
.L4a56ac:
 pop    edi
.L4a56ad:
 pop    esi
.L4a56ae:
 pop    ebx
.L4a56af:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a56b2:
 {load} xor    ecx,ebp
.L4a56b4:
 call target_8f8958
.L4a56b9:
 {load} mov    esp,ebp
.L4a56bb:
 pop    ebp
.L4a56bc:
 ret
.equ target_416fa0,0x416fa0
.equ target_440a20,0x440a20
.equ target_44f380,0x44f380
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
