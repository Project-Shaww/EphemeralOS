; EphemeralOS Bootloader Stage 2
; Loads the kernel and switches to protected mode

[BITS 16]
[ORG 0x7E00]

stage2_start:
    mov si, stage2_msg
    call print_string

    call enable_a20
    call load_kernel

    mov si, kernel_loaded_msg
    call print_string

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_start

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

load_kernel:
    mov ah, 0x02
    mov al, 80
    mov ch, 0
    mov cl, 17
    mov dh, 0
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13
    ret

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

stage2_msg db 'Stage 2 loaded. Loading kernel...', 13, 10, 0
kernel_loaded_msg db 'Kernel loaded. Entering protected mode...', 13, 10, 0

gdt_start:
    dq 0

gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Jump to kernel entry point
    jmp CODE_SEG:0x10000

times 7680-($-$$) db 0