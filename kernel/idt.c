#include <stdint.h>

struct idt_entry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idt_descriptor;

extern void isr0(void);
extern void isr6(void);
extern void isr_keyboard(void);
extern void isr_timer(void);
extern void idt_load(struct idt_ptr* idt_ptr);

static void idt_set_gate(int vector, uint64_t handler)
{
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = 0x8E;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void idt_init(void)
{
    for (int i = 0; i < 256; i++)
    {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type_attr = 0;
        idt[i].offset_mid = 0;
        idt[i].offset_high = 0;
        idt[i].zero = 0;
    }

    idt_set_gate(0, (uint64_t)isr0);
idt_set_gate(6, (uint64_t)isr6);
idt_set_gate(0x20, (uint64_t)isr_timer);
idt_set_gate(0x21, (uint64_t)isr_keyboard);

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (uint64_t)&idt;

    idt_load(&idt_descriptor);
}
