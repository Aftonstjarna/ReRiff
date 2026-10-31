#!/usr/bin/env bash
set -euo pipefail
drafter=$(realpath "$1")
draft_test_dir=$(mktemp -d)
trap 'rm -rf "$draft_test_dir"' EXIT
cat > "$draft_test_dir/input.S" <<'ASM'
.intel_syntax noprefix
.code32
.global fixture
.text
fixture:
 {load} mov ebp,esp
 mov eax,DWORD PTR [0x12345678]
 mov DWORD PTR [0x12345678],eax
 {load} xor edx,edx
 test eax,eax
 je done
 call outside
 {disp8} lea ecx,[ecx+0]
 {disp32} lea ebx,[ebx+0]
 call DWORD PTR [0x12345678]
done:
 ret
.equ outside,0x401234
ASM
as --32 "$draft_test_dir/input.S" -o "$draft_test_dir/input.o"
ld -m elf_i386 -Ttext=0x400000 -e fixture "$draft_test_dir/input.o" -o "$draft_test_dir/input.elf"
objcopy -O binary --only-section=.text "$draft_test_dir/input.elf" "$draft_test_dir/input.bin"
objdump -d -Mintel --insn-width=16 "$draft_test_dir/input.elf" > "$draft_test_dir/listing"
draft_test_size=$(stat -c %s "$draft_test_dir/input.bin")
printf -v draft_test_end '%x' "$((0x400000 + draft_test_size))"
"$drafter" "$draft_test_dir/listing" 400000 "$draft_test_end" generated "$draft_test_dir/output.S"
as --32 "$draft_test_dir/output.S" -o "$draft_test_dir/output.o"
ld -m elf_i386 -Ttext=0x400000 -e generated "$draft_test_dir/output.o" -o "$draft_test_dir/output.elf"
objcopy -O binary --only-section=.text "$draft_test_dir/output.elf" "$draft_test_dir/output.bin"
cmp "$draft_test_dir/input.bin" "$draft_test_dir/output.bin"
if "$drafter" "$draft_test_dir/listing" 400000 "$draft_test_end" generated "$draft_test_dir/output.S" 2>/dev/null; then
  echo 'existing output was not protected' >&2; exit 1
fi
if "$drafter" "$draft_test_dir/listing" 400000 400001 generated "$draft_test_dir/truncated.S" 2>/dev/null; then
  echo 'truncated instruction was accepted' >&2; exit 1
fi
sed '/400002:/d' "$draft_test_dir/listing" > "$draft_test_dir/gap"
if "$drafter" "$draft_test_dir/gap" 400000 "$draft_test_end" generated "$draft_test_dir/gap.S" 2>/dev/null; then
  echo 'listing gap was accepted' >&2; exit 1
fi
echo 'Assembly drafting roundtrip and rejection checks passed'
