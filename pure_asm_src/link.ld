ENTRY(_start)

SECTIONS
{
  . = 0x40080000;

  .text : {
    *(.text._start)
    *(.text*)
  }

  .rodata : ALIGN(8) {
    *(.rodata*)
  }

  .bss (NOLOAD) : ALIGN(8) {
    __bss_start = .;
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  }

  /* Simple static stack for core 0 */
  . = ALIGN(16);
  __stack_top_core0 = . + 0x1000;
}