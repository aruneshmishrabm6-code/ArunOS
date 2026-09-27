#include <stdint.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT     0x10
#define ICW1_ICW4     0x01
#define ICW4_8086     0x01

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void io_wait(void)
{
    outb(0x80, 0);
}

void pic_remap(void)
{
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* Master PIC: IRQs start at interrupt 0x20 */
    outb(PIC1_DATA, 0x20);
    io_wait();

    /* Slave PIC: IRQs start at interrupt 0x28 */
    outb(PIC2_DATA, 0x28);
    io_wait();

    /* Tell Master PIC that Slave is connected to IRQ2 */
    outb(PIC1_DATA, 4);
    io_wait();

    /* Tell Slave PIC its cascade identity */
    outb(PIC2_DATA, 2);
    io_wait();

    /* 8086/88 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();

    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /*
     * Mask everything except keyboard IRQ1.
     *
     * 0xFD = 11111101
     * IRQ1 = 0, so keyboard is enabled.
     * IRQ0 = 1, so timer is disabled.
     */
    outb(PIC1_DATA, 0xFC);

    /* Disable all slave PIC IRQs for now */
    outb(PIC2_DATA, 0xFF);
}
