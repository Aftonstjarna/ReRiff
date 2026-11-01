ENTRY(_start)
SECTIONS {
 . = 0x400000;
 .original : { image.o(.data) }
 . = 0x10000000;
 .text : { harness.o(.text) }
 . = 0x20000000;
 .bss : { harness.o(.bss) }
}
