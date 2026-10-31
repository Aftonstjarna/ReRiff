# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a3630..0x4a3a33
.intel_syntax noprefix
.code32
.text
.global original_device_create
original_device_create:
.L4a3630:
 push   ebp
.L4a3631:
 {load} mov    ebp,esp
.L4a3633:
 push   0xffffffff
.L4a3635:
 push   0x902a98
.L4a363a:
 mov    eax,fs:0x0
.L4a3640:
 push   eax
.L4a3641:
 sub    esp,0xa4
.L4a3647:
 mov    eax,[0xbaabc0]
.L4a364c:
 {load} xor    eax,ebp
.L4a364e:
 mov    DWORD PTR [ebp-0x10],eax
.L4a3651:
 push   ebx
.L4a3652:
 push   esi
.L4a3653:
 push   edi
.L4a3654:
 push   eax
.L4a3655:
 lea    eax,[ebp-0xc]
.L4a3658:
 mov    fs:0x0,eax
.L4a365e:
 {load} mov    esi,ecx
.L4a3660:
 mov    edx,DWORD PTR [0xbfcc28]
.L4a3666:
 mov    ecx,DWORD PTR [esi+0x74]
.L4a3669:
 mov    eax,DWORD PTR [edx+0x4]
.L4a366c:
 mov    DWORD PTR [esi+0x1d4],eax
.L4a3672:
 test   ecx,ecx
.L4a3674:
 je .L4a36fa
.L4a367a:
 {load} xor    ebx,ebx
.L4a367c:
 cmp    DWORD PTR [esi+0x1d8],ebx
.L4a3682:
 jne .L4a36b5
.L4a3684:
 mov    eax,DWORD PTR [esi+0x1b8]
.L4a368a:
 mov    DWORD PTR [ebp-0x24],eax
.L4a368d:
 mov    eax,DWORD PTR [esi+0x1bc]
.L4a3693:
 mov    DWORD PTR [ebp-0x20],eax
.L4a3696:
 mov    eax,DWORD PTR [esi+0x1e8]
.L4a369c:
 mov    DWORD PTR [ebp-0x1c],eax
.L4a369f:
 mov    eax,DWORD PTR [esi+0x1c0]
.L4a36a5:
 mov    DWORD PTR [ebp-0x14],ebx
.L4a36a8:
 lea    ebx,[ebp-0x28]
.L4a36ab:
 mov    DWORD PTR [ebp-0x28],0x18
.L4a36b2:
 mov    DWORD PTR [ebp-0x18],eax
.L4a36b5:
 lea    eax,[esi+0x7c]
.L4a36b8:
 mov    DWORD PTR [ebp-0x74],eax
.L4a36bb:
 lea    edi,[esi+0x1b8]
.L4a36c1:
 push   DWORD PTR [ebp-0x74]
.L4a36c4:
 mov    eax,DWORD PTR [ecx]
.L4a36c6:
 push   ebx
.L4a36c7:
 push   edi
.L4a36c8:
 push   DWORD PTR [esi+0x1b4]
.L4a36ce:
 push   DWORD PTR [edx+0x4]
.L4a36d1:
 push   DWORD PTR [esi+0x1b0]
.L4a36d7:
 push   DWORD PTR [esi+0x6c]
.L4a36da:
 push   ecx
.L4a36db:
 call   DWORD PTR [eax+0x50]
.L4a36de:
 {load} mov    ebx,eax
.L4a36e0:
 mov    DWORD PTR [ebp-0x74],ebx
.L4a36e3:
 test   ebx,ebx
.L4a36e5:
 js .L4a372e
.L4a36e7:
 mov    edx,DWORD PTR [esi+0x7c]
.L4a36ea:
 lea    ebx,[esi+0x78]
.L4a36ed:
 mov    DWORD PTR [ebx],edx
.L4a36ef:
 push   edx
.L4a36f0:
 mov    ecx,DWORD PTR [edx]
.L4a36f2:
 call   DWORD PTR [ecx+0x4]
.L4a36f5:
 mov    eax,DWORD PTR [ebp-0x74]
.L4a36f8:
 jmp .L4a3723
.L4a36fa:
 mov    ecx,DWORD PTR [esi+0x70]
.L4a36fd:
 lea    ebx,[esi+0x78]
.L4a3700:
 push   ebx
.L4a3701:
 lea    edi,[esi+0x1b8]
.L4a3707:
 push   edi
.L4a3708:
 push   DWORD PTR [esi+0x1b4]
.L4a370e:
 mov    eax,DWORD PTR [ecx]
.L4a3710:
 push   DWORD PTR [edx+0x4]
.L4a3713:
 push   DWORD PTR [esi+0x1b0]
.L4a3719:
 push   DWORD PTR [esi+0x6c]
.L4a371c:
 push   ecx
.L4a371d:
 call   DWORD PTR [eax+0x40]
.L4a3720:
 mov    DWORD PTR [ebp-0x74],eax
.L4a3723:
 test   eax,eax
.L4a3725:
 jns .L4a3850
.L4a372b:
 mov    ebx,DWORD PTR [ebp-0x74]
.L4a372e:
 movdqu xmm0,XMMWORD PTR [edi]
.L4a3732:
 push   0x2d0
.L4a3737:
 push   0x500
.L4a373c:
 movdqu XMMWORD PTR [ebp-0xac],xmm0
.L4a3744:
 {load} mov    ecx,esi
.L4a3746:
 mov    BYTE PTR [esi+0x4bf],0x0
.L4a374d:
 movdqu xmm0,XMMWORD PTR [edi+0x10]
.L4a3752:
 movdqu XMMWORD PTR [ebp-0x9c],xmm0
.L4a375a:
 movdqu xmm0,XMMWORD PTR [edi+0x20]
.L4a375f:
 movdqu XMMWORD PTR [ebp-0x8c],xmm0
.L4a3767:
 movq   xmm0,QWORD PTR [edi+0x30]
.L4a376c:
 movq   QWORD PTR [ebp-0x7c],xmm0
.L4a3771:
 call target_4a22e0
.L4a3776:
 {load} mov    ecx,esi
.L4a3778:
 call target_4a33f0
.L4a377d:
 lea    ecx,[ebp-0xac]
.L4a3783:
 mov    edx,0x34
.L4a3788:
 jmp .L4a3790
.L4a378a:
 {disp32} lea    ebx,[ebx+0x0]
.L4a3790:
 mov    eax,DWORD PTR [ecx]
.L4a3792:
 cmp    eax,DWORD PTR [edi]
.L4a3794:
 jne .L4a3844
.L4a379a:
 add    ecx,0x4
.L4a379d:
 add    edi,0x4
.L4a37a0:
 sub    edx,0x4
.L4a37a3:
 jae .L4a3790
.L4a37a5:
 {load} mov    edx,ebx
.L4a37a7:
 lea    ecx,[ebp-0x40]
.L4a37aa:
 call target_44f380
.L4a37af:
 push   eax
.L4a37b0:
 lea    eax,[ebp-0x28]
.L4a37b3:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a37ba:
 push   0x9fc54c
.L4a37bf:
 push   eax
.L4a37c0:
 call target_4b2d20
.L4a37c5:
 mov    ebx,DWORD PTR [0x92d4b0]
.L4a37cb:
 lea    edi,[esi+0x54]
.L4a37ce:
 add    esp,0xc
.L4a37d1:
 mov    DWORD PTR [ebp-0x74],eax
.L4a37d4:
 {load} cmp    edi,eax
.L4a37d6:
 je .L4a3801
.L4a37d8:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a37dc:
 jb .L4a37e8
.L4a37de:
 push   DWORD PTR [edi]
.L4a37e0:
 call   ebx
.L4a37e2:
 mov    eax,DWORD PTR [ebp-0x74]
.L4a37e5:
 add    esp,0x4
.L4a37e8:
 mov    DWORD PTR [edi+0x14],0xf
.L4a37ef:
 {load} mov    ecx,edi
.L4a37f1:
 mov    DWORD PTR [edi+0x10],0x0
.L4a37f8:
 push   eax
.L4a37f9:
 mov    BYTE PTR [edi],0x0
.L4a37fc:
 call target_416fa0
.L4a3801:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a3805:
 jb .L4a380f
.L4a3807:
 push   DWORD PTR [ebp-0x28]
.L4a380a:
 call   ebx
.L4a380c:
 add    esp,0x4
.L4a380f:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a3816:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a381a:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a3821:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a3828:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a382c:
 jb .L4a3836
.L4a382e:
 push   DWORD PTR [ebp-0x40]
.L4a3831:
 call   ebx
.L4a3833:
 add    esp,0x4
.L4a3836:
 {load} mov    ecx,esi
.L4a3838:
 call target_4a5c40
.L4a383d:
 {load} xor    al,al
.L4a383f:
 jmp .L4a3a17
.L4a3844:
 {load} mov    ecx,esi
.L4a3846:
 call .L4a3630
.L4a384b:
 jmp .L4a3a17
.L4a3850:
 {load} mov    ecx,esi
.L4a3852:
 call target_4a5240
.L4a3857:
 {load} mov    ecx,esi
.L4a3859:
 test   al,al
.L4a385b:
 je .L4a3838
.L4a385d:
 call target_4a5530
.L4a3862:
 test   al,al
.L4a3864:
 je .L4a3836
.L4a3866:
 mov    al,BYTE PTR [esi+0x9e]
.L4a386c:
 lea    ecx,[esi+0x214]
.L4a3872:
 push   0x60000
.L4a3877:
 and    al,0x1
.L4a3879:
 push   0x1000000
.L4a387e:
 movzx  eax,al
.L4a3881:
 push   eax
.L4a3882:
 push   DWORD PTR [ebx]
.L4a3884:
 call target_5429a0
.L4a3889:
 test   eax,eax
.L4a388b:
 jns .L4a38de
.L4a388d:
 {load} mov    edx,eax
.L4a388f:
 lea    ecx,[ebp-0x28]
.L4a3892:
 call target_44f380
.L4a3897:
 push   eax
.L4a3898:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a389f:
 push   0x9fc520
.L4a38a4:
 lea    eax,[ebp-0x40]
.L4a38a7:
 push   eax
.L4a38a8:
 call target_4b2d20
.L4a38ad:
 add    esp,0xc
.L4a38b0:
 lea    ecx,[esi+0x54]
.L4a38b3:
 push   eax
.L4a38b4:
 call target_440a20
.L4a38b9:
 lea    ecx,[ebp-0x40]
.L4a38bc:
 call target_415b60
.L4a38c1:
 lea    ecx,[ebp-0x28]
.L4a38c4:
 mov    DWORD PTR [ebp-0x4],0xffffffff
.L4a38cb:
 call target_415b60
.L4a38d0:
 {load} mov    ecx,esi
.L4a38d2:
 call target_4a5c40
.L4a38d7:
 {load} xor    al,al
.L4a38d9:
 jmp .L4a3a17
.L4a38de:
 {load} mov    ecx,esi
.L4a38e0:
 call target_4a3a40
.L4a38e5:
 test   al,al
.L4a38e7:
 je .L4a383d
.L4a38ed:
 mov    eax,[0xbfcc44]
.L4a38f2:
 {load} xor    ecx,ecx
.L4a38f4:
 cmp    DWORD PTR [eax+0x7c],ecx
.L4a38f7:
 lea    eax,[esi+0x204]
.L4a38fd:
 push   eax
.L4a38fe:
 push   0x0
.L4a3900:
 push   0x0
.L4a3902:
 push   0x0
.L4a3904:
 push   0xffffffff
.L4a3906:
 push   0xffffffff
.L4a3908:
 sete   cl
.L4a390b:
 push   ecx
.L4a390c:
 push   0x0
.L4a390e:
 push   0x0
.L4a3910:
 push   0x1
.L4a3912:
 push   0xfffffffe
.L4a3914:
 push   0xfffffffe
.L4a3916:
 push   0x15
.L4a3918:
 push   0xbaaf88
.L4a391d:
 push   DWORD PTR [ebx]
.L4a391f:
 mov    DWORD PTR [ebp-0x74],ecx
.L4a3922:
 call   DWORD PTR [0x92d668]
.L4a3928:
 test   eax,eax
.L4a392a:
 jns .L4a3948
.L4a392c:
 {load} mov    edx,eax
.L4a392e:
 lea    ecx,[ebp-0x28]
.L4a3931:
 call target_44f380
.L4a3936:
 push   eax
.L4a3937:
 mov    DWORD PTR [ebp-0x4],0x2
.L4a393e:
 push   0x9fc4e4
.L4a3943:
 jmp .L4a38a4
.L4a3948:
 mov    edi,DWORD PTR [ebp-0x74]
.L4a394b:
 lea    eax,[esi+0x28c]
.L4a3951:
 push   eax
.L4a3952:
 push   0x0
.L4a3954:
 push   0x0
.L4a3956:
 push   0x0
.L4a3958:
 push   0xffffffff
.L4a395a:
 push   0xffffffff
.L4a395c:
 push   edi
.L4a395d:
 push   0x0
.L4a395f:
 push   0x0
.L4a3961:
 push   0x1
.L4a3963:
 push   0xfffffffe
.L4a3965:
 push   0xfffffffe
.L4a3967:
 push   0x1840
.L4a396c:
 push   0xbaafa0
.L4a3971:
 push   DWORD PTR [ebx]
.L4a3973:
 call   DWORD PTR [0x92d668]
.L4a3979:
 test   eax,eax
.L4a397b:
 jns .L4a3999
.L4a397d:
 {load} mov    edx,eax
.L4a397f:
 lea    ecx,[ebp-0x28]
.L4a3982:
 call target_44f380
.L4a3987:
 push   eax
.L4a3988:
 mov    DWORD PTR [ebp-0x4],0x3
.L4a398f:
 push   0x9fc4a4
.L4a3994:
 jmp .L4a38a4
.L4a3999:
 lea    eax,[esi+0x290]
.L4a399f:
 push   eax
.L4a39a0:
 push   0x0
.L4a39a2:
 push   0x0
.L4a39a4:
 push   0x0
.L4a39a6:
 push   0xffffffff
.L4a39a8:
 push   0xffffffff
.L4a39aa:
 push   edi
.L4a39ab:
 push   0x0
.L4a39ad:
 push   0x0
.L4a39af:
 push   0x1
.L4a39b1:
 push   0xfffffffe
.L4a39b3:
 push   0xfffffffe
.L4a39b5:
 push   0x2c0
.L4a39ba:
 push   0xbaacc8
.L4a39bf:
 push   DWORD PTR [ebx]
.L4a39c1:
 call   DWORD PTR [0x92d668]
.L4a39c7:
 test   eax,eax
.L4a39c9:
 jns .L4a3a07
.L4a39cb:
 {load} mov    edx,eax
.L4a39cd:
 lea    ecx,[ebp-0x70]
.L4a39d0:
 call target_44f380
.L4a39d5:
 push   eax
.L4a39d6:
 lea    eax,[ebp-0x58]
.L4a39d9:
 mov    DWORD PTR [ebp-0x4],0x4
.L4a39e0:
 push   0x9fc5b0
.L4a39e5:
 push   eax
.L4a39e6:
 call target_4b2d20
.L4a39eb:
 add    esp,0xc
.L4a39ee:
 lea    ecx,[esi+0x54]
.L4a39f1:
 push   eax
.L4a39f2:
 call target_440a20
.L4a39f7:
 lea    ecx,[ebp-0x58]
.L4a39fa:
 call target_415b60
.L4a39ff:
 lea    ecx,[ebp-0x70]
.L4a3a02:
 jmp .L4a38c4
.L4a3a07:
 {load} mov    ecx,esi
.L4a3a09:
 call target_4a4c50
.L4a3a0e:
 {load} mov    ecx,esi
.L4a3a10:
 call target_4a5e80
.L4a3a15:
 mov    al,0x1
.L4a3a17:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a3a1a:
 mov    DWORD PTR fs:0x0,ecx
.L4a3a21:
 pop    ecx
.L4a3a22:
 pop    edi
.L4a3a23:
 pop    esi
.L4a3a24:
 pop    ebx
.L4a3a25:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a3a28:
 {load} xor    ecx,ebp
.L4a3a2a:
 call target_8f8958
.L4a3a2f:
 {load} mov    esp,ebp
.L4a3a31:
 pop    ebp
.L4a3a32:
 ret
.equ target_415b60,0x415b60
.equ target_416fa0,0x416fa0
.equ target_440a20,0x440a20
.equ target_44f380,0x44f380
.equ target_4a22e0,0x4a22e0
.equ target_4a33f0,0x4a33f0
.equ target_4a3a40,0x4a3a40
.equ target_4a4c50,0x4a4c50
.equ target_4a5240,0x4a5240
.equ target_4a5530,0x4a5530
.equ target_4a5c40,0x4a5c40
.equ target_4a5e80,0x4a5e80
.equ target_4b2d20,0x4b2d20
.equ target_5429a0,0x5429a0
.equ target_8f8958,0x8f8958
