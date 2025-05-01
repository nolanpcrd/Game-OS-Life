OUTPUT_ARCH(aarch64)
ENTRY(_start)

_kernel_start = 0x80000;

SECTIONS
{
    . = _kernel_start;

    .text.boot : ALIGN(4K)
    {
        KEEP(*(.text.boot))
    }

    .vectors : ALIGN(2K)
    {
        KEEP(*(.vectors))
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .bss : ALIGN(4K)
    {
        _bss_start = .;
        *(.bss)
        _bss_end = .;
    }

    .bss.stack : ALIGN(16)
    {
        stack_bottom = .;
        *(.bss.stack)
        stack_top = .;
    }

    _end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note.gnu.build-id)
        *(.ARM.attributes)
    }
}