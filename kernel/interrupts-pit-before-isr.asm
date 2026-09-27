bits 64

section .text

global isr0
global isr6
global isr_keyboard
global idt_load

extern exception_handler
extern keyboard_handler


; ============================================================
; Exception 0 — Divide by Zero
; ============================================================

isr0:
    cli

    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    mov edi, 0
    xor esi, esi
    call exception_handler

.hang0:
    hlt
    jmp .hang0


; ============================================================
; Exception 6 — Invalid Opcode
; ============================================================

isr6:
    cli

    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    mov edi, 6

    ; Original CPU-pushed RIP:
    ; 9 registers were pushed = 72 bytes
    mov rsi, [rsp + 72]

    call exception_handler

.hang6:
    hlt
    jmp .hang6


; ============================================================
; Keyboard IRQ1
; PIC remapped IRQ1 -> interrupt vector 0x21
; ============================================================

isr_keyboard:
    cli

    ; Save registers that the C handler may use
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    ; Read keyboard scancode
    in al, 0x60

    ; Pass scancode to C handler
    movzx edi, al
    call keyboard_handler

    ; Send EOI to Master PIC
    mov al, 0x20
    out 0x20, al

    ; Restore registers
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rax

    sti
    iretq


; ============================================================
; Load IDT
; RDI = address of IDT descriptor
; ============================================================

idt_load:
    lidt [rdi]
    ret
