OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . = 1M;

    .text ALIGN(16) :
    {
        KEEP(*(.multiboot))
        *(.text*)
    } :text

    .rodata ALIGN(16) :
    {
        *(.rodata*)
    } :rodata

    .data ALIGN(16) :
    {
        *(.data*)
    } :data

    .bss ALIGN(4096) :
    {
        *(COMMON)
        *(.bss*)
    } :data
    __kernel_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
