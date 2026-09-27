#include <stdint.h>

/* ============================================================
   External kernel functions
   ============================================================ */

extern void idt_init(void);
extern void pic_remap(void);
extern void pit_init(uint32_t frequency);

extern uint64_t timer_get_seconds(void);

extern void memory_init(uint64_t multiboot_info);
extern uint64_t memory_get_usable(void);


/* ============================================================
   Keyboard interface
   ============================================================ */

extern volatile int command_ready;

extern const char* keyboard_get_command(void);

extern void keyboard_clear_command(void);

extern void keyboard_set_cursor(int position);

extern int keyboard_get_cursor(void);

/* ============================================================
   ATA disk interface
   ============================================================ */

extern int ata_read_sector(
    uint32_t lba,
    uint8_t* buffer
);

extern int ata_write_sector(
    uint32_t lba,
    const uint8_t* buffer
);

/* ============================================================
   Disk and Persistent Filesystem Interface
   ============================================================ */

extern int disk_read(
    uint32_t lba,
    uint8_t* buffer
);

extern int disk_write(
    uint32_t lba,
    const uint8_t* buffer
);

extern int fs_disk_format(void);
extern int fs_disk_check(void);
extern int fs_disk_test_entry(void);
extern int fs_disk_test_entry_read(void);
/* ============================================================
   Disk storage interface
   ============================================================ */

extern int disk_read(
    uint32_t lba,
    uint8_t* buffer
);

extern int disk_write(
    uint32_t lba,
    const uint8_t* buffer
);

/* ============================================================
   Virtual filesystem interface
   ============================================================ */

extern void fs_init(void);

extern void fs_list(void);

extern int fs_mkdir(const char* name);

extern int fs_touch(const char* name);

extern int fs_cat(
    const char* name,
    char* output,
    int output_size
);
extern int fs_write(const char* name, const char* data);
extern int fs_rm(const char* name);
extern int fs_rmdir(const char* name);
extern int fs_is_directory(const char* name);
extern int fs_cd(const char* name);
extern int fs_get_current_path(char* output, int output_size);
/* ============================================================
   VGA
   ============================================================ */

static volatile uint16_t* video =
    (volatile uint16_t*)0xB8000;

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25
#define SCREEN_SIZE   (SCREEN_WIDTH * SCREEN_HEIGHT)
#define FS_DATA_LENGTH 128

static int cursor_position = 0;
static char current_directory[64] = "/";

/* ============================================================
   Command history
   ============================================================ */

#define HISTORY_SIZE   10
#define HISTORY_LENGTH 128

static char history[
    HISTORY_SIZE
][
    HISTORY_LENGTH
];

static int history_count = 0;


/* ============================================================
   String functions
   ============================================================ */

static int string_length(const char* text)
{
    int length = 0;

    while (text[length] != '\0')
        length++;

    return length;
}


static void string_copy(
    char* destination,
    const char* source
)
{
    int i = 0;

    while (source[i] != '\0' &&
           i < HISTORY_LENGTH - 1)
    {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}


static int string_equal(
    const char* a,
    const char* b
)
{
    int i = 0;

    while (a[i] != '\0' &&
           b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == '\0' &&
           b[i] == '\0';
}


static int starts_with(
    const char* text,
    const char* prefix
)
{
    int i = 0;

    while (prefix[i] != '\0')
    {
        if (text[i] != prefix[i])
            return 0;

        i++;
    }

    return 1;
}


/* ============================================================
   Get argument after command
   Example:
   "mkdir projects" -> "projects"
   ============================================================ */

static const char* get_argument(
    const char* command
)
{
    int i = 0;

    while (command[i] != ' ' &&
           command[i] != '\0')
    {
        i++;
    }

    if (command[i] == '\0')
        return command + i;

    return command + i + 1;
}


/* ============================================================
   Screen scrolling
   ============================================================ */

static void scroll_screen(void)
{
    int row;
    int column;

    /* Move every row one line upward */
    for (row = 1; row < SCREEN_HEIGHT; row++)
    {
        for (column = 0; column < SCREEN_WIDTH; column++)
        {
            video[
                (row - 1) * SCREEN_WIDTH + column
            ] =
                video[
                    row * SCREEN_WIDTH + column
                ];
        }
    }

    /* Clear the new bottom line */
    for (column = 0; column < SCREEN_WIDTH; column++)
    {
        video[
            (SCREEN_HEIGHT - 1) * SCREEN_WIDTH + column
        ] = (uint16_t)' ' | 0x0700;
    }

    /* Cursor starts at bottom line */
    cursor_position =
        (SCREEN_HEIGHT - 1) * SCREEN_WIDTH;
}


/* ============================================================
   Cursor safety
   ============================================================ */

static void ensure_cursor(void)
{
    while (cursor_position >= SCREEN_SIZE)
    {
        scroll_screen();
    }

    if (cursor_position < 0)
    {
        cursor_position = 0;
    }
}


/* ============================================================
   Shell output
   IMPORTANT:
   fs.c uses this function.
   ============================================================ */

void shell_print(const char* text)
{
    int i;

    for (i = 0; text[i] != '\0'; i++)
    {
        /* ----------------------------------------------------
           Newline
           ---------------------------------------------------- */

        if (text[i] == '\n')
        {
            cursor_position =
                ((cursor_position / SCREEN_WIDTH) + 1)
                * SCREEN_WIDTH;

            ensure_cursor();

            continue;
        }

        /* ----------------------------------------------------
           Normal character
           ---------------------------------------------------- */

        ensure_cursor();

        video[cursor_position] =
            (uint16_t)text[i] | 0x0700;

        cursor_position++;

        /* ----------------------------------------------------
           If character filled the screen, scroll immediately
           ---------------------------------------------------- */

        if (cursor_position >= SCREEN_SIZE)
        {
            scroll_screen();
        }
    }
}






/* ============================================================
   Print number
   ============================================================ */

static void print_number(uint64_t value)
{
    char digits[20];

    int count = 0;


    if (value == 0)
    {
        ensure_cursor();

        video[cursor_position++] =
            '0' | 0x0700;

        return;
    }


    while (value > 0)
    {
        digits[count++] =
            '0' + (value % 10);

        value /= 10;
    }


    for (int i = count - 1;
         i >= 0;
         i--)
    {
        ensure_cursor();

        video[cursor_position++] =
            digits[i] | 0x0700;
    }
}

/* ============================================================
   Print 8-bit hexadecimal value
   ============================================================ */

static void print_hex8(uint8_t value)
{
    const char* hex = "0123456789ABCDEF";

    ensure_cursor();
    video[cursor_position++] =
        hex[(value >> 4) & 0x0F] | 0x0700;

    ensure_cursor();
    video[cursor_position++] =
        hex[value & 0x0F] | 0x0700;
}

/* ============================================================
   Clear screen
   ============================================================ */

static void clear_screen(void)
{
    for (int i = 0;
         i < SCREEN_SIZE;
         i++)
    {
        video[i] = ' ' | 0x0700;
    }

    cursor_position = 0;
}


/* ============================================================
   History
   ============================================================ */

static void add_history(
    const char* command
)
{
    if (command[0] == '\0')
        return;


    if (history_count >= HISTORY_SIZE)
    {
        for (int i = 1;
             i < HISTORY_SIZE;
             i++)
        {
            string_copy(
                history[i - 1],
                history[i]
            );
        }

        history_count =
            HISTORY_SIZE - 1;
    }


    string_copy(
        history[history_count],
        command
    );

    history_count++;
}


static void show_history(void)
{
    if (history_count == 0)
    {
        shell_print(
            "No commands in history.\n"
        );

        return;
    }


    for (int i = 0;
         i < history_count;
         i++)
    {
        print_number(i + 1);

        shell_print("  ");

        shell_print(history[i]);

        shell_print("\n");
    }
}


/* ============================================================
   Shell prompt
   ============================================================ */

static void print_prompt(void)
{
    char path[64];

    shell_print("arunos:");

    if (fs_get_current_path(path, sizeof(path)) == 0)
    {
        shell_print(path);
    }
    else
    {
        shell_print("/");
    }

    shell_print("> ");

    keyboard_set_cursor(cursor_position);
}


/* ============================================================
   Execute command
   ============================================================ */

static void execute_command(
    const char* command
)
{
    /* --------------------------------------------------------
       HELP
       -------------------------------------------------------- */

    if (string_equal(command, "help"))
    {
        shell_print(
            "Available commands:\n"
        );

        shell_print(
            "help     - Show available commands\n"
        );

        shell_print(
            "about    - About ArunOS\n"
        );

        shell_print(
            "version  - Show ArunOS version\n"
        );

        shell_print(
            "echo     - Print text\n"
        );

        shell_print(
            "history  - Show command history\n"
        );

        shell_print(
            "clear    - Clear the screen\n"
        );

        shell_print(
            "mem      - Show usable RAM\n"
        );

        shell_print(
            "uptime   - Show system uptime\n"
        );
           
         shell_print(
             "fstestwrite - Write filesystem test entry\n"
         "fstestread  - Read filesystem test entry\n"
             "diskread - Read disk sector <LBA>\n"
        );
         shell_print(
              "diskwrite - Write test data to disk sector <LBA>\n"
        );
         shell_print(
              "disktest - Test disk storage layer\n"
        );

        shell_print(
            "pwd      - Show current directory\n"
        );

        shell_print(
            "cd       - Change current directory\n"
        );

        shell_print(
            "ls       - List files and directories\n"
        );

        shell_print(
            "mkdir    - Create directory\n"
        );

        shell_print(
            "touch    - Create file\n"
        );

        shell_print(
            "cat      - Read file\n"
        );
        shell_print(
            "write    - Write text to file\n"
        );

        shell_print(
            "rm       - Remove file\n"
        );

        shell_print(
            "rmdir    - Remove directory\n"
        );
    }


    /* --------------------------------------------------------
       ABOUT
       -------------------------------------------------------- */

    else if (string_equal(command, "about"))
    {
        shell_print(
            "ArunOS 64-bit Hobby Operating System\n"
        );

        shell_print(
            "Custom x86_64 kernel project.\n"
        );
    }


    /* --------------------------------------------------------
       VERSION
       -------------------------------------------------------- */

    else if (string_equal(command, "version"))
    {
        shell_print(
            "ArunOS v0.6\n"
        );

        shell_print(
            "Architecture: x86_64\n"
        );
    }


    /* --------------------------------------------------------
       ECHO
       -------------------------------------------------------- */

    else if (starts_with(command, "echo "))
    {
        shell_print(command + 5);

        shell_print("\n");
    }


    /* ---
-----------------------------------------------------
       HISTORY
       -------------------------------------------------------- */

    else if (string_equal(command, "history"))
    {
        show_history();
    }


    /* --------------------------------------------------------
       CLEAR
       -------------------------------------------------------- */

    else if (string_equal(command, "clear"))
    {
        clear_screen();
    }


    /* --------------------------------------------------------
       MEMORY
       -------------------------------------------------------- */

    else if (string_equal(command, "mem"))
    {
        uint64_t memory =
            memory_get_usable();

        uint64_t mb =
            memory / (1024 * 1024);

        shell_print("Usable RAM: ");

        print_number(mb);

        shell_print(" MB\n");
    }


    /* --------------------------------------------------------
       UPTIME
       -------------------------------------------------------- */

    else if (string_equal(command, "uptime"))
    {
        uint64_t seconds =
            timer_get_seconds();

        shell_print("Uptime: ");

        print_number(seconds);

        shell_print(" seconds\n");
    }

/* --------------------------------------------------------
   DISK READ TEST
   -------------------------------------------------------- */

else if (starts_with(command, "diskread "))
{
    const char* argument = get_argument(command);

    uint32_t lba = 0;

    if (argument[0] == '\0')
    {
        shell_print("Usage: diskread <LBA>\n");
    }
    else
    {
        int valid = 1;

        for (int i = 0; argument[i] != '\0'; i++)
        {
            if (argument[i] < '0' ||
                argument[i] > '9')
            {
                valid = 0;
                break;
            }

            lba =
                (lba * 10) +
                (argument[i] - '0');
        }

        if (!valid)
        {
            shell_print("Error: LBA must be a number.\n");
        }
        else
        {
            uint8_t buffer[512];

            shell_print("Reading disk sector ");

            print_number(lba);

            shell_print("...\n");

            int result =
                ata_read_sector(lba, buffer);

            if (result != 0)
            {
                shell_print("Disk read failed.\n");
            }
            else
            {
                shell_print(
                    "Sector read successful.\n"
                );

                shell_print(
                    "First 16 bytes: "
                );

                for (int i = 0; i < 16; i++)
                {
                    print_hex8(buffer[i]);
                    shell_print(" ");
                }

                shell_print("\n");
            }
        }
    }
}


/* --------------------------------------------------------
   DISK WRITE TEST
   -------------------------------------------------------- */

else if (starts_with(command, "diskwrite "))
{
    const char* argument = get_argument(command);

    uint32_t lba = 0;

    if (argument[0] == '\0')
    {
        shell_print("Usage: diskwrite <LBA>\n");
    }
    else
    {
        int valid = 1;

        for (int i = 0; argument[i] != '\0'; i++)
        {
            if (argument[i] < '0' ||
                argument[i] > '9')
            {
                valid = 0;
                break;
            }

            lba =
                (lba * 10) +
                (argument[i] - '0');
        }

        if (!valid)
        {
            shell_print("Error: LBA must be a number.\n");
        }
        else
        {
            uint8_t buffer[512];

            for (int i = 0; i < 512; i++)
            {
                buffer[i] = 0;
            }

            const char* message =
                "ArunOS ATA WRITE TEST";

            for (int i = 0;
                 message[i] != '\0' && i < 511;
                 i++)
            {
                buffer[i] = message[i];
            }

            shell_print("Writing disk sector ");

            print_number(lba);

            shell_print("...\n");

            int result =
                ata_write_sector(lba, buffer);

            if (result != 0)
            {
                shell_print("Disk write failed.\n");
            }
            else
            {
                shell_print(
                    "Sector write successful.\n"
                );

                shell_print(
                    "Written data: "
                );

                for (int i = 0; i < 21; i++)
                {
                    if (buffer[i] == 0)
                        break;

                    ensure_cursor();

                    video[cursor_position++] =
                        buffer[i] | 0x0700;
                }

                shell_print("\n");
            }
        }
    }
}
 
else if (starts_with(command, "disktest "))
{
    const char* argument = get_argument(command);

    uint32_t lba = 0;
    int valid = 1;

    if (argument[0] == '\0')
    {
        shell_print("Usage: disktest <LBA>\n");
    }
    else
    {
        for (int i = 0; argument[i] != '\0'; i++)
        {
            if (argument[i] < '0' ||
                argument[i] > '9')
            {
                valid = 0;
                break;
            }

            lba =
                (lba * 10) +
                (argument[i] - '0');
        }

        if (!valid)
        {
            shell_print("Error: LBA must be a number.\n");
        }
        else
        {
            uint8_t buffer[512];

            shell_print("Disk layer reading LBA ");

            print_number(lba);

            shell_print("...\n");

            int result =
                disk_read(lba, buffer);

            if (result != 0)
            {
                shell_print("Disk layer read failed.\n");
            }
            else
            {
                shell_print(
                    "Disk layer read successful.\n"
                );

                shell_print("First 16 bytes: ");

                for (int i = 0; i < 16; i++)
                {
                    print_hex8(buffer[i]);
                    shell_print(" ");
                }

                shell_print("\n");
            }
        }
    }
}

/* --------------------------------------------------------
   DISK LAYER TEST
   -------------------------------------------------------- */

else if (starts_with(command, "disktest "))
{
    const char* argument = get_argument(command);

    uint32_t lba = 0;
    int valid = 1;

    if (argument[0] == '\0')
    {
        shell_print("Usage: disktest <LBA>\n");
    }
    else
    {
        for (int i = 0; argument[i] != '\0'; i++)
        {
            if (argument[i] < '0' ||
                argument[i] > '9')
            {
                valid = 0;
                break;
            }

            lba =
                (lba * 10) +
                (argument[i] - '0');
        }

        if (!valid)
        {
            shell_print("Error: LBA must be a number.\n");
        }
        else
        {
            uint8_t buffer[512];

            shell_print("Disk layer reading LBA ");

            print_number(lba);

            shell_print("...\n");

            int result =
                disk_read(lba, buffer);

            if (result != 0)
            {
                shell_print(
                    "Disk layer read failed.\n"
                );
            }
            else
            {
                shell_print(
                    "Disk layer read successful.\n"
                );

                shell_print(
                    "First 16 bytes: "
                );

                for (int i = 0; i < 16; i++)
                {
                    print_hex8(buffer[i]);
                    shell_print(" ");
                }

                shell_print("\n");
            }
        }
    }
}

/* --------------------------------------------------------
   PERSISTENT FILESYSTEM TEST
   -------------------------------------------------------- */

else if (string_equal(command, "diskformat"))
{
    shell_print("Formatting ArunOS filesystem metadata...\n");

    int result = fs_disk_format();

    if (result == 0)
    {
        shell_print(
            "Filesystem metadata written successfully.\n"
        );
    }
    else
    {
        shell_print(
            "Filesystem metadata write failed.\n"
        );
    }
}

else if (string_equal(command, "diskcheck"))
{
    shell_print("Checking ArunOS filesystem metadata...\n");

    int result = fs_disk_check();

    if (result == 0)
    {
        shell_print(
            "Filesystem metadata is valid.\n"
        );
    }
    else if (result == -2)
    {
        shell_print(
            "Filesystem metadata not found.\n"
        );
    }
    else if (result == -3)
    {
        shell_print(
            "Filesystem metadata version mismatch.\n"
        );
    }
    else
    {
        shell_print(
            "Filesystem metadata read failed.\n"
        );
    }
}

/* --------------------------------------------------------
   FILESYSTEM ENTRY DISK TEST
   -------------------------------------------------------- */

else if (string_equal(command, "fstestwrite"))
{
    shell_print("Writing filesystem test entry...\n");

    int result =
        fs_disk_test_entry();

    if (result == 0)
    {
        shell_print(
            "Filesystem test entry written successfully.\n"
        );
    }
    else
    {
        shell_print(
            "Filesystem test entry write failed.\n"
        );
    }
}

else if (string_equal(command, "fstestread"))
{
    shell_print("Reading filesystem test entry...\n");

    int result =
        fs_disk_test_entry_read();

    if (result == 0)
    {
        shell_print(
            "Filesystem test entry is valid.\n"
        );
    }
    else if (result == -2)
    {
        shell_print(
            "Filesystem test entry is missing.\n"
        );
    }
    else if (result == -3)
    {
        shell_print(
            "Filesystem test entry type is invalid.\n"
        );
    }
    else if (result == -4)
    {
        shell_print(
            "Filesystem test entry name is invalid.\n"
        );
    }
    else
    {
        shell_print(
            "Filesystem test entry read failed.\n"
        );
    }
}

    /* --------------------------------------------------------
   PWD
   -------------------------------------------------------- */

else if (string_equal(command, "pwd"))
{
    if (fs_get_current_path(
            current_directory,
            sizeof(current_directory)) == 0)
    {
        shell_print(current_directory);
        shell_print("\n");
    }
    else
    {
        shell_print("Error: unable to get current directory.\n");
    }
}


/* --------------------------------------------------------
   CD
   -------------------------------------------------------- */

else if (string_equal(command, "cd"))
{
    int result = fs_cd("/");

    if (result != 0)
    {
        shell_print("Error: cannot change to root directory.\n");
    }
}

else if (starts_with(command, "cd "))
{
    const char* name =
        get_argument(command);

    if (name[0] == '\0')
    {
        shell_print("Usage: cd <directory>\n");
    }
    else
    {
        int result = fs_cd(name);

        if (result == 0)
        {
            /* Directory changed successfully */
        }
        else if (result == -2)
        {
            shell_print("Error: directory not found.\n");
        }
        else if (result == -3)
        {
            shell_print("Error: not a directory.\n");
        }
        else
        {
            shell_print("Error: cannot change directory.\n");
        }
    }
}



    /* --------------------------------------------------------
       LS
       -------------------------------------------------------- */

    else if (string_equal(command, "ls"))
    {
        fs_list();
    }


    /* --------------------------------------------------------
       MKDIR
       -------------------------------------------------------- */

    else if (starts_with(command, "mkdir "))
    {
        const char* name =
            get_argument(command);

        if (name[0] == '\0')
        {
            shell_print(
                "Usage: mkdir <name>\n"
            );
        }
        else
        {
            int result =
                fs_mkdir(name);

            if (result == 0)
            {
                shell_print(
                    "Directory created: "
                );

                shell_print(name);

                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print(
                    "Error: entry already exists.\n"
                );
            }
            else if (result == -3)
            {
                shell_print(
                    "Error: filesystem is full.\n"
                );
            }
            else
            {
                shell_print(
                    "Error: invalid directory name.\n"
                );
            }
        }
    }


    /* --------------------------------------------------------
       TOUCH
       -------------------------------------------------------- */

    else if (starts_with(command, "touch "))
    {
        const char* name =
            get_argument(command);

        if (name[0] == '\0')
        {
            shell_print(
                "Usage: touch <name>\n"
            );
        }
        else
        {
            int result =
                fs_touch(name);

            if (result == 0)
            {
                shell_print(
                    "File created: "
                );

                shell_print(name);

                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print(
                    "Error: entry already exists.\n"
                );
            }
            else if (result == -3)
            {
                shell_print(
                    "Error: filesystem is full.\n"
                );
            }
            else
            {
                shell_print(
                    "Error: invalid file name.\n"
                );
            }
        }
    }


    /* --------------------------------------------------------
       CAT
       -------------------------------------------------------- */

    else if (starts_with(command, "cat "))
    {
        const char* name =
            get_argument(command);

        char output[FS_DATA_LENGTH];

        if (name[0] == '\0')
        {
            shell_print(
                "Usage: cat <name>\n"
            );
        }
        else
        {
            int result =
                fs_cat(
                    name,
                    output,
                    sizeof(output)
                );

            if (result == 0)
            {
                shell_print(output);

                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print(
                    "Error: file not found.\n"
                );
            }
            else if (result == -3)
            {
                shell_print(
                    "Error: not a file.\n"
                );
            }
            else
            {
                shell_print(
                    "Error: invalid file name.\n"
                );
            }
        }
    }


    /* --------------------------------------------------------
       WRITE
       -------------------------------------------------------- */

    else if (starts_with(command, "write "))
    {
        const char* args = command + 6;

        char name[32];
        int i = 0;

        while (args[i] != '\0' &&
               args[i] != ' ' &&
               i < 31)
        {
            name[i] = args[i];
            i++;
        }

        name[i] = '\0';

        if (args[i] == '\0' || args[i + 1] == '\0')
        {
            shell_print("Usage: write <name> <text>\n");
        }
        else
        {
            const char* data = args + i + 1;
            int result = fs_write(name, data);

            if (result == 0)
            {
                shell_print("File written: ");
                shell_print(name);
                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print("Error: file not found.\n");
            }
            else if (result == -3)
            {
                shell_print("Error: not a file.\n");
            }
            else
            {
                shell_print("Error: unable to write file.\n");
            }
        }
    }


    /* --------------------------------------------------------
       RM
       -------------------------------------------------------- */

    else if (starts_with(command, "rm "))
    {
        const char* name = get_argument(command);

        if (name[0] == '\0')
        {
            shell_print("Usage: rm <name>\n");
        }
        else
        {
            int result = fs_rm(name);

            if (result == 0)
            {
                shell_print("File removed: ");
                shell_print(name);
                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print("Error: file not found.\n");
            }
            else if (result == -3)
            {
                shell_print("Error: not a file.\n");
            }
            else
            {
                shell_print("Error: unable to remove file.\n");
            }
        }
    }


    /* --------------------------------------------------------
       RMDIR
       -------------------------------------------------------- */

    else if (starts_with(command, "rmdir "))
    {
        const char* name = get_argument(command);

        if (name[0] == '\0')
        {
            shell_print("Usage: rmdir <name>\n");
        }
        else
        {
            int result = fs_rmdir(name);

            if (result == 0)
            {
                shell_print("Directory removed: ");
                shell_print(name);
                shell_print("\n");
            }
            else if (result == -2)
            {
                shell_print("Error: directory not found.\n");
            }
            else if (result == -3)
            {
                shell_print("Error: not a directory.\n");
            }
            else if (result == -4)
            {
                shell_print(
                    "Error: system directory cannot be removed.\n"
                );
            }
            else
            {
                shell_print(
                    "Error: unable to remove directory.\n"
                );
            }
        }
    }


    /* --------------------------------------------------------
       EMPTY COMMAND
       -------------------------------------------------------- */

    else if (command[0] == '\0')
    {
        /* Nothing */
    }


    /* --------------------------------------------------------
       UNKNOWN COMMAND
       -------------------------------------------------------- */
else if (string_equal(command, "scrolltest"))
{
    for (int i = 1; i <= 40; i++)
    {
        shell_print("Scroll test line ");
        print_number(i);
        shell_print("\n");
    }
}
    else
    {
        shell_print(
            "Unknown command: "
        );

        shell_print(command);

        shell_print("\n");
    }
}


/* ============================================================
   Kernel main
   ============================================================ */

void kernel_main(uint64_t multiboot_info)
{
    /* --------------------------------------------------------
       Memory
       -------------------------------------------------------- */

    memory_init(multiboot_info);


    /* --------------------------------------------------------
       Virtual filesystem
       -------------------------------------------------------- */

    fs_init();


    /* --------------------------------------------------------
       Screen
       -------------------------------------------------------- */

    clear_screen();


    /* --------------------------------------------------------
       Kernel banner
       -------------------------------------------------------- */

    shell_print(
        "ArunOS 64-bit Kernel Started!\n"
    );

    shell_print("\n");


    /* --------------------------------------------------------
       Interrupt system
       -------------------------------------------------------- */

    idt_init();

    pic_remap();

    pit_init(100);


    /* --------------------------------------------------------
       Enable interrupts
       -------------------------------------------------------- */

    __asm__ volatile ("sti");


    /* --------------------------------------------------------
       Shell
       -------------------------------------------------------- */

    shell_print(
        "ArunOS Shell v0.3\n"
    );

    shell_print(
        "Type 'help' for available commands.\n"
    );

    shell_print("\n");

    print_prompt();


    /* --------------------------------------------------------
       Main kernel loop
       -------------------------------------------------------- */

    while (1)
{
    if (command_ready)
{
    const char* command =
        keyboard_get_command();

    add_history(command);

    /*
       Move to the next line after the command.
       kernel_main.c owns the real cursor.
    */
    cursor_position =
        ((cursor_position / SCREEN_WIDTH) + 1)
        * SCREEN_WIDTH;

    ensure_cursor();

    execute_command(command);

    keyboard_clear_command();

    /*
       Synchronize keyboard input cursor
       with the kernel cursor after output
       and possible scrolling.
    */
    keyboard_set_cursor(cursor_position);

    print_prompt();
}
}if (command_ready)
{
    const char* command =
        keyboard_get_command();

    add_history(command);

    /*
       Move to the next line after the command.
       kernel_main.c owns the real cursor.
    */
    cursor_position =
        ((cursor_position / SCREEN_WIDTH) + 1)
        * SCREEN_WIDTH;

    ensure_cursor();

    execute_command(command);

    keyboard_clear_command();

    /*
       Synchronize keyboard input cursor
       with the kernel cursor after output
       and possible scrolling.
    */
    keyboard_set_cursor(cursor_position);

    print_prompt();
}
}
