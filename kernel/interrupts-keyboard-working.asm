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

    ; CPU exception frame:
    ; [rsp]     = RIP
    ; [rsp+8]   = CS
    ; [rsp+16]  = RFLAGS

    mov edi, 6
    mov rsi, [rsp]
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

    ; Read keyboard scancode from port 0x60
    in al, 0x60

    ; Pass scancode to C handler
    movzx edi, al
    call keyboard_handler

    ; Send End Of Interrupt (EOI) to master PIC
    mov al, 0x20
    out 0x20, al

    sti
    iretq


; ============================================================
; Load IDT
; RDI = address of IDT descriptor
; ============================================================

idt_load:
    lidt [rdi]
    ret
