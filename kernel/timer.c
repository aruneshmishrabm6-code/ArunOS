#include <stdint.h>

volatile uint64_t timer_ticks = 0;

void timer_handler(void)
{
    timer_ticks++;
}

uint64_t timer_get_ticks(void)
{
    return timer_ticks;
}

uint64_t timer_get_seconds(void)
{
    return timer_ticks / 100;
}
