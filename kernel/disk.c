#include <stdint.h>

/* ============================================================
   ArunOS Disk Storage Layer
   ============================================================ */

#define DISK_SECTOR_SIZE 512

extern int ata_read_sector(
    uint32_t lba,
    uint8_t* buffer
);

extern int ata_write_sector(
    uint32_t lba,
    const uint8_t* buffer
);

/* ------------------------------------------------------------
   Read one disk sector
   ------------------------------------------------------------ */

int disk_read(
    uint32_t lba,
    uint8_t* buffer
)
{
    if (buffer == 0)
        return -1;

    return ata_read_sector(lba, buffer);
}

/* ------------------------------------------------------------
   Write one disk sector
   ------------------------------------------------------------ */

int disk_write(
    uint32_t lba,
    const uint8_t* buffer
)
{
    if (buffer == 0)
        return -1;

    return ata_write_sector(lba, buffer);
}
