#include <stdint.h>

volatile uint64_t timer_ticks = 0;

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

static void print_number(uint64_t value)
{
    int position = 176;

    /* Clear previous number area */
    for (int i = 0; i < 20; i++)
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

void timer_handler(void)
{
    timer_ticks++;

    /* 100 PIT ticks = approximately 1 second */
    if ((timer_ticks % 100) == 0)
    {
        uint64_t seconds = timer_ticks / 100;

        const char* label = "Uptime: ";

        for (int i = 0; label[i] != '\0'; i++)
        {
            video[160 + i] =
                label[i] | 0x0700;
        }

        print_number(seconds);

        video[184] = 's' | 0x0700;
    }
}
