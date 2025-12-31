[BITS 32]

section .text
global _start
extern _kernel_main

_start:
    ; Configurar segmentos
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Llamar al kernel en C
    call _kernel_main

    ; Si kernel_main retorna, halt
hang:
    cli
    hlt
    jmp hang