OUTPUT_ARCH(mips)
ENTRY(kernel_entry)

MEMORY
{
 dram (wx) : ORIGIN = 0xc0100000, LENGTH = 1M
}

stack_size = 65536;
heap_size = 65536;

PHDRS
{
 text PT_LOAD FLAGS(7); /* RWX */
 note PT_NOTE FLAGS(4); /* R__ */
}

SECTIONS
{
 . = ALIGN(8);

 __text_start__ = .;
 .text : {
  *(.text.*)
  *(.rodata.*)
 } :text = 0
 __text_end__ = .;

 . = ALIGN(8);

 __bss_start__ = .;
 .bss : {
  *(.bss.*)
 }
 __bss_end__ = .;

 . = ALIGN(8);

 __heap_start__ = .;
 .heap : {
  . = . + heap_size;
 }
 __heap_end__ = .;

 . = ALIGN(8);

 .stack : {
  __stack_start__ = .;
  . = . + stack_size;
  __stack_end__ = .;
  . = . + 8;
 }
}

