.code32
.section .text
.global _start
_start:
 fninit
 mov $input, %edi
 xor %esi,%esi
read_more:
 mov $3,%eax
 xor %ebx,%ebx
 lea (%edi,%esi),%ecx
 mov $65536,%edx
 sub %esi,%edx
 int $0x80
 test %eax,%eax
 jle ready
 add %eax,%esi
 jmp read_more
ready:
 mov $sqrt_stub, %eax
 mov %eax,0x92d498
 cmpl $4,input
 je random_sequence
 cmpl $5,input
 je matrix_product
 cmpl $3,input
 je orientation
 mov $input+40,%eax
 mov %eax,spline
 mov input+4,%edx
 imul $12,%edx
 add %edx,%eax
 mov %eax,spline+4
 mov %eax,spline+8
 mov $spline,%ecx
 push input+8
 push $output
 mov $0x5d0030,%eax
 cmpl $1,input
 je call_spline
 mov $0x5d0360,%eax
call_spline:
 call *%eax
 mov $12,%edx
 jmp write_out
orientation:
 push $input+28
 push $input+16
 mov $output,%ecx
 mov $0x4582a0,%eax
 call *%eax
 mov $36,%edx
 jmp write_out
random_sequence:
 mov $rng_state,%eax
 mov %eax,rng+12
 mov $rng,%ecx
 push input+4
 mov $0x478ff0,%eax
 call *%eax
 xor %edi,%edi
random_next:
 mov $rng,%ecx
 mov input+12,%eax
 cmpl $1,%eax
 je random_float
 cmpl $2,%eax
 je random_range
 cmpl $3,%eax
 je random_sign
 mov $0x479000,%eax
 call *%eax
 mov %eax,output(,%edi,4)
 jmp random_done
random_float:
 push input+20
 push input+16
 mov $0x479150,%eax
 call *%eax
 fstps output(,%edi,4)
 jmp random_done
random_range:
 push input+20
 push input+16
 mov $0x4790f0,%eax
 call *%eax
 mov %eax,output(,%edi,4)
 jmp random_done
random_sign:
 mov $0x4790b0,%eax
 call *%eax
 fstps output(,%edi,4)
random_done:
 inc %edi
 cmp input+8,%edi
 jb random_next
 lea (,%edi,4),%edx
 jmp write_out
matrix_product:
 push $input+64
 mov $input+16,%ecx
 mov $0x459300,%eax
 call *%eax
 mov $input+16,%esi
 mov $output,%edi
 mov $12,%ecx
 rep movsl
 mov $48,%edx
write_out:
 mov $4,%eax
 mov $1,%ebx
 mov $output,%ecx
 int $0x80
 mov $1,%eax
 xor %ebx,%ebx
 int $0x80
sqrt_stub:
 sqrtsd %xmm0,%xmm0
 ret
.section .bss
.align 16
input: .skip 65536
output: .skip 32768
spline: .skip 12
rng: .skip 16
rng_state: .skip 2496
