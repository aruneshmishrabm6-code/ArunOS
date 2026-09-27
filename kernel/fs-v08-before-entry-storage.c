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
    int parent;
    char name[FS_NAME_LENGTH];
    char data[FS_DATA_LENGTH];
};

static struct fs_entry entries[FS_MAX_ENTRIES];
static int fs_initialized = 0;
static int current_directory = 0;

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
static int fs_find_child(int parent, const char* name)
{
    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (entries[i].used &&
            entries[i].parent == parent &&
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
        entries[i].parent = -1;
        entries[i].name[0] = '\0';
        entries[i].data[0] = '\0';
    }

    /* Root directory */
    entries[0].used = 1;
    entries[0].type = FS_DIRECTORY;
    entries[0].parent = -1;

    fs_strcpy(entries[0].name, "/");

    /* Default file */
    entries[1].used = 1;
    entries[1].type = FS_FILE;
    entries[1].parent = 0;

    fs_strcpy(entries[1].name, "hello.txt");
    fs_data_copy(entries[1].data, "Hello from ArunOS!");

    /* Default directory */
    entries[2].used = 1;
    entries[2].type = FS_DIRECTORY;
    entries[2].parent = 0;

    fs_strcpy(entries[2].name, "system");

    /* Start at root */
    current_directory = 0;

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

        if (entries[i].parent != current_directory)
            continue;

        shell_print(entries[i].name);

        if (entries[i].type == FS_DIRECTORY)
            shell_print("/");

        shell_print("\n");

        found = 1;
    }

    if (!found)
        shell_print("Directory is empty.\n");
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

    if (fs_streq(name, ".") ||
        fs_streq(name, ".."))
        return -1;

    if (fs_find_child(current_directory, name) >= 0)
        return -2;

    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
        {
            entries[i].used = 1;
            entries[i].type = FS_DIRECTORY;
            entries[i].parent = current_directory;

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

    if (fs_streq(name, ".") ||
        fs_streq(name, ".."))
        return -1;

    if (fs_find_child(current_directory, name) >= 0)
        return -2;

    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (!entries[i].used)
        {
            entries[i].used = 1;
            entries[i].type = FS_FILE;
            entries[i].parent = current_directory;

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

    if (output == 0 || output_size <= 0)
        return -1;

    int index = fs_find_child(current_directory, name);

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

    int index = fs_find_child(current_directory, name);

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

    int index = fs_find_child(current_directory, name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_FILE)
        return -3;

    entries[index].used = 0;
    entries[index].type = 0;
    entries[index].parent = -1;
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

    int index = fs_find_child(current_directory, name);

    if (index < 0)
        return -2;

    if (entries[index].type != FS_DIRECTORY)
        return -3;

    /* Protect root */
    if (index == 0)
        return -4;

    /* Protect default system directory */
    if (entries[index].parent == 0 &&
        fs_streq(entries[index].name, "system"))
        return -4;

    /* Directory must be empty */
    for (int i = 0; i < FS_MAX_ENTRIES; i++)
    {
        if (entries[i].used &&
            entries[i].parent == index)
        {
            return -5;
        }
    }

    entries[index].used = 0;
    entries[index].type = 0;
    entries[index].parent = -1;
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

    if (fs_streq(name, "/"))
        return 1;

    int index = fs_find_child(current_directory, name);

    if (index < 0)
        return 0;

    return entries[index].type == FS_DIRECTORY;
}

/* ============================================================
   CHANGE DIRECTORY
   ============================================================ */

int fs_cd(const char* name)
{
    if (!fs_initialized)
        return -1;

    if (name == 0 || name[0] == '\0')
        return -1;

    /*
       Start from current directory for
       relative paths.
    */
    int target_directory = current_directory;

    /*
       Absolute path starts from root.
    */
    if (name[0] == '/')
        target_directory = 0;

    int i = 0;

    while (name[i] != '\0')
    {
        /*
           Skip '/'
        */
        while (name[i] == '/')
            i++;

        if (name[i] == '\0')
            break;

        /*
           Read one path component.
        */
        char component[FS_NAME_LENGTH];

        int j = 0;

        while (name[i] != '\0' &&
               name[i] != '/')
        {
            if (j < FS_NAME_LENGTH - 1)
            {
                component[j] = name[i];
                j++;
            }

            i++;
        }

        component[j] = '\0';

        /*
           cd .
        */
        if (fs_streq(component, "."))
        {
            continue;
        }

        /*
           cd ..
        */
        if (fs_streq(component, ".."))
        {
            if (target_directory != 0)
            {
                target_directory =
                    entries[target_directory].parent;
            }

            continue;
        }

        /*
           Find directory inside current
           target directory.
        */
        int index =
            fs_find_child(
                target_directory,
                component
            );

        if (index < 0)
            return -2;

        if (entries[index].type != FS_DIRECTORY)
            return -3;

        target_directory = index;
    }

    /*
       Change directory only after the
       complete path was successfully checked.
    */
    current_directory = target_directory;

    return 0;
}



/* ============================================================
   GET CURRENT PATH
   ============================================================ */

int fs_get_current_path(char* output, int output_size)
{
    if (!fs_initialized)
        return -1;

    if (output == 0 || output_size <= 0)
        return -1;

    /* Root */
    if (current_directory == 0)
    {
        if (output_size < 2)
            return -1;

        output[0] = '/';
        output[1] = '\0';

        return 0;
    }

    int path[FS_MAX_ENTRIES];
    int count = 0;
    int index = current_directory;

    /* Collect directory indexes */
    while (index != 0 &&
           count < FS_MAX_ENTRIES)
    {
        path[count] = index;
        count++;

        index = entries[index].parent;
    }

    int position = 0;

    /* Start with / */
    if (position < output_size - 1)
    {
        output[position] = '/';
        position++;
    }

    /* Build path from root to current directory */
    for (int i = count - 1; i >= 0; i--)
    {
        int j = 0;

        while (entries[path[i]].name[j] != '\0')
        {
            if (position >= output_size - 1)
            {
                output[position] = '\0';
                return -2;
            }

            output[position] =
                entries[path[i]].name[j];

            position++;
            j++;
        }

        if (i > 0)
        {
            if (position >= output_size - 1)
            {
                output[position] = '\0';
                return -2;
            }

            output[position] = '/';
            position++;
        }
    }

    output[position] = '\0';

    return 0;
}
