#include <stdint.h>

/* ============================================================
   ArunOS Persistent Filesystem - Disk Storage
   ============================================================ */


extern int fs_disk_format(void);
extern int fs_disk_check(void);

extern int fs_disk_test_entry(void);
extern int fs_disk_test_entry_read(void);

extern int disk_read(
    uint32_t lba,
    uint8_t* buffer
);

extern int disk_write(
    uint32_t lba,
    const uint8_t* buffer
);

/* ------------------------------------------------------------
   Helper: clear sector buffer
   ------------------------------------------------------------ */

static void fs_disk_clear_buffer(uint8_t* buffer)
{
    for (int i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }
}

/* ------------------------------------------------------------
   Format filesystem metadata
   ------------------------------------------------------------ */

int fs_disk_format(void)
{
    uint8_t buffer[512];

    fs_disk_clear_buffer(buffer);

    /* Magic: ARUN */
    buffer[0] = 'A';
    buffer[1] = 'R';
    buffer[2] = 'U';
    buffer[3] = 'N';

    /* Version */
    buffer[4] = FS_DISK_VERSION;

    /* Metadata LBA */
    buffer[8] =
        FS_METADATA_LBA & 0xFF;

    buffer[9] =
        (FS_METADATA_LBA >> 8) & 0xFF;

    buffer[10] =
        (FS_METADATA_LBA >> 16) & 0xFF;

    buffer[11] =
        (FS_METADATA_LBA >> 24) & 0xFF;

    /* Entry table LBA */
    buffer[12] =
        FS_ENTRIES_LBA & 0xFF;

    buffer[13] =
        (FS_ENTRIES_LBA >> 8) & 0xFF;

    buffer[14] =
        (FS_ENTRIES_LBA >> 16) & 0xFF;

    buffer[15] =
        (FS_ENTRIES_LBA >> 24) & 0xFF;

    /* Maximum entries */
    buffer[16] =
        FS_DISK_MAX_ENTRIES;

    int result =
        disk_write(
            FS_METADATA_LBA,
            buffer
        );

    if (result != 0)
        return -1;

    /* Clear entry table sector */
    fs_disk_clear_buffer(buffer);

    result =
        disk_write(
            FS_ENTRIES_LBA,
            buffer
        );

    if (result != 0)
        return -2;

    return 0;
}

/* ------------------------------------------------------------
   Check filesystem metadata
   ------------------------------------------------------------ */

int fs_disk_check(void)
{
    uint8_t buffer[512];

    int result =
        disk_read(
            FS_METADATA_LBA,
            buffer
        );

    if (result != 0)
        return -1;

    if (buffer[0] != 'A' ||
        buffer[1] != 'R' ||
        buffer[2] != 'U' ||
        buffer[3] != 'N')
    {
        return -2;
    }

    if (buffer[4] != FS_DISK_VERSION)
        return -3;

    return 0;
}
/* ------------------------------------------------------------
   Write one filesystem test entry
   ------------------------------------------------------------ */

int fs_disk_test_entry(void)
{
    uint8_t buffer[512];

    for (int i = 0; i < 512; i++)
    {
        buffer[i] = 0;
    }

    /* Entry 0 = root directory */
    buffer[0] = 1;          /* used */
    buffer[1] = 2;          /* directory */
    buffer[2] = 0xFF;       /* parent = -1 */
    buffer[3] = 0xFF;

    /* Name: root */
    buffer[4] = '/';

    return disk_write(
        FS_ENTRIES_LBA,
        buffer
    );
}

/* ------------------------------------------------------------
   Read and verify one filesystem test entry
   ------------------------------------------------------------ */

int fs_disk_test_entry_read(void)
{
    uint8_t buffer[512];

    int result =
        disk_read(
            FS_ENTRIES_LBA,
            buffer
        );

    if (result != 0)
        return -1;

    if (buffer[0] != 1)
        return -2;

    if (buffer[1] != 2)
        return -3;

    if (buffer[4] != '/')
        return -4;

    return 0;
}
