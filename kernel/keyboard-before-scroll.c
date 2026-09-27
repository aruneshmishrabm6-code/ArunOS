#include <stdint.h>

/* ============================================================
   VGA
   ============================================================ */

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT)


/* ============================================================
   Cursor
   ============================================================ */

static int cursor = 0;


/* ============================================================
   Input buffer
   ============================================================ */

static char input_buffer[128];

static int input_length = 0;

volatile int command_ready = 0;


/* ============================================================
   Keyboard map
   ============================================================ */

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


/* ============================================================
   Cursor functions
   ============================================================ */

void keyboard_set_cursor(int position)
{
    if (position < 0)
        position = 0;

    if (position >= SCREEN_SIZE)
        position = SCREEN_SIZE - 1;

    cursor = position;
}


int keyboard_get_cursor(void)
{
    return cursor;
}


/* ============================================================
   Command functions
   ============================================================ */

const char* keyboard_get_command(void)
{
    return input_buffer;
}


void keyboard_clear_command(void)
{
    input_length = 0;

    input_buffer[0] = '\0';

    command_ready = 0;
}


/* ============================================================
   Keyboard handler
   ============================================================ */

void keyboard_handler(uint64_t scancode)
{
    /* Ignore key release */
    if (scancode & 0x80)
        return;

    if (scancode >= 128)
        return;

    char c = keyboard_map[scancode];

    if (c == 0)
        return;


    /* Do not type while command is being processed */
    if (command_ready)
        return;


    /* --------------------------------------------------------
       Backspace
       -------------------------------------------------------- */

    if (c == '\b')
    {
        if (input_length > 0)
        {
            input_length--;

            input_buffer[input_length] = '\0';

            if (cursor > 0)
            {
                cursor--;

                video[cursor] =
                    ' ' | 0x0700;
            }
        }

        return;
    }


    /* --------------------------------------------------------
       Enter
       -------------------------------------------------------- */

    if (c == '\n')
    {
        input_buffer[input_length] = '\0';

        command_ready = 1;

        cursor =
            ((cursor / SCREEN_WIDTH) + 1)
            * SCREEN_WIDTH;

        if (cursor >= SCREEN_SIZE)
        {
            cursor = SCREEN_SIZE - SCREEN_WIDTH;
        }

        return;
    }


    /* --------------------------------------------------------
       Buffer limit
       -------------------------------------------------------- */

    if (input_length >= 127)
        return;


    /* --------------------------------------------------------
       Print character
       -------------------------------------------------------- */

    if (cursor < SCREEN_SIZE)
    {
        video[cursor] =
            (uint16_t)c | 0x0700;

        cursor++;

        input_buffer[input_length] = c;

        input_length++;

        input_buffer[input_length] = '\0';
    }
}
