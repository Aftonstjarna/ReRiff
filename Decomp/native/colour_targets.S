# Reviewed against original instructions and Ghidra control flow; native-check verifies bytes.
# Original VA range 0x4a5240..0x4a5530
.intel_syntax noprefix
.code32
.text
.global original_colour_targets
original_colour_targets:
.L4a5240:
 push   ebp
.L4a5241:
 {load} mov    ebp,esp
.L4a5243:
 push   0xffffffff
.L4a5245:
 push   0x902b18
.L4a524a:
 mov    eax,fs:0x0
.L4a5250:
 push   eax
.L4a5251:
 sub    esp,0x6c
.L4a5254:
 mov    eax,[0xbaabc0]
.L4a5259:
 {load} xor    eax,ebp
.L4a525b:
 mov    DWORD PTR [ebp-0x10],eax
.L4a525e:
 push   ebx
.L4a525f:
 push   esi
.L4a5260:
 push   edi
.L4a5261:
 push   eax
.L4a5262:
 lea    eax,[ebp-0xc]
.L4a5265:
 mov    fs:0x0,eax
.L4a526b:
 {load} mov    ebx,ecx
.L4a526d:
 mov    edx,DWORD PTR [ebx+0x23c]
.L4a5273:
 lea    esi,[ebx+0x420]
.L4a5279:
 mov    eax,DWORD PTR [ebx+0x240]
.L4a527f:
 {load} xor    edi,edi
.L4a5281:
 mov    DWORD PTR [ebp-0x74],edx
.L4a5284:
 mov    DWORD PTR [ebp-0x78],eax
.L4a5287:
 mov    eax,DWORD PTR [ebx+0x78]
.L4a528a:
 push   0x0
.L4a528c:
 push   esi
.L4a528d:
 push   0x0
.L4a528f:
 push   DWORD PTR [ebx+0x1f8]
.L4a5295:
 mov    ecx,DWORD PTR [eax]
.L4a5297:
 push   0x1
.L4a5299:
 push   0x1
.L4a529b:
 push   DWORD PTR [ebp-0x78]
.L4a529e:
 push   edx
.L4a529f:
 push   eax
.L4a52a0:
 call   DWORD PTR [ecx+0x5c]
.L4a52a3:
 test   eax,eax
.L4a52a5:
 js .L4a5453
.L4a52ab:
 mov    ecx,DWORD PTR [esi]
.L4a52ad:
 lea    eax,[esi+0x8]
.L4a52b0:
 push   eax
.L4a52b1:
 push   0x0
.L4a52b3:
 push   ecx
.L4a52b4:
 mov    edx,DWORD PTR [ecx]
.L4a52b6:
 call   DWORD PTR [edx+0x48]
.L4a52b9:
 test   eax,eax
.L4a52bb:
 js .L4a53c3
.L4a52c1:
 mov    edx,DWORD PTR [ebp-0x74]
.L4a52c4:
 inc    edi
.L4a52c5:
 add    esi,0x4
.L4a52c8:
 cmp    edi,0x2
.L4a52cb:
 jb .L4a5287
.L4a52cd:
 mov    ecx,DWORD PTR [ebx+0x78]
.L4a52d0:
 lea    eax,[ebx+0x430]
.L4a52d6:
 push   0x0
.L4a52d8:
 push   eax
.L4a52d9:
 push   0x1
.L4a52db:
 mov    edx,DWORD PTR [ecx]
.L4a52dd:
 push   0x0
.L4a52df:
 push   0x0
.L4a52e1:
 push   DWORD PTR [ebx+0x1e0]
.L4a52e7:
 push   DWORD PTR [ebp-0x78]
.L4a52ea:
 push   DWORD PTR [ebp-0x74]
.L4a52ed:
 push   ecx
.L4a52ee:
 call   DWORD PTR [edx+0x74]
.L4a52f1:
 test   eax,eax
.L4a52f3:
 jns .L4a53bc
.L4a52f9:
 {load} mov    edx,eax
.L4a52fb:
 lea    ecx,[ebp-0x70]
.L4a52fe:
 call target_44f380
.L4a5303:
 push   eax
.L4a5304:
 lea    eax,[ebp-0x40]
.L4a5307:
 mov    DWORD PTR [ebp-0x4],0x2
.L4a530e:
 push   0x9fc624
.L4a5313:
 push   eax
.L4a5314:
 call target_4b2d20
.L4a5319:
 lea    esi,[ebx+0x54]
.L4a531c:
 {load} mov    edi,eax
.L4a531e:
 mov    ebx,DWORD PTR [0x92d4b0]
.L4a5324:
 add    esp,0xc
.L4a5327:
 {load} cmp    esi,edi
.L4a5329:
 je .L4a538a
.L4a532b:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a532f:
 jb .L4a5338
.L4a5331:
 push   DWORD PTR [esi]
.L4a5333:
 call   ebx
.L4a5335:
 add    esp,0x4
.L4a5338:
 mov    DWORD PTR [esi+0x14],0xf
.L4a533f:
 mov    DWORD PTR [esi+0x10],0x0
.L4a5346:
 mov    BYTE PTR [esi],0x0
.L4a5349:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a534d:
 jae .L4a5363
.L4a534f:
 mov    eax,DWORD PTR [edi+0x10]
.L4a5352:
 inc    eax
.L4a5353:
 je .L4a536d
.L4a5355:
 push   eax
.L4a5356:
 push   edi
.L4a5357:
 push   esi
.L4a5358:
 call   DWORD PTR [0x92d4a4]
.L4a535e:
 add    esp,0xc
.L4a5361:
 jmp .L4a536d
.L4a5363:
 mov    eax,DWORD PTR [edi]
.L4a5365:
 mov    DWORD PTR [esi],eax
.L4a5367:
 mov    DWORD PTR [edi],0x0
.L4a536d:
 mov    eax,DWORD PTR [edi+0x10]
.L4a5370:
 mov    DWORD PTR [esi+0x10],eax
.L4a5373:
 mov    eax,DWORD PTR [edi+0x14]
.L4a5376:
 mov    DWORD PTR [esi+0x14],eax
.L4a5379:
 mov    DWORD PTR [edi+0x14],0xf
.L4a5380:
 mov    DWORD PTR [edi+0x10],0x0
.L4a5387:
 mov    BYTE PTR [edi],0x0
.L4a538a:
 cmp    DWORD PTR [ebp-0x2c],0x10
.L4a538e:
 jb .L4a5398
.L4a5390:
 push   DWORD PTR [ebp-0x40]
.L4a5393:
 call   ebx
.L4a5395:
 add    esp,0x4
.L4a5398:
 cmp    DWORD PTR [ebp-0x5c],0x10
.L4a539c:
 mov    DWORD PTR [ebp-0x2c],0xf
.L4a53a3:
 mov    DWORD PTR [ebp-0x30],0x0
.L4a53aa:
 mov    BYTE PTR [ebp-0x40],0x0
.L4a53ae:
 jb .L4a5512
.L4a53b4:
 push   DWORD PTR [ebp-0x70]
.L4a53b7:
 jmp .L4a550d
.L4a53bc:
 mov    al,0x1
.L4a53be:
 jmp .L4a5514
.L4a53c3:
 {load} mov    edx,eax
.L4a53c5:
 lea    ecx,[ebp-0x58]
.L4a53c8:
 call target_44f380
.L4a53cd:
 push   eax
.L4a53ce:
 lea    eax,[ebp-0x28]
.L4a53d1:
 mov    DWORD PTR [ebp-0x4],0x1
.L4a53d8:
 push   0x9fc654
.L4a53dd:
 push   eax
.L4a53de:
 call target_4b2d20
.L4a53e3:
 mov    edi,DWORD PTR [0x92d4b0]
.L4a53e9:
 lea    esi,[ebx+0x54]
.L4a53ec:
 add    esp,0xc
.L4a53ef:
 mov    DWORD PTR [ebp-0x74],eax
.L4a53f2:
 {load} cmp    esi,eax
.L4a53f4:
 je .L4a541f
.L4a53f6:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a53fa:
 jb .L4a5406
.L4a53fc:
 push   DWORD PTR [esi]
.L4a53fe:
 call   edi
.L4a5400:
 mov    eax,DWORD PTR [ebp-0x74]
.L4a5403:
 add    esp,0x4
.L4a5406:
 mov    DWORD PTR [esi+0x14],0xf
.L4a540d:
 {load} mov    ecx,esi
.L4a540f:
 mov    DWORD PTR [esi+0x10],0x0
.L4a5416:
 push   eax
.L4a5417:
 mov    BYTE PTR [esi],0x0
.L4a541a:
 call target_416fa0
.L4a541f:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a5423:
 jb .L4a542d
.L4a5425:
 push   DWORD PTR [ebp-0x28]
.L4a5428:
 call   edi
.L4a542a:
 add    esp,0x4
.L4a542d:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a5431:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a5438:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a543f:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a5443:
 jb .L4a5512
.L4a5449:
 push   DWORD PTR [ebp-0x58]
.L4a544c:
 call   edi
.L4a544e:
 jmp .L4a550f
.L4a5453:
 {load} mov    edx,eax
.L4a5455:
 lea    ecx,[ebp-0x58]
.L4a5458:
 call target_44f380
.L4a545d:
 push   eax
.L4a545e:
 lea    eax,[ebp-0x28]
.L4a5461:
 mov    DWORD PTR [ebp-0x4],0x0
.L4a5468:
 push   0x9fc690
.L4a546d:
 push   eax
.L4a546e:
 call target_4b2d20
.L4a5473:
 lea    esi,[ebx+0x54]
.L4a5476:
 {load} mov    edi,eax
.L4a5478:
 mov    ebx,DWORD PTR [0x92d4b0]
.L4a547e:
 add    esp,0xc
.L4a5481:
 {load} cmp    esi,edi
.L4a5483:
 je .L4a54e4
.L4a5485:
 cmp    DWORD PTR [esi+0x14],0x10
.L4a5489:
 jb .L4a5492
.L4a548b:
 push   DWORD PTR [esi]
.L4a548d:
 call   ebx
.L4a548f:
 add    esp,0x4
.L4a5492:
 mov    DWORD PTR [esi+0x14],0xf
.L4a5499:
 mov    DWORD PTR [esi+0x10],0x0
.L4a54a0:
 mov    BYTE PTR [esi],0x0
.L4a54a3:
 cmp    DWORD PTR [edi+0x14],0x10
.L4a54a7:
 jae .L4a54bd
.L4a54a9:
 mov    eax,DWORD PTR [edi+0x10]
.L4a54ac:
 inc    eax
.L4a54ad:
 je .L4a54c7
.L4a54af:
 push   eax
.L4a54b0:
 push   edi
.L4a54b1:
 push   esi
.L4a54b2:
 call   DWORD PTR [0x92d4a4]
.L4a54b8:
 add    esp,0xc
.L4a54bb:
 jmp .L4a54c7
.L4a54bd:
 mov    eax,DWORD PTR [edi]
.L4a54bf:
 mov    DWORD PTR [esi],eax
.L4a54c1:
 mov    DWORD PTR [edi],0x0
.L4a54c7:
 mov    eax,DWORD PTR [edi+0x10]
.L4a54ca:
 mov    DWORD PTR [esi+0x10],eax
.L4a54cd:
 mov    eax,DWORD PTR [edi+0x14]
.L4a54d0:
 mov    DWORD PTR [esi+0x14],eax
.L4a54d3:
 mov    DWORD PTR [edi+0x14],0xf
.L4a54da:
 mov    DWORD PTR [edi+0x10],0x0
.L4a54e1:
 mov    BYTE PTR [edi],0x0
.L4a54e4:
 cmp    DWORD PTR [ebp-0x14],0x10
.L4a54e8:
 jb .L4a54f2
.L4a54ea:
 push   DWORD PTR [ebp-0x28]
.L4a54ed:
 call   ebx
.L4a54ef:
 add    esp,0x4
.L4a54f2:
 cmp    DWORD PTR [ebp-0x44],0x10
.L4a54f6:
 mov    DWORD PTR [ebp-0x14],0xf
.L4a54fd:
 mov    DWORD PTR [ebp-0x18],0x0
.L4a5504:
 mov    BYTE PTR [ebp-0x28],0x0
.L4a5508:
 jb .L4a5512
.L4a550a:
 push   DWORD PTR [ebp-0x58]
.L4a550d:
 call   ebx
.L4a550f:
 add    esp,0x4
.L4a5512:
 {load} xor    al,al
.L4a5514:
 mov    ecx,DWORD PTR [ebp-0xc]
.L4a5517:
 mov    DWORD PTR fs:0x0,ecx
.L4a551e:
 pop    ecx
.L4a551f:
 pop    edi
.L4a5520:
 pop    esi
.L4a5521:
 pop    ebx
.L4a5522:
 mov    ecx,DWORD PTR [ebp-0x10]
.L4a5525:
 {load} xor    ecx,ebp
.L4a5527:
 call target_8f8958
.L4a552c:
 {load} mov    esp,ebp
.L4a552e:
 pop    ebp
.L4a552f:
 ret
.equ target_416fa0,0x416fa0
.equ target_44f380,0x44f380
.equ target_4b2d20,0x4b2d20
.equ target_8f8958,0x8f8958
