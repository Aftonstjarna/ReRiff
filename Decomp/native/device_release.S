# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a6330..0x4a6551
.intel_syntax noprefix
.code32
.text
.global original_device_release
original_device_release:
.L4a6330:
 push   ebp
.L4a6331:
 {load} mov    ebp,esp
.L4a6333:
 push   0xffffffff
.L4a6335:
 push   0x902b88
.L4a633a:
 mov    eax,fs:0x0
.L4a6340:
 push   eax
.L4a6341:
 push   ecx
.L4a6342:
 push   ebx
.L4a6343:
 push   esi
.L4a6344:
 push   edi
.L4a6345:
 mov    eax,[0xbaabc0]
.L4a634a:
 {load} xor    eax,ebp
.L4a634c:
 push   eax
.L4a634d:
 lea    eax,[ebp-0xc]
.L4a6350:
 mov    fs:0x0,eax
.L4a6356:
 {load} mov    esi,ecx
.L4a6358:
 cmp    DWORD PTR [esi+0x7c],0x0
.L4a635c:
 jne .L4a653f
.L4a6362:
 push   ecx
.L4a6363:
 call target_4add20
.L4a6368:
 lea    eax,[esi+0xa84]
.L4a636e:
 push   eax
.L4a636f:
 mov    DWORD PTR [ebp-0x10],eax
.L4a6372:
 call   DWORD PTR [0x92d024]
.L4a6378:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a637f:
 {load} xor    edi,edi
.L4a6381:
 mov    eax,DWORD PTR [esi+0xa7c]
.L4a6387:
 sub    eax,DWORD PTR [esi+0xa78]
.L4a638d:
 sar    eax,0x2
.L4a6390:
 test   eax,eax
.L4a6392:
 je .L4a63b6
.L4a6394:
 mov    eax,DWORD PTR [esi+0xa78]
.L4a639a:
 mov    ecx,DWORD PTR [eax+edi*4]
.L4a639d:
 mov    eax,DWORD PTR [ecx]
.L4a639f:
 call   DWORD PTR [eax+0x4]
.L4a63a2:
 mov    eax,DWORD PTR [esi+0xa7c]
.L4a63a8:
 inc    edi
.L4a63a9:
 sub    eax,DWORD PTR [esi+0xa78]
.L4a63af:
 sar    eax,0x2
.L4a63b2:
 {load} cmp    edi,eax
.L4a63b4:
 jb .L4a6394
.L4a63b6:
 mov    ecx,DWORD PTR [0xbfcd0c]
.L4a63bc:
 mov    eax,DWORD PTR [ecx]
.L4a63be:
 call   DWORD PTR [eax+0x4]
.L4a63c1:
 {load} mov    ecx,esi
.L4a63c3:
 call target_4a4d20
.L4a63c8:
 lea    edi,[esi+0x420]
.L4a63ce:
 mov    ebx,0x2
.L4a63d3:
 mov    ecx,DWORD PTR [edi+0x8]
.L4a63d6:
 test   ecx,ecx
.L4a63d8:
 je .L4a63e0
.L4a63da:
 mov    eax,DWORD PTR [ecx]
.L4a63dc:
 push   ecx
.L4a63dd:
 call   DWORD PTR [eax+0x8]
.L4a63e0:
 mov    ecx,DWORD PTR [edi]
.L4a63e2:
 mov    DWORD PTR [edi+0x8],0x0
.L4a63e9:
 test   ecx,ecx
.L4a63eb:
 je .L4a63f3
.L4a63ed:
 mov    eax,DWORD PTR [ecx]
.L4a63ef:
 push   ecx
.L4a63f0:
 call   DWORD PTR [eax+0x8]
.L4a63f3:
 mov    DWORD PTR [edi],0x0
.L4a63f9:
 add    edi,0x4
.L4a63fc:
 dec    ebx
.L4a63fd:
 jne .L4a63d3
.L4a63ff:
 mov    ecx,DWORD PTR [esi+0x430]
.L4a6405:
 test   ecx,ecx
.L4a6407:
 je .L4a640f
.L4a6409:
 mov    eax,DWORD PTR [ecx]
.L4a640b:
 push   ecx
.L4a640c:
 call   DWORD PTR [eax+0x8]
.L4a640f:
 mov    ecx,DWORD PTR [esi+0x438]
.L4a6415:
 mov    DWORD PTR [esi+0x430],0x0
.L4a641f:
 test   ecx,ecx
.L4a6421:
 je .L4a6429
.L4a6423:
 mov    eax,DWORD PTR [ecx]
.L4a6425:
 push   ecx
.L4a6426:
 call   DWORD PTR [eax+0x8]
.L4a6429:
 mov    ecx,DWORD PTR [esi+0x434]
.L4a642f:
 mov    DWORD PTR [esi+0x438],0x0
.L4a6439:
 test   ecx,ecx
.L4a643b:
 je .L4a6443
.L4a643d:
 mov    eax,DWORD PTR [ecx]
.L4a643f:
 push   ecx
.L4a6440:
 call   DWORD PTR [eax+0x8]
.L4a6443:
 mov    ecx,DWORD PTR [esi+0x440]
.L4a6449:
 mov    DWORD PTR [esi+0x434],0x0
.L4a6453:
 test   ecx,ecx
.L4a6455:
 je .L4a645d
.L4a6457:
 mov    eax,DWORD PTR [ecx]
.L4a6459:
 push   ecx
.L4a645a:
 call   DWORD PTR [eax+0x8]
.L4a645d:
 mov    ecx,DWORD PTR [esi+0x43c]
.L4a6463:
 mov    DWORD PTR [esi+0x440],0x0
.L4a646d:
 test   ecx,ecx
.L4a646f:
 je .L4a6477
.L4a6471:
 mov    eax,DWORD PTR [ecx]
.L4a6473:
 push   ecx
.L4a6474:
 call   DWORD PTR [eax+0x8]
.L4a6477:
 mov    ecx,DWORD PTR [esi+0x448]
.L4a647d:
 mov    DWORD PTR [esi+0x43c],0x0
.L4a6487:
 test   ecx,ecx
.L4a6489:
 je .L4a6491
.L4a648b:
 mov    eax,DWORD PTR [ecx]
.L4a648d:
 push   ecx
.L4a648e:
 call   DWORD PTR [eax+0x8]
.L4a6491:
 mov    ecx,DWORD PTR [esi+0x444]
.L4a6497:
 mov    DWORD PTR [esi+0x448],0x0
.L4a64a1:
 test   ecx,ecx
.L4a64a3:
 je .L4a64ab
.L4a64a5:
 mov    eax,DWORD PTR [ecx]
.L4a64a7:
 push   ecx
.L4a64a8:
 call   DWORD PTR [eax+0x8]
.L4a64ab:
 mov    DWORD PTR [esi+0x444],0x0
.L4a64b5:
 mov    ecx,DWORD PTR [esi+0x21c]
.L4a64bb:
 test   ecx,ecx
.L4a64bd:
 je .L4a64c5
.L4a64bf:
 mov    eax,DWORD PTR [ecx]
.L4a64c1:
 push   ecx
.L4a64c2:
 call   DWORD PTR [eax+0x8]
.L4a64c5:
 mov    ecx,DWORD PTR [esi+0x218]
.L4a64cb:
 mov    DWORD PTR [esi+0x21c],0x0
.L4a64d5:
 test   ecx,ecx
.L4a64d7:
 je .L4a64df
.L4a64d9:
 mov    eax,DWORD PTR [ecx]
.L4a64db:
 push   ecx
.L4a64dc:
 call   DWORD PTR [eax+0x8]
.L4a64df:
 mov    ecx,DWORD PTR [esi+0x214]
.L4a64e5:
 mov    DWORD PTR [esi+0x218],0x0
.L4a64ef:
 test   ecx,ecx
.L4a64f1:
 je .L4a64f9
.L4a64f3:
 mov    eax,DWORD PTR [ecx]
.L4a64f5:
 push   ecx
.L4a64f6:
 call   DWORD PTR [eax+0x8]
.L4a64f9:
 lea    eax,[esi+0xa84]
.L4a64ff:
 mov    DWORD PTR [esi+0x214],0x0
.L4a6509:
 mov    DWORD PTR [esi+0x220],0x0
.L4a6513:
 mov    DWORD PTR [esi+0x224],0x0
.L4a651d:
 mov    DWORD PTR [esi+0x228],0x0
.L4a6527:
 mov    DWORD PTR [esi+0x22c],0x0
.L4a6531:
 push   eax
.L4a6532:
 mov    BYTE PTR [esi+0xa74],0x1
.L4a6539:
 call   DWORD PTR [0x92d028]
.L4a653f:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a6542:
 mov    DWORD PTR fs:0x0,ecx
.L4a6549:
 pop    ecx
.L4a654a:
 pop    edi
.L4a654b:
 pop    esi
.L4a654c:
 pop    ebx
.L4a654d:
 {load} mov    esp,ebp
.L4a654f:
 pop    ebp
.L4a6550:
 ret
.equ target_4a4d20,0x4a4d20
.equ target_4add20,0x4add20
