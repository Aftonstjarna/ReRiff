# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a0480..0x4a08d6
.intel_syntax noprefix
.code32
.text
.global original_graphics_startup
original_graphics_startup:
.L4a0480:
 push   ebp
.L4a0481:
 {load} mov    ebp,esp
.L4a0483:
 push   0xffffffff
.L4a0485:
 push   0x9029bc
.L4a048a:
 mov    eax,fs:0x0
.L4a0490:
 push   eax
.L4a0491:
 sub    esp,0x30
.L4a0494:
 mov    eax,[0xbaabc0]
.L4a0499:
 {load} xor    eax,ebp
.L4a049b:
 mov    DWORD PTR [ebp-0x14],eax
.L4a049e:
 push   ebx
.L4a049f:
 push   esi
.L4a04a0:
 push   edi
.L4a04a1:
 push   eax
.L4a04a2:
 lea    eax,[ebp-0xc]
.L4a04a5:
 mov    fs:0x0,eax
.L4a04ab:
 {load} mov    edi,ecx
.L4a04ad:
 mov    eax,DWORD PTR [ebp+0x8]
.L4a04b0:
 lea    esi,[edi+0x460]
.L4a04b6:
 mov    DWORD PTR [ebp-0x38],eax
.L4a04b9:
 {load} xor    ebx,ebx
.L4a04bb:
 mov    eax,DWORD PTR [ebp+0xc]
.L4a04be:
 mov    DWORD PTR [ebp-0x3c],eax
.L4a04c1:
 push   0x0
.L4a04c3:
 push   0x0
.L4a04c5:
 push   0x0
.L4a04c7:
 push   0x0
.L4a04c9:
 call   DWORD PTR [0x92d114]
.L4a04cf:
 mov    DWORD PTR [esi-0x8],eax
.L4a04d2:
 test   eax,eax
.L4a04d4:
 je .L4a0874
.L4a04da:
 push   0x0
.L4a04dc:
 push   0x1
.L4a04de:
 push   0x1
.L4a04e0:
 push   0x0
.L4a04e2:
 call   DWORD PTR [0x92d114]
.L4a04e8:
 mov    DWORD PTR [esi],eax
.L4a04ea:
 test   eax,eax
.L4a04ec:
 je .L4a084a
.L4a04f2:
 inc    ebx
.L4a04f3:
 add    esi,0x4
.L4a04f6:
 cmp    ebx,0x2
.L4a04f9:
 jb .L4a04c1
.L4a04fb:
 cmp    DWORD PTR [0xbfcd18],0x0
.L4a0502:
 mov    ebx,DWORD PTR [0x92d4b4]
.L4a0508:
 jne .L4a0556
.L4a050a:
 push   0xc
.L4a050c:
 call   ebx
.L4a050e:
 {load} mov    esi,eax
.L4a0510:
 add    esp,0x4
.L4a0513:
 mov    DWORD PTR [ebp-0x30],esi
.L4a0516:
 mov    DWORD PTR [ebp-0x4],0x2
.L4a051d:
 test   esi,esi
.L4a051f:
 je .L4a0547
.L4a0521:
 mov    DWORD PTR [esi],0x0
.L4a0527:
 mov    DWORD PTR [esi+0x4],0x0
.L4a052e:
 mov    DWORD PTR [esi+0x8],0x0
.L4a0535:
 push   0x400
.L4a053a:
 {load} mov    ecx,esi
.L4a053c:
 mov    BYTE PTR [ebp-0x4],0x3
.L4a0540:
 call target_426800
.L4a0545:
 jmp .L4a0549
.L4a0547:
 {load} xor    esi,esi
.L4a0549:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a0550:
 mov    DWORD PTR [0xbfcd18],esi
.L4a0556:
 cmp    DWORD PTR [0xbfcd04],0x0
.L4a055d:
 jne .L4a058e
.L4a055f:
 push   0x2864
.L4a0564:
 call   ebx
.L4a0566:
 add    esp,0x4
.L4a0569:
 mov    DWORD PTR [ebp-0x30],eax
.L4a056c:
 mov    DWORD PTR [ebp-0x4],0x4
.L4a0573:
 test   eax,eax
.L4a0575:
 je .L4a0580
.L4a0577:
 {load} mov    ecx,eax
.L4a0579:
 call target_52d900
.L4a057e:
 jmp .L4a0582
.L4a0580:
 {load} xor    eax,eax
.L4a0582:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a0589:
 mov    [0xbfcd04],eax
.L4a058e:
 cmp    DWORD PTR [0xbfcd0c],0x0
.L4a0595:
 jne .L4a05c6
.L4a0597:
 push   0x264
.L4a059c:
 call   ebx
.L4a059e:
 add    esp,0x4
.L4a05a1:
 mov    DWORD PTR [ebp-0x30],eax
.L4a05a4:
 mov    DWORD PTR [ebp-0x4],0x5
.L4a05ab:
 test   eax,eax
.L4a05ad:
 je .L4a05b8
.L4a05af:
 {load} mov    ecx,eax
.L4a05b1:
 call target_536340
.L4a05b6:
 jmp .L4a05ba
.L4a05b8:
 {load} xor    eax,eax
.L4a05ba:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a05c1:
 mov    [0xbfcd0c],eax
.L4a05c6:
 cmp    DWORD PTR [0xbfcd00],0x0
.L4a05cd:
 jne .L4a05fb
.L4a05cf:
 push   0x48
.L4a05d1:
 call   ebx
.L4a05d3:
 add    esp,0x4
.L4a05d6:
 mov    DWORD PTR [ebp-0x30],eax
.L4a05d9:
 mov    DWORD PTR [ebp-0x4],0x6
.L4a05e0:
 test   eax,eax
.L4a05e2:
 je .L4a05ed
.L4a05e4:
 {load} mov    ecx,eax
.L4a05e6:
 call target_528ae0
.L4a05eb:
 jmp .L4a05ef
.L4a05ed:
 {load} xor    eax,eax
.L4a05ef:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a05f6:
 mov    [0xbfcd00],eax
.L4a05fb:
 cmp    DWORD PTR [0xbfcc70],0x0
.L4a0602:
 jne .L4a0656
.L4a0604:
 push   0x24
.L4a0606:
 call   ebx
.L4a0608:
 add    esp,0x4
.L4a060b:
 test   eax,eax
.L4a060d:
 je .L4a064f
.L4a060f:
 mov    DWORD PTR [eax],0x0
.L4a0615:
 mov    DWORD PTR [eax+0x4],0x0
.L4a061c:
 mov    DWORD PTR [eax+0x8],0x0
.L4a0623:
 mov    DWORD PTR [eax+0xc],0x0
.L4a062a:
 mov    DWORD PTR [eax+0x10],0x0
.L4a0631:
 mov    DWORD PTR [eax+0x14],0x0
.L4a0638:
 mov    DWORD PTR [eax+0x18],0x0
.L4a063f:
 mov    DWORD PTR [eax+0x1c],0x0
.L4a0646:
 mov    DWORD PTR [eax+0x20],0x0
.L4a064d:
 jmp .L4a0651
.L4a064f:
 {load} xor    eax,eax
.L4a0651:
 mov    [0xbfcc70],eax
.L4a0656:
 cmp    DWORD PTR [0xbfcd10],0x0
.L4a065d:
 jne .L4a068b
.L4a065f:
 push   0x68
.L4a0661:
 call   ebx
.L4a0663:
 add    esp,0x4
.L4a0666:
 mov    DWORD PTR [ebp-0x30],eax
.L4a0669:
 mov    DWORD PTR [ebp-0x4],0x7
.L4a0670:
 test   eax,eax
.L4a0672:
 je .L4a067d
.L4a0674:
 {load} mov    ecx,eax
.L4a0676:
 call target_540810
.L4a067b:
 jmp .L4a067f
.L4a067d:
 {load} xor    eax,eax
.L4a067f:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a0686:
 mov    [0xbfcd10],eax
.L4a068b:
 {load} mov    ecx,edi
.L4a068d:
 call target_4a2be0
.L4a0692:
 test   al,al
.L4a0694:
 je .L4a0792
.L4a069a:
 cmp    DWORD PTR [0xbfcc28],0x0
.L4a06a1:
 jne .L4a0706
.L4a06a3:
 push   0x2c
.L4a06a5:
 call   ebx
.L4a06a7:
 add    esp,0x4
.L4a06aa:
 test   eax,eax
.L4a06ac:
 je .L4a06ff
.L4a06ae:
 mov    DWORD PTR [eax],0x0
.L4a06b4:
 mov    DWORD PTR [eax+0x24],0x0
.L4a06bb:
 mov    WORD PTR [eax+0x28],0x0
.L4a06c1:
 mov    BYTE PTR [eax+0x2a],0x1
.L4a06c5:
 mov    DWORD PTR [eax+0x4],0x0
.L4a06cc:
 mov    DWORD PTR [eax+0x8],0x0
.L4a06d3:
 mov    DWORD PTR [eax+0xc],0x0
.L4a06da:
 mov    DWORD PTR [eax+0x10],0x0
.L4a06e1:
 mov    DWORD PTR [eax+0x14],0x0
.L4a06e8:
 mov    DWORD PTR [eax+0x18],0x0
.L4a06ef:
 mov    DWORD PTR [eax+0x1c],0x0
.L4a06f6:
 mov    DWORD PTR [eax+0x20],0x0
.L4a06fd:
 jmp .L4a0701
.L4a06ff:
 {load} xor    eax,eax
.L4a0701:
 mov    [0xbfcc28],eax
.L4a0706:
 push   0x2d0
.L4a070b:
 push   0x500
.L4a0710:
 {load} mov    ecx,edi
.L4a0712:
 call target_4a22e0
.L4a0717:
 {load} mov    ecx,edi
.L4a0719:
 call target_4a2460
.L4a071e:
 push   DWORD PTR [ebp-0x3c]
.L4a0721:
 {load} mov    ecx,edi
.L4a0723:
 call target_4a27d0
.L4a0728:
 {load} mov    ecx,edi
.L4a072a:
 call target_4a33f0
.L4a072f:
 call target_6d86bb
.L4a0734:
 push   DWORD PTR [ebp+0x10]
.L4a0737:
 mov    esi,DWORD PTR [0xbfcc28]
.L4a073d:
 {load} mov    ecx,eax
.L4a073f:
 push   0x1
.L4a0741:
 mov    edx,DWORD PTR [eax]
.L4a0743:
 call   DWORD PTR [edx+0xc]
.L4a0746:
 push   eax
.L4a0747:
 movzx  eax,BYTE PTR [edi+0x4bf]
.L4a074e:
 {load} mov    ecx,esi
.L4a0750:
 push   eax
.L4a0751:
 push   DWORD PTR [edi+0x490]
.L4a0757:
 push   DWORD PTR [edi+0x48c]
.L4a075d:
 push   DWORD PTR [edi+0x210]
.L4a0763:
 push   DWORD PTR [edi+0x20c]
.L4a0769:
 push   DWORD PTR [ebp-0x38]
.L4a076c:
 call target_48c510
.L4a0771:
 {load} mov    ecx,edi
.L4a0773:
 test   al,al
.L4a0775:
 je .L4a078d
.L4a0777:
 call target_4a3630
.L4a077c:
 test   al,al
.L4a077e:
 jne .L4a079f
.L4a0780:
 mov    ecx,DWORD PTR [0xbfcc28]
.L4a0786:
 call target_48c690
.L4a078b:
 {load} mov    ecx,edi
.L4a078d:
 call target_4a5c40
.L4a0792:
 lea    ecx,[edi+0x54]
.L4a0795:
 call target_44f4f0
.L4a079a:
 jmp .L4a08b6
.L4a079f:
 mov    ecx,DWORD PTR [edi+0x78]
.L4a07a2:
 lea    esi,[edi+0x3a0]
.L4a07a8:
 mov    DWORD PTR [esi],ecx
.L4a07aa:
 push   ecx
.L4a07ab:
 mov    eax,DWORD PTR [ecx]
.L4a07ad:
 call   DWORD PTR [eax+0x4]
.L4a07b0:
 call   DWORD PTR [0x92d0ec]
.L4a07b6:
 mov    ecx,DWORD PTR [0xbfcd0c]
.L4a07bc:
 mov    DWORD PTR [esi+0x4],eax
.L4a07bf:
 push   DWORD PTR [edi+0x1bc]
.L4a07c5:
 push   DWORD PTR [edi+0x1b8]
.L4a07cb:
 call target_53d2d0
.L4a07d0:
 mov    ecx,DWORD PTR [0xbfcd04]
.L4a07d6:
 call target_52db30
.L4a07db:
 test   al,al
.L4a07dd:
 jne .L4a0828
.L4a07df:
 {load} mov    ecx,esi
.L4a07e1:
 mov    BYTE PTR [esi+0x74],0x1
.L4a07e5:
 call target_542f80
.L4a07ea:
 mov    ecx,DWORD PTR [esi]
.L4a07ec:
 test   ecx,ecx
.L4a07ee:
 je .L4a07f6
.L4a07f0:
 mov    eax,DWORD PTR [ecx]
.L4a07f2:
 push   ecx
.L4a07f3:
 call   DWORD PTR [eax+0x8]
.L4a07f6:
 mov    ecx,DWORD PTR [0xbfcc28]
.L4a07fc:
 mov    DWORD PTR [esi],0x0
.L4a0802:
 mov    BYTE PTR [esi+0x74],0x0
.L4a0806:
 call target_48c690
.L4a080b:
 {load} mov    ecx,edi
.L4a080d:
 call target_4a5c40
.L4a0812:
 push   0x9fc2a0
.L4a0817:
 lea    ecx,[ebp-0x2c]
.L4a081a:
 call target_415b00
.L4a081f:
 mov    DWORD PTR [ebp-0x4],0x8
.L4a0826:
 jmp .L4a089c
.L4a0828:
 {load} mov    ecx,edi
.L4a082a:
 call target_4a5b00
.L4a082f:
 lea    eax,[ebp-0x34]
.L4a0832:
 push   eax
.L4a0833:
 call   DWORD PTR [0x92d0f4]
.L4a0839:
 mov    eax,DWORD PTR [ebp-0x34]
.L4a083c:
 mov    DWORD PTR [edi+0x4c0],eax
.L4a0842:
 mov    al,0x1
.L4a0844:
 mov    BYTE PTR [edi+0x50],0x0
.L4a0848:
 jmp .L4a08b8
.L4a084a:
 push   0x15
.L4a084c:
 push   0x9fc2d0
.L4a0851:
 lea    ecx,[ebp-0x2c]
.L4a0854:
 mov    DWORD PTR [ebp-0x18],0xf
.L4a085b:
 mov    DWORD PTR [ebp-0x1c],0x0
.L4a0862:
 mov    BYTE PTR [ebp-0x2c],0x0
.L4a0866:
 call target_416520
.L4a086b:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a0872:
 jmp .L4a089c
.L4a0874:
 push   0x15
.L4a0876:
 push   0x9fc2d0
.L4a087b:
 lea    ecx,[ebp-0x2c]
.L4a087e:
 mov    DWORD PTR [ebp-0x18],0xf
.L4a0885:
 mov    DWORD PTR [ebp-0x1c],0x0
.L4a088c:
 mov    BYTE PTR [ebp-0x2c],0x0
.L4a0890:
 call target_416520
.L4a0895:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a089c:
 lea    ecx,[ebp-0x2c]
.L4a089f:
 call target_44f4f0
.L4a08a4:
 cmp    DWORD PTR [ebp-0x18],0x10
.L4a08a8:
 jb .L4a08b6
.L4a08aa:
 push   DWORD PTR [ebp-0x2c]
.L4a08ad:
 call   DWORD PTR [0x92d4b0]
.L4a08b3:
 add    esp,0x4
.L4a08b6:
 {load} xor    al,al
.L4a08b8:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a08bb:
 mov    DWORD PTR fs:0x0,ecx
.L4a08c2:
 pop    ecx
.L4a08c3:
 pop    edi
.L4a08c4:
 pop    esi
.L4a08c5:
 pop    ebx
.L4a08c6:
 mov    ecx,DWORD PTR [ebp-0x14]
.L4a08c9:
 {load} xor    ecx,ebp
.L4a08cb:
 call target_8f8958
.L4a08d0:
 {load} mov    esp,ebp
.L4a08d2:
 pop    ebp
.L4a08d3:
 ret    0xc
.equ target_415b00,0x415b00
.equ target_416520,0x416520
.equ target_426800,0x426800
.equ target_44f4f0,0x44f4f0
.equ target_48c510,0x48c510
.equ target_48c690,0x48c690
.equ target_4a22e0,0x4a22e0
.equ target_4a2460,0x4a2460
.equ target_4a27d0,0x4a27d0
.equ target_4a2be0,0x4a2be0
.equ target_4a33f0,0x4a33f0
.equ target_4a3630,0x4a3630
.equ target_4a5b00,0x4a5b00
.equ target_4a5c40,0x4a5c40
.equ target_528ae0,0x528ae0
.equ target_52d900,0x52d900
.equ target_52db30,0x52db30
.equ target_536340,0x536340
.equ target_53d2d0,0x53d2d0
.equ target_540810,0x540810
.equ target_542f80,0x542f80
.equ target_6d86bb,0x6d86bb
.equ target_8f8958,0x8f8958
