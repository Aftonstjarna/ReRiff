# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a33f0..0x4a362f
.intel_syntax noprefix
.code32
.text
.global original_presentation_parameters
original_presentation_parameters:
.L4a33f0:
 push   ebp
.L4a33f1:
 {load} mov    ebp,esp
.L4a33f3:
 sub    esp,0x1c
.L4a33f6:
 push   ebx
.L4a33f7:
 push   esi
.L4a33f8:
 {load} mov    esi,ecx
.L4a33fa:
 mov    DWORD PTR [ebp-0x10],0x0
.L4a3401:
 push   edi
.L4a3402:
 mov    DWORD PTR [ebp-0x14],esi
.L4a3405:
 mov    eax,0x2aaaaaab
.L4a340a:
 cmp    BYTE PTR [esi+0x4bf],0x0
.L4a3411:
 je .L4a353d
.L4a3417:
 {load} xor    ecx,ecx
.L4a3419:
 mov    DWORD PTR [ebp-0xc],0x0
.L4a3420:
 mov    DWORD PTR [ebp-0x8],ecx
.L4a3423:
 or     edi,0xffffffff
.L4a3426:
 mov    ecx,DWORD PTR [esi+0x280]
.L4a342c:
 or     ebx,0xffffffff
.L4a342f:
 sub    ecx,DWORD PTR [esi+0x27c]
.L4a3435:
 imul   ecx
.L4a3437:
 sar    edx,0x2
.L4a343a:
 {load} mov    eax,edx
.L4a343c:
 shr    eax,0x1f
.L4a343f:
 {load} add    eax,edx
.L4a3441:
 je .L4a351a
.L4a3447:
 mov    ecx,DWORD PTR [esi+0x27c]
.L4a344d:
 {load} xor    eax,eax
.L4a344f:
 mov    edx,DWORD PTR [esi+0x48c]
.L4a3455:
 mov    DWORD PTR [ebp-0x18],ecx
.L4a3458:
 mov    DWORD PTR [ebp-0x4],eax
.L4a345b:
 jmp .L4a3460
.L4a345d:
 {disp8} lea    ecx,[ecx+0x0]
.L4a3460:
 mov    ecx,DWORD PTR [eax+ecx*1]
.L4a3463:
 {load} cmp    ecx,edx
.L4a3465:
 jne .L4a3482
.L4a3467:
 mov    esi,DWORD PTR [ebp-0x18]
.L4a346a:
 mov    eax,DWORD PTR [eax+esi*1+0x4]
.L4a346e:
 mov    esi,DWORD PTR [ebp-0x14]
.L4a3471:
 cmp    eax,DWORD PTR [esi+0x490]
.L4a3477:
 je .L4a352c
.L4a347d:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a3480:
 {load} cmp    ecx,edx
.L4a3482:
 jb .L4a34a7
.L4a3484:
 mov    edx,DWORD PTR [esi+0x27c]
.L4a348a:
 mov    eax,DWORD PTR [eax+edx*1+0x4]
.L4a348e:
 cmp    eax,DWORD PTR [esi+0x490]
.L4a3494:
 jb .L4a34a7
.L4a3496:
 mov    edx,DWORD PTR [ebp-0x4]
.L4a3499:
 {load} cmp    ecx,edi
.L4a349b:
 ja .L4a34c4
.L4a349d:
 {load} cmp    eax,ebx
.L4a349f:
 ja .L4a34c4
.L4a34a1:
 {load} mov    edi,ecx
.L4a34a3:
 {load} mov    ebx,eax
.L4a34a5:
 jmp .L4a34c4
.L4a34a7:
 mov    edx,DWORD PTR [ebp-0x4]
.L4a34aa:
 cmp    ecx,DWORD PTR [ebp-0xc]
.L4a34ad:
 jb .L4a34c4
.L4a34af:
 mov    eax,DWORD PTR [esi+0x27c]
.L4a34b5:
 mov    eax,DWORD PTR [edx+eax*1+0x4]
.L4a34b9:
 cmp    eax,DWORD PTR [ebp-0x10]
.L4a34bc:
 jb .L4a34c4
.L4a34be:
 mov    DWORD PTR [ebp-0xc],ecx
.L4a34c1:
 mov    DWORD PTR [ebp-0x10],eax
.L4a34c4:
 mov    ecx,DWORD PTR [esi+0x280]
.L4a34ca:
 add    edx,0x18
.L4a34cd:
 sub    ecx,DWORD PTR [esi+0x27c]
.L4a34d3:
 mov    eax,0x2aaaaaab
.L4a34d8:
 inc    DWORD PTR [ebp-0x8]
.L4a34db:
 mov    DWORD PTR [ebp-0x4],edx
.L4a34de:
 imul   ecx
.L4a34e0:
 mov    ecx,DWORD PTR [esi+0x27c]
.L4a34e6:
 sar    edx,0x2
.L4a34e9:
 {load} mov    eax,edx
.L4a34eb:
 shr    eax,0x1f
.L4a34ee:
 {load} add    eax,edx
.L4a34f0:
 mov    edx,DWORD PTR [esi+0x48c]
.L4a34f6:
 cmp    DWORD PTR [ebp-0x8],eax
.L4a34f9:
 mov    eax,DWORD PTR [ebp-0x4]
.L4a34fc:
 jb .L4a3460
.L4a3502:
 cmp    edi,0xffffffff
.L4a3505:
 je .L4a351a
.L4a3507:
 push   ebx
.L4a3508:
 mov    DWORD PTR [esi+0x48c],edi
.L4a350e:
 mov    DWORD PTR [esi+0x490],ebx
.L4a3514:
 push   edi
.L4a3515:
 jmp .L4a35be
.L4a351a:
 mov    eax,DWORD PTR [ebp-0xc]
.L4a351d:
 mov    DWORD PTR [esi+0x48c],eax
.L4a3523:
 mov    eax,DWORD PTR [ebp-0x10]
.L4a3526:
 mov    DWORD PTR [esi+0x490],eax
.L4a352c:
 push   DWORD PTR [esi+0x490]
.L4a3532:
 push   DWORD PTR [esi+0x48c]
.L4a3538:
 jmp .L4a35be
.L4a353d:
 mov    ecx,DWORD PTR [esi+0x280]
.L4a3543:
 {load} xor    edi,edi
.L4a3545:
 sub    ecx,DWORD PTR [esi+0x27c]
.L4a354b:
 {load} xor    ebx,ebx
.L4a354d:
 imul   ecx
.L4a354f:
 sar    edx,0x2
.L4a3552:
 {load} mov    ecx,edx
.L4a3554:
 shr    ecx,0x1f
.L4a3557:
 {load} add    ecx,edx
.L4a3559:
 je .L4a35a9
.L4a355b:
 mov    eax,DWORD PTR [esi+0x27c]
.L4a3561:
 mov    ecx,DWORD PTR [esi+0x280]
.L4a3567:
 {load} sub    ecx,eax
.L4a3569:
 mov    DWORD PTR [ebp-0x10],eax
.L4a356c:
 mov    eax,0x2aaaaaab
.L4a3571:
 imul   ecx
.L4a3573:
 {load} xor    ecx,ecx
.L4a3575:
 sar    edx,0x2
.L4a3578:
 {load} mov    eax,edx
.L4a357a:
 shr    eax,0x1f
.L4a357d:
 {load} add    eax,edx
.L4a357f:
 {load} xor    esi,esi
.L4a3581:
 {load} mov    edx,eax
.L4a3583:
 mov    eax,DWORD PTR [ebp-0x10]
.L4a3586:
 mov    eax,DWORD PTR [ecx+eax*1]
.L4a3589:
 {load} cmp    eax,esi
.L4a358b:
 jb .L4a359b
.L4a358d:
 {load} mov    esi,eax
.L4a358f:
 mov    eax,DWORD PTR [ebp-0x10]
.L4a3592:
 mov    eax,DWORD PTR [ecx+eax*1+0x4]
.L4a3596:
 {load} cmp    eax,edi
.L4a3598:
 cmovae edi,eax
.L4a359b:
 inc    ebx
.L4a359c:
 add    ecx,0x18
.L4a359f:
 {load} cmp    ebx,edx
.L4a35a1:
 jb .L4a3583
.L4a35a3:
 mov    DWORD PTR [ebp-0x10],esi
.L4a35a6:
 mov    esi,DWORD PTR [ebp-0x14]
.L4a35a9:
 mov    eax,DWORD PTR [ebp-0x10]
.L4a35ac:
 cmp    DWORD PTR [esi+0x48c],eax
.L4a35b2:
 ja .L4a35bc
.L4a35b4:
 cmp    DWORD PTR [esi+0x490],edi
.L4a35ba:
 jbe .L4a35c5
.L4a35bc:
 push   edi
.L4a35bd:
 push   eax
.L4a35be:
 {load} mov    ecx,esi
.L4a35c0:
 call target_4a22e0
.L4a35c5:
 mov    eax,DWORD PTR [esi+0x274]
.L4a35cb:
 mov    edx,DWORD PTR [esi+0x48c]
.L4a35d1:
 mov    ecx,DWORD PTR [esi+0x490]
.L4a35d7:
 mov    DWORD PTR [esi+0x24c],eax
.L4a35dd:
 mov    eax,DWORD PTR [esi+0x200]
.L4a35e3:
 mov    DWORD PTR [esi+0x1c8],eax
.L4a35e9:
 {load} xor    eax,eax
.L4a35eb:
 cmp    BYTE PTR [esi+0x4bf],al
.L4a35f1:
 pop    edi
.L4a35f2:
 sete   al
.L4a35f5:
 mov    DWORD PTR [esi+0x23c],edx
.L4a35fb:
 mov    DWORD PTR [esi+0x240],ecx
.L4a3601:
 mov    DWORD PTR [esi+0x244],edx
.L4a3607:
 mov    DWORD PTR [esi+0x248],ecx
.L4a360d:
 mov    DWORD PTR [esi+0x1b8],edx
.L4a3613:
 mov    DWORD PTR [esi+0x1bc],ecx
.L4a3619:
 mov    DWORD PTR [esi+0x1d8],eax
.L4a361f:
 mov    DWORD PTR [esi+0x1e8],0x0
.L4a3629:
 pop    esi
.L4a362a:
 pop    ebx
.L4a362b:
 {load} mov    esp,ebp
.L4a362d:
 pop    ebp
.L4a362e:
 ret
.equ target_4a22e0,0x4a22e0
