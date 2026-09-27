#include <stdint.h>

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

static int cursor = 29;

static const char keyboard_map[128] =
{
    0,
    0,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',
    ',','.','/',
    0,
    '*',
    0,
    ' '
};

void keyboard_handler(uint64_t scancode)
{
    /* Ignore key-release scancodes */
    if (scancode & 0x80)
        return;

    if (scancode >= 128)
        return;

    char c = keyboard_map[scancode];

    if (c == 0)
        return;

    if (c == '\b')
    {
        if (cursor > 0)
        {
            cursor--;
            video[cursor] = ' ' | 0x0700;
        }

        return;
    }

    if (c == '\n')
    {
        cursor = ((cursor / 80) + 1) * 80;
        return;
    }

    if (cursor < 80 * 25)
    {
        video[cursor] =
            (uint16_t)c | 0x0700;

        cursor++;
    }
}
