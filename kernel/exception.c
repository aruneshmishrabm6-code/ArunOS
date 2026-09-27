#include <stdint.h>

static void print_hex64(volatile uint16_t* video, int position, uint64_t value)
{
    const char* hex = "0123456789ABCDEF";

    video[position++] = 'R' | 0x4F00;
    video[position++] = 'I' | 0x4F00;
    video[position++] = 'P' | 0x4F00;
    video[position++] = ':' | 0x4F00;
    video[position++] = ' ' | 0x4F00;
    video[position++] = '0' | 0x4F00;
    video[position++] = 'x' | 0x4F00;

    for (int i = 15; i >= 0; i--)
    {
        uint8_t digit = (value >> (i * 4)) & 0xF;
        video[position++] =
            (uint16_t)hex[digit] | 0x4F00;
    }
}

void exception_handler(uint64_t exception_number, uint64_t rip)
{
    volatile uint16_t* video =
        (volatile uint16_t*)0xB8000;

    const char* message = "EXCEPTION: ";

    for (int i = 0; message[i] != '\0'; i++)
    {
        video[i] =
            (uint16_t)message[i] | 0x4F00;
    }

    if (exception_number == 0)
    {
        const char* name = "Divide by Zero";

        for (int i = 0; name[i] != '\0'; i++)
        {
            video[11 + i] =
                (uint16_t)name[i] | 0x4F00;
        }
    }
    else if (exception_number == 6)
    {
        const char* name = "Invalid Opcode";

        for (int i = 0; name[i] != '\0'; i++)
        {
            video[11 + i] =
                (uint16_t)name[i] | 0x4F00;
        }
    }

    print_hex64(video, 80, rip);

    while (1)
    {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}
