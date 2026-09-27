#include <stdint.h>

#define MULTIBOOT2_TAG_TYPE_END          0
#define MULTIBOOT2_TAG_TYPE_MMAP         6

#define MULTIBOOT_MEMORY_AVAILABLE       1

struct multiboot_tag
{
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_mmap
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot_mmap_entry
{
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

static uint64_t total_usable_memory = 0;
#define PAGE_SIZE 4096

static uint8_t* page_bitmap = 0;
static uint64_t bitmap_pages = 0;
static uint64_t total_pages = 0;
static uint64_t highest_usable_address = 0;
static uint64_t pages_for_bytes(uint64_t bytes)
{
    return (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}
static void bitmap_set(uint64_t page)
{
    page_bitmap[page / 8] |=
        (uint8_t)(1 << (page % 8));
}

static void bitmap_clear(uint64_t page)
{
    page_bitmap[page / 8] &=
        (uint8_t)~(1 << (page % 8));
}

static int bitmap_test(uint64_t page)
{
    return page_bitmap[page / 8] &
           (uint8_t)(1 << (page % 8));
}

void memory_init(uint64_t multiboot_info)
{
    total_usable_memory = 0;
    highest_usable_address = 0;
    total_pages = 0;
    bitmap_pages = 0;

    uint32_t total_size =
        *(uint32_t*)(uintptr_t)multiboot_info;

    uint8_t* current =
        (uint8_t*)(uintptr_t)(multiboot_info + 8);

    uint8_t* end =
        (uint8_t*)(uintptr_t)(multiboot_info + total_size);

    while (current < end)
    {
        struct multiboot_tag* tag =
            (struct multiboot_tag*)current;

        if (tag->type == MULTIBOOT2_TAG_TYPE_END)
            break;

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP)
        {
            struct multiboot_tag_mmap* mmap =
                (struct multiboot_tag_mmap*)current;

            uint8_t* entry_ptr =
                current + 16;

            while (entry_ptr < current + mmap->size)
            {
                struct multiboot_mmap_entry* entry =
                    (struct multiboot_mmap_entry*)entry_ptr;

                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
{
    total_usable_memory += entry->len;

    uint64_t end_address =
        entry->addr + entry->len;

    if (end_address > highest_usable_address)
    {
        highest_usable_address = end_address;
    }
}

                entry_ptr += mmap->entry_size;
            }
        }

        current += (tag->size + 7) & ~7;
    }
    total_pages = pages_for_bytes(highest_usable_address);
bitmap_pages = pages_for_bytes((total_pages + 7) / 8);
}

uint64_t memory_get_usable(void)
{
    return total_usable_memory;
}
