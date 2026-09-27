#include <stdint.h>

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

void pit_init(uint32_t frequency)
{
    uint32_t divisor = 1193182 / frequency;

    /* Channel 0, lobyte/hibyte, mode 3 */
    outb(PIT_COMMAND, 0x36);

    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}
