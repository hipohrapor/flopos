ENTRY(kernel_main)

SECTIONS {
    . = 1M;

    .multiboot : { *(.multiboot) }
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}

