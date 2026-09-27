#include <stdint.h>

extern void idt_init(void);
extern void pic_remap(void);
extern void pit_init(uint32_t frequency);

void kernel_main(void)
{
    volatile uint16_t* video =
        (volatile uint16_t*)0xB8000ULL;

    const char* message = "MyOS 64-bit Kernel Started!";

    for (unsigned long i = 0; message[i] != '\0'; i++)
    {
        video[i] = (uint16_t)message[i] | 0x0700;
    }

    idt_init();

    pic_remap();
pit_init(100);
__asm__ volatile ("sti");


    while (1)
    {
        __asm__ volatile ("hlt");
    }
}
