; ============================================================
; ArunOS v0.4 - Kernel Entry
; Multiboot2 + 32-bit Protected Mode -> 64-bit Long Mode
; ============================================================


; ============================================================
; Multiboot2 Header
; ============================================================

section .multiboot
align 8

multiboot_header:
    dd 0xE85250D6
    dd 0
    dd multiboot_header_end - multiboot_header
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    ; Multiboot2 End Tag
    dw 0
    dw 0
    dd 8

multiboot_header_end:


; ============================================================
; Kernel Entry - 32-bit
; ============================================================

section .text
bits 32

global _start
extern kernel_main

_start:
    cli
    cld

    ; --------------------------------------------------------
    ; Create our 32-bit stack
    ; --------------------------------------------------------

    mov esp, stack_top

    ; --------------------------------------------------------
    ; Verify Multiboot2 magic
    ; EAX = 0x36D76289
    ; EBX = Multiboot information structure
    ; --------------------------------------------------------

    cmp eax, 0x36D76289
    jne .hang

    ; --------------------------------------------------------
    ; Preserve Multiboot information pointer
    ; --------------------------------------------------------

    mov ebp, ebx
    mov edi, ebx

    ; --------------------------------------------------------
    ; Initialize identity-mapped page tables
    ; Map first 2 MiB using one 2 MiB page
    ; --------------------------------------------------------

    mov eax, pdpt_table
    or eax, 0x03
    mov [pml4_table], eax

    mov eax, pd_table
    or eax, 0x03
    mov [pdpt_table], eax

    ; Present + Writable + Page Size
    mov eax, 0x83
    mov [pd_table], eax

    ; --------------------------------------------------------
    ; Check whether CPU supports Long Mode
    ; --------------------------------------------------------

    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid

    test edx, (1 << 29)
    jz .no_long_mode

    ; --------------------------------------------------------
    ; Enable PAE
    ; --------------------------------------------------------

    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; --------------------------------------------------------
    ; Enable Long Mode in EFER
    ; --------------------------------------------------------

    mov ecx, 0xC0000080
    rdmsr

    or eax, (1 << 8)

    wrmsr

    ; --------------------------------------------------------
    ; Load PML4 and enable paging
    ; --------------------------------------------------------

    mov eax, pml4_table
    mov cr3, eax

    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; --------------------------------------------------------
    ; Load 64-bit GDT
    ; --------------------------------------------------------

    lgdt [gdt64_pointer]

    ; --------------------------------------------------------
    ; Far jump into 64-bit code
    ; --------------------------------------------------------

    jmp 0x08:long_mode_start


; ============================================================
; Long Mode Not Supported
; ============================================================

.no_long_mode:
    mov esi, no_long_mode_message
    call print_string


; ============================================================
; 32-bit Hang
; ============================================================

.hang:
    cli
    hlt
    jmp .hang


; ============================================================
; 64-bit Long Mode
; ============================================================

bits 64

long_mode_start:

    ; --------------------------------------------------------
    ; Reload data segments
    ; --------------------------------------------------------

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; --------------------------------------------------------
    ; Set up 64-bit stack
    ; --------------------------------------------------------

    mov rsp, stack_top

    ; --------------------------------------------------------
    ; RDI already contains Multiboot information pointer
    ; because it was copied from EBX before entering long mode
    ; --------------------------------------------------------

    ; --------------------------------------------------------
    ; Call C kernel
    ; --------------------------------------------------------

    call kernel_main


; ============================================================
; 64-bit Hang
; ============================================================

.hang64:
    cli
    hlt
    jmp .hang64


; ============================================================
; 32-bit VGA Error Message
; ============================================================

bits 32

print_string:
    mov edi, 0xB8000
    mov ah, 0x07

.next:
    lodsb

    test al, al
    jz .done

    mov [edi], al
    mov [edi + 1], ah

    add edi, 2

    jmp .next

.done:
    ret


; ============================================================
; Read-Only Data
; ============================================================

section .rodata

no_long_mode_message:
    db "CPU does not support x86-64 Long Mode", 0


; ============================================================
; Page Tables
; ============================================================

section .bss

align 4096

pml4_table:
    resq 512

pdpt_table:
    resq 512

pd_table:
    resq 512


; ============================================================
; Kernel Stack
; ============================================================

align 16

stack_bottom:
    resb 16384

stack_top:


; ============================================================
; 64-bit GDT
; ============================================================

section .data

align 8

gdt64:
    ; Null descriptor
    dq 0x0000000000000000

    ; 64-bit Code Segment
    dq 0x00AF9A000000FFFF

    ; Data Segment
    dq 0x00CF92000000FFFF

gdt64_pointer:
    dw gdt64_end - gdt64 - 1
    dd gdt64

gdt64_end:
