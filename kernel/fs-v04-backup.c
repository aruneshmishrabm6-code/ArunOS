#include <stdint.h>

/* ============================================================
   ArunOS Virtual File System v0.4
   ============================================================ */

#define FS_MAX_ENTRIES 32
#define FS_NAME_LENGTH 32
#define FS_DATA_LENGTH 128

#define FS_FILE 1
#define FS_DIRECTORY 2

struct fs_entry
{
    int used;
    int type;
    char name[FS_NAME_LENGTH];
    char data[FS_DATA_LENGTH];
};

static struct fs_entry entries[FS_MAX_ENTRIES];
static int fs_initialized = 0;

static void fs_strcpy(char* destination, const char* source)
{
    int i = 0;

    while (source[i] != '\0' && i < FS_NAME_LENGTH - 1)
    {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

static void fs_data_copy(char* destination, const char* source)
{
    int i = 0;

    while (source[i] != '\0' && i < FS_DATA_LENGTH - 1)
    {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

static int fs_streq(const char* a, const char* b)
{
    int i = 0;

    while (a[i] != '\0' && b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return a[i] == '\0' && b[i] == '\0';
}

static int fs_find(const char* name)
{
    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (entries[i].used &&
            fs_streq(entries[i].name, name))
        {
            return i;
        }
    }

    return -1;
}

void fs_init(void)
{
    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        entries[i].used = 0;
        entries[i].type = 0;
        entries[i].name[0] = '\0';
        entries[i].data[0] = '\0';
    }

    /* Default file */
    entries[0].used = 1;
    entries[0].type = FS_FILE;

    fs_strcpy(entries[0].name, "hello.txt");
    fs_data_copy(entries[0].data, "Hello from ArunOS!");

    /* Default directory */
    entries[1].used = 1;
    entries[1].type = FS_DIRECTORY;

    fs_strcpy(entries[1].name, "system");

    fs_initialized = 1;
}

/* ============================================================
   LS
   ============================================================ */

void fs_list(void)
{
    if (!fs_initialized)
        return;

    extern void shell_print(const char* text);

    int found = 0;

    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
            continue;

        shell_print(entries[i].name);

        if (entries[i].type == FS_DIRECTORY)
            shell_print("/");

        shell_print("\n");

        found = 1;
    }

    if (!found)
        shell_print("Filesystem is empty.\n");
}

/* ============================================================
   MKDIR
   ============================================================ */

int fs_mkdir(const char* name)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    if (fs_find(name) >= 0)
        return -2;

    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
        {
            entries[i].used = 1;
            entries[i].type = FS_DIRECTORY;

            fs_strcpy(entries[i].name, name);

            entries[i].data[0] = '\0';

            return 0;
        }
    }

    return -3;
}

/* ============================================================
   TOUCH
   ============================================================ */

int fs_touch(const char* name)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    if (fs_find(name) >= 0)
        return -2;

    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
        {
            entries[i].used = 1;
            entries[i].type = FS_FILE;

            fs_strcpy(entries[i].name, name);

            entries[i].data[0] = '\0';

            return 0;
        }
    }

    return -3;
}

/* ============================================================
   CAT
   ============================================================ */

int fs_cat(const char* name, char* output, int output_size)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    int index = fs_find(name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_FILE)
        return -3;

    int i = 0;

    while (entries[index].data[i] != '\0' &&
           i < output_size - 1)
    {
        output[i] = entries[index].data[i];
        i++;
    }

    output[i] = '\0';

    return 0;
}

/* ============================================================
   WRITE
   ============================================================ */

int fs_write(const char* name, const char* data)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    if (data == 0)
        return -1;

    int index = fs_find(name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_FILE)
        return -3;

    fs_data_copy(entries[index].data, data);

    return 0;
}

/* ============================================================
   RM
   ============================================================ */

int fs_rm(const char* name)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    int index = fs_find(name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_FILE)
        return -3;

    entries[index].used = 0;
    entries[index].type = 0;
    entries[index].name[0] = '\0';
    entries[index].data[0] = '\0';

    return 0;
}

/* ============================================================
   RMDIR
   ============================================================ */

int fs_rmdir(const char* name)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    int index = fs_find(name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_DIRECTORY)
        return -3;

    /* Protect the default system directory */
    if (fs_streq(name, "system"))
        return -4;

    entries[index].used = 0;
    entries[index].type = 0;
    entries[index].name[0] = '\0';
    entries[index].data[0] = '\0';

    return 0;
}

/* ============================================================
   CHECK DIRECTORY
   ============================================================ */

int fs_is_directory(const char* name)
{
    if (!fs_initialized)
        return 0;

    if (name == 0 || name[0] == '\0')
        return 0;

    int index = fs_find(name);

    if (index < 0)
        return 0;

    return entries[index].type == FS_DIRECTORY;
}
