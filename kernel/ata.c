#include <stdint.h>

/* ============================================================
   ATA PIO - Primary ATA Channel
   ============================================================ */

#define ATA_PRIMARY_DATA       0x1F0
#define ATA_PRIMARY_ERROR      0x1F1
#define ATA_PRIMARY_SECCOUNT   0x1F2
#define ATA_PRIMARY_LBA_LOW    0x1F3
#define ATA_PRIMARY_LBA_MID    0x1F4
#define ATA_PRIMARY_LBA_HIGH   0x1F5
#define ATA_PRIMARY_DRIVE      0x1F6
#define ATA_PRIMARY_COMMAND    0x1F7
#define ATA_PRIMARY_STATUS     0x1F7

#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30
#define ATA_CMD_CACHE_FLUSH    0xE7

#define ATA_STATUS_BSY         0x80
#define ATA_STATUS_DRQ         0x08
#define ATA_STATUS_ERR         0x01

static inline void ata_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t ata_inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void ata_outw(uint16_t port, uint16_t value)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint16_t ata_inw(uint16_t port)
{
    uint16_t value;

    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void ata_wait_bsy(void)
{
    while (ata_inb(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY)
    {
    }
}

static int ata_wait_drq(void)
{
    while (1)
    {
        uint8_t status =
            ata_inb(ATA_PRIMARY_STATUS);

        if (status & ATA_STATUS_ERR)
            return -1;

        if (!(status & ATA_STATUS_BSY) &&
            (status & ATA_STATUS_DRQ))
        {
            return 0;
        }
    }
}


/* ============================================================
   READ ONE SECTOR
   ============================================================ */

int ata_read_sector(
    uint32_t lba,
    uint8_t* buffer
)
{
    if (buffer == 0)
        return -1;

    ata_wait_bsy();

    ata_outb(
        ATA_PRIMARY_DRIVE,
        0xE0 | ((lba >> 24) & 0x0F)
    );

    ata_outb(
        ATA_PRIMARY_SECCOUNT,
        1
    );

    ata_outb(
        ATA_PRIMARY_LBA_LOW,
        lba & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_LBA_MID,
        (lba >> 8) & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_LBA_HIGH,
        (lba >> 16) & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_COMMAND,
        ATA_CMD_READ_SECTORS
    );

    if (ata_wait_drq() != 0)
        return -2;

    for (int i = 0; i < 256; i++)
    {
        uint16_t value =
            ata_inw(ATA_PRIMARY_DATA);

        buffer[i * 2] =
            value & 0xFF;

        buffer[i * 2 + 1] =
            (value >> 8) & 0xFF;
    }

    return 0;
}


/* ============================================================
   WRITE ONE SECTOR
   ============================================================ */

int ata_write_sector(
    uint32_t lba,
    const uint8_t* buffer
)
{
    if (buffer == 0)
        return -1;

    ata_wait_bsy();

    ata_outb(
        ATA_PRIMARY_DRIVE,
        0xE0 | ((lba >> 24) & 0x0F)
    );

    ata_outb(
        ATA_PRIMARY_SECCOUNT,
        1
    );

    ata_outb(
        ATA_PRIMARY_LBA_LOW,
        lba & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_LBA_MID,
        (lba >> 8) & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_LBA_HIGH,
        (lba >> 16) & 0xFF
    );

    ata_outb(
        ATA_PRIMARY_COMMAND,
        ATA_CMD_WRITE_SECTORS
    );

    if (ata_wait_drq() != 0)
        return -2;

    for (int i = 0; i < 256; i++)
    {
        uint16_t value =
            (uint16_t)buffer[i * 2]
            |
            ((uint16_t)buffer[i * 2 + 1] << 8);

        ata_outw(
            ATA_PRIMARY_DATA,
            value
        );
    }

    ata_outb(
        ATA_PRIMARY_COMMAND,
        ATA_CMD_CACHE_FLUSH
    );

    ata_wait_bsy();

    return 0;
}
