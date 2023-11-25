#include "vm/swap.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "devices/block.h"
#include "userprog/pagedir.h"

struct block *swap_block;

struct bitmap *swap_table;

struct lock swap_lock;

extern struct lock frame_lock;

void 
swap_table_init (size_t block_size)
{   
    swap_block = block_get_role(BLOCK_SWAP);
    swap_table = bitmap_create(block_size / SECTORS_PER_PAGE);
    bitmap_set_all(swap_table, false);
    lock_init(&swap_lock);
}

void
swap_in (size_t sec_idx, uint8_t *kpage)
{   
    lock_acquire(&frame_lock);
    struct ft_entry *fte = frame_table_find(kpage);
    lock_release(&frame_lock);

    struct spt_entry *spte = spt_search_page(fte->upage);

    ASSERT (fte != NULL);

    lock_acquire(&swap_table);
    for (block_sector_t idx = 0; idx < SECTORS_PER_PAGE; idx++)
    {
        block_read(swap_block, sec_idx * SECTORS_PER_PAGE + idx, fte->upage + idx * BLOCK_SECTOR_SIZE);
    }
    bitmap_flip(swap_table, sec_idx);
    lock_release(&swap_table);

    spte->is_loaded = true;
}

size_t 
swap_out (uint8_t *kpage)
{   
    lock_acquire(&frame_lock);
    struct ft_entry *fte = frame_table_find(kpage);
    lock_release(&frame_lock);

    lock_acquire(&swap_table);
    size_t saved_idx = bitmap_scan_and_flip(swap_table, 0, 1, false);

    for (block_sector_t idx = 0; idx < SECTORS_PER_PAGE; idx++)
    {
        block_write(swap_block, saved_idx * SECTORS_PER_PAGE + idx, fte->upage + idx * BLOCK_SECTOR_SIZE);
    }
    
    lock_release(&swap_table);

    return saved_idx;
}