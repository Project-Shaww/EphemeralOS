; EphemeralOS Bootloader
; Stage 1 - Boot sector

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov si, boot_msg
    call print_string

    call load_stage2

    jmp 0x0000:0x7E00

load_stage2:
    mov ah, 0x02
    mov al, 15
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov bx, 0x7E00
    int 0x13
    jc disk_error
    ret

disk_error:
    mov si, disk_error_msg
    call print_string
    hlt

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string
.done:
    ret

boot_msg db 'EphemeralOS Booting...', 13, 10, 0
disk_error_msg db 'Disk Error!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
