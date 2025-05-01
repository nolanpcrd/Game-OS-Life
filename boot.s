.section .text.boot
.global _start

_start:
    ldr x0, =stack_top
    mov sp, x0
    bl kernel_main

halt:
    wfe
    b halt

.section .bss.stack, "aw", @nobits
.align 4
stack_bottom:
.space 4096
stack_top: