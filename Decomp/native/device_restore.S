# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a6560..0x4a669f
.intel_syntax noprefix
.code32
.text
.global original_device_restore
original_device_restore:
.L4a6560:
 push   ebp
.L4a6561:
 {load} mov    ebp,esp
.L4a6563:
 push   0xffffffff
.L4a6565:
 push   0x902b88
.L4a656a:
 mov    eax,fs:0x0
.L4a6570:
 push   eax
.L4a6571:
 push   ecx
.L4a6572:
 push   ebx
.L4a6573:
 push   esi
.L4a6574:
 push   edi
.L4a6575:
 mov    eax,[0xbaabc0]
.L4a657a:
 {load} xor    eax,ebp
.L4a657c:
 push   eax
.L4a657d:
 lea    eax,[ebp-0xc]
.L4a6580:
 mov    fs:0x0,eax
.L4a6586:
 {load} mov    esi,ecx
.L4a6588:
 cmp    DWORD PTR [esi+0x7c],0x0
.L4a658c:
 je .L4a65a2
.L4a658e:
 mov    al,0x1
.L4a6590:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a6593:
 mov    DWORD PTR fs:0x0,ecx
.L4a659a:
 pop    ecx
.L4a659b:
 pop    edi
.L4a659c:
 pop    esi
.L4a659d:
 pop    ebx
.L4a659e:
 {load} mov    esp,ebp
.L4a65a0:
 pop    ebp
.L4a65a1:
 ret
.L4a65a2:
 lea    eax,[esi+0xa84]
.L4a65a8:
 push   eax
.L4a65a9:
 mov    DWORD PTR [ebp-0x10],eax
.L4a65ac:
 call   DWORD PTR [0x92d024]
.L4a65b2:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a65b9:
 mov    ecx,DWORD PTR [0xbfcd0c]
.L4a65bf:
 mov    eax,DWORD PTR [ecx]
.L4a65c1:
 mov    eax,DWORD PTR [eax+0x8]
.L4a65c4:
 call   eax
.L4a65c6:
 test   al,al
.L4a65c8:
 je .L4a667c
.L4a65ce:
 mov    eax,DWORD PTR [esi+0xa7c]
.L4a65d4:
 {load} xor    edi,edi
.L4a65d6:
 sub    eax,DWORD PTR [esi+0xa78]
.L4a65dc:
 sar    eax,0x2
.L4a65df:
 test   eax,eax
.L4a65e1:
 je .L4a660f
.L4a65e3:
 mov    eax,DWORD PTR [esi+0xa78]
.L4a65e9:
 mov    ecx,DWORD PTR [eax+edi*4]
.L4a65ec:
 mov    eax,DWORD PTR [ecx]
.L4a65ee:
 mov    eax,DWORD PTR [eax+0x8]
.L4a65f1:
 call   eax
.L4a65f3:
 test   al,al
.L4a65f5:
 je .L4a667c
.L4a65fb:
 mov    eax,DWORD PTR [esi+0xa7c]
.L4a6601:
 inc    edi
.L4a6602:
 sub    eax,DWORD PTR [esi+0xa78]
.L4a6608:
 sar    eax,0x2
.L4a660b:
 {load} cmp    edi,eax
.L4a660d:
 jb .L4a65e3
.L4a660f:
 {load} mov    ecx,esi
.L4a6611:
 call target_4a4c50
.L4a6616:
 {load} mov    ecx,esi
.L4a6618:
 call target_4a5240
.L4a661d:
 test   al,al
.L4a661f:
 je .L4a667c
.L4a6621:
 {load} mov    ecx,esi
.L4a6623:
 call target_4a5530
.L4a6628:
 test   al,al
.L4a662a:
 je .L4a667c
.L4a662c:
 {load} mov    ecx,esi
.L4a662e:
 call target_4a56c0
.L4a6633:
 test   al,al
.L4a6635:
 je .L4a667c
.L4a6637:
 {load} mov    ecx,esi
.L4a6639:
 call target_4a5850
.L4a663e:
 test   al,al
.L4a6640:
 je .L4a667c
.L4a6642:
 mov    al,BYTE PTR [esi+0x9e]
.L4a6648:
 lea    ecx,[esi+0x214]
.L4a664e:
 push   0x60000
.L4a6653:
 and    al,0x1
.L4a6655:
 push   0x1000000
.L4a665a:
 movzx  eax,al
.L4a665d:
 push   eax
.L4a665e:
 push   DWORD PTR [esi+0x78]
.L4a6661:
 call target_5429a0
.L4a6666:
 test   eax,eax
.L4a6668:
 js .L4a667c
.L4a666a:
 {load} mov    ecx,esi
.L4a666c:
 mov    BYTE PTR [esi+0xa74],0x0
.L4a6673:
 call target_4a5e80
.L4a6678:
 mov    bl,0x1
.L4a667a:
 jmp .L4a667e
.L4a667c:
 {load} xor    bl,bl
.L4a667e:
 lea    eax,[esi+0xa84]
.L4a6684:
 push   eax
.L4a6685:
 call   DWORD PTR [0x92d028]
.L4a668b:
 {load} mov    al,bl
.L4a668d:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a6690:
 mov    DWORD PTR fs:0x0,ecx
.L4a6697:
 pop    ecx
.L4a6698:
 pop    edi
.L4a6699:
 pop    esi
.L4a669a:
 pop    ebx
.L4a669b:
 {load} mov    esp,ebp
.L4a669d:
 pop    ebp
.L4a669e:
 ret
.equ target_4a4c50,0x4a4c50
.equ target_4a5240,0x4a5240
.equ target_4a5530,0x4a5530
.equ target_4a56c0,0x4a56c0
.equ target_4a5850,0x4a5850
.equ target_4a5e80,0x4a5e80
.equ target_5429a0,0x5429a0
