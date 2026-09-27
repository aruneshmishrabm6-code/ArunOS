#include <stdint.h>

extern void idt_init(void);
extern void pic_remap(void);
extern void pit_init(uint32_t frequency);

extern uint64_t timer_get_seconds(void);
extern void memory_init(uint64_t multiboot_info);
extern uint64_t memory_get_usable(void);

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

static void print_number(uint64_t value)
{
    int position = 168;

    /* Clear old number */
    for (int i = 0; i < 10; i++)
    {
        video[position + i] = ' ' | 0x0700;
    }

    if (value == 0)
    {
        video[position] = '0' | 0x0700;
        return;
    }

    char digits[20];
    int count = 0;

    while (value > 0)
    {
        digits[count++] = '0' + (value % 10);
        value /= 10;
    }

    for (int i = 0; i < count; i++)
    {
        video[position + i] =
            digits[count - 1 - i] | 0x0700;
    }
}

static void display_uptime(uint64_t seconds)
{
    const char* label = "Uptime: ";

    for (int i = 0; label[i] != '\0'; i++)
    {
        video[160 + i] =
            label[i] | 0x0700;
    }

    print_number(seconds);

    video[178] = 's' | 0x0700;
}

static void display_memory(uint64_t bytes)
{
    const char* label = "RAM: ";

    for (int i = 0; label[i] != '\0'; i++)
    {
        video[240 + i] = label[i] | 0x0700;
    }

    uint64_t mb = bytes / (1024 * 1024);

    char digits[20];
    int count = 0;

    if (mb == 0)
    {
        digits[count++] = '0';
    }
    else
    {
        while (mb > 0)
        {
            digits[count++] = '0' + (mb % 10);
            mb /= 10;
        }
    }

    for (int i = 0; i < count; i++)
    {
        video[245 + i] =
            digits[count - 1 - i] | 0x0700;
    }

    video[245 + count] = 'M' | 0x0700;
    video[246 + count] = 'B' | 0x0700;
}

void kernel_main(uint64_t multiboot_info)
{
    memory_init(multiboot_info);
    display_memory(memory_get_usable());
    const char* message =
        "ArunOS 64-bit Kernel Started!";

    for (unsigned long i = 0;
         message[i] != '\0';
         i++)
    {
        video[i] =
            (uint16_t)message[i] | 0x0700;
    }

    idt_init();

    pic_remap();

    pit_init(100);

    __asm__ volatile ("sti");

    uint64_t last_seconds = 0;

    while (1)
    {
        uint64_t seconds = timer_get_seconds();

        if (seconds != last_seconds)
        {
            last_seconds = seconds;
            display_uptime(seconds);
        }





        __asm__ volatile ("hlt");
    }
}
