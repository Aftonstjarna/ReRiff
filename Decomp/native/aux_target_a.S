# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a56c0..0x4a5849
.intel_syntax noprefix
.code32
.text
.global original_aux_target_a
original_aux_target_a:
.L4a56c0:
 push   ebp
.L4a56c1:
 {load} mov    ebp,esp
.L4a56c3:
 push   0xffffffff
.L4a56c5:
 push   0x902b50
.L4a56ca:
 mov    eax,fs:0x0
.L4a56d0:
 push   eax
.L4a56d1:
 sub    esp,0x64
.L4a56d4:
 mov    eax,[0xbaabc0]
.L4a56d9:
 {load} xor    eax,ebp
.L4a56db:
 mov    DWORD PTR [ebp-0x10],eax
.L4a56de:
 push   ebx
.L4a56df:
 push   esi
.L4a56e0:
 push   edi
.L4a56e1:
 push   eax
.L4a56e2:
 lea    eax,[ebp-0xc]
.L4a56e5:
 mov    fs:0x0,eax
.L4a56eb:
 {load} mov    esi,ecx
.L4a56ed:
 cmp    BYTE PTR [esi+0xa5e],0x0
.L4a56f4:
 je .L4a582b
.L4a56fa:
 mov    ecx,DWORD PTR [esi+0x78]
.L4a56fd:
 lea    edi,[esi+0x43c]
.L4a5703:
 push   0x0
.L4a5705:
 push   edi
.L4a5706:
 push   0x0
.L4a5708:
 mov    eax,DWORD PTR [ecx]
.L4a570a:
 push   0x15
.L4a570c:
 push   0x1
.L4a570e:
 push   0x1
.L4a5710:
 push   DWORD PTR [esi+0x240]
.L4a5716:
 push   DWORD PTR [esi+0x23c]
.L4a571c:
 push   ecx
.L4a571d:
 call   DWORD PTR [eax+0x5c]
.L4a5720:
 test   eax,eax
.L4a5722:
 jns .L4a57b2
.L4a5728:
 {load} mov    edx,eax
.L4a572a:
 lea    ecx,[ebp-0x58]
.L4a572d:
 call target_44f380
.L4a5732:
 push   eax
.L4a5733:
 lea    eax,[ebp-0x40]
.L4a5736:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a573d:
 push   0x9fc5f4
.L4a5742:
 push   eax
.L4a5743:
 call target_4b2d20
.L4a5748:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a574e:
 {load} mov    ebx,eax
.L4a5750:
 add    esi,0x54
.L4a5753:
 add    esp,0xc
.L4a5756:
 {load} cmp    esi,ebx
.L4a5758:
 je .L4a5780
.L4a575a:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a575e:
 jb .L4a5767
.L4a5760:
 push   DWORD PTR [esi]
.L4a5762:
 call   edi
.L4a5764:
 add    esp,0x4
.L4a5767:
 mov    DWORD PTR [esi+0x14],0xf
.L4a576e:
 {load} mov    ecx,esi
.L4a5770:
 mov    DWORD PTR [esi+0x10],0x0
.L4a5777:
 push   ebx
.L4a5778:
 mov    BYTE PTR [esi],0x0
.L4a577b:
 call target_416fa0
.L4a5780:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a5784:
 jb .L4a578e
.L4a5786:
 push   DWORD PTR [ebp-0x40]
.L4a5789:
 call   edi
.L4a578b:
 add    esp,0x4
.L4a578e:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a5792:
 mov    DWORD PTR [ebp-0x2c],0xf
.L4a5799:
 mov    DWORD PTR [ebp-0x30],0x0
.L4a57a0:
 mov    BYTE PTR [ebp-0x40],0x0
.L4a57a4:
 jb .L4a57ae
.L4a57a6:
 push   DWORD PTR [ebp-0x58]
.L4a57a9:
 call   edi
.L4a57ab:
 add    esp,0x4
.L4a57ae:
 {load} xor    al,al
.L4a57b0:
 jmp .L4a582d
.L4a57b2:
 mov    ecx,DWORD PTR [edi]
.L4a57b4:
 lea    eax,[esi+0x440]
.L4a57ba:
 push   eax
.L4a57bb:
 push   0x0
.L4a57bd:
 push   ecx
.L4a57be:
 mov    edx,DWORD PTR [ecx]
.L4a57c0:
 call   DWORD PTR [edx+0x48]
.L4a57c3:
 test   eax,eax
.L4a57c5:
 jns .L4a582b
.L4a57c7:
 {load} mov    edx,eax
.L4a57c9:
 lea    ecx,[ebp-0x70]
.L4a57cc:
 call target_44f380
.L4a57d1:
 push   eax
.L4a57d2:
 lea    eax,[ebp-0x28]
.L4a57d5:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a57dc:
 push   0x9fc740
.L4a57e1:
 push   eax
.L4a57e2:
 call target_4b2d20
.L4a57e7:
 add    esp,0xc
.L4a57ea:
 lea    ecx,[esi+0x54]
.L4a57ed:
 push   eax
.L4a57ee:
 call target_440a20
.L4a57f3:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a57f7:
 mov    esi,DWORD PTR [0x92d4b0]
.L4a57fd:
 jb .L4a5807
.L4a57ff:
 push   DWORD PTR [ebp-0x28]
.L4a5802:
 call   esi
.L4a5804:
 add    esp,0x4
.L4a5807:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a580b:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a5812:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a5819:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a581d:
 jb .L4a57ae
.L4a581f:
 push   DWORD PTR [ebp-0x70]
.L4a5822:
 call   esi
.L4a5824:
 add    esp,0x4
.L4a5827:
 {load} xor    al,al
.L4a5829:
 jmp .L4a582d
.L4a582b:
 mov    al,0x1
.L4a582d:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a5830:
 mov    DWORD PTR fs:0x0,ecx
.L4a5837:
 pop    ecx
.L4a5838:
 pop    edi
.L4a5839:
 pop    esi
.L4a583a:
 pop    ebx
.L4a583b:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a583e:
 {load} xor    ecx,ebp
.L4a5840:
 call target_8f8958
.L4a5845:
 {load} mov    esp,ebp
.L4a5847:
 pop    ebp
.L4a5848:
 ret
.equ target_416fa0,0x416fa0
.equ target_440a20,0x440a20
.equ target_44f380,0x44f380
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
