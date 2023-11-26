#include "vm/swap.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "devices/block.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

struct block *swap_block;

struct bitmap *swap_table;

struct lock swap_lock;

extern struct lock frame_lock;

void 
swap_table_init (size_t block_size)
{   
    swap_block = block_get_role(BLOCK_SWAP);
    ASSERT(swap_block != NULL);
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
    
    ASSERT (fte != NULL);

    struct spt_entry *spte = spt_search_page(fte->upage);

    ASSERT (spte != NULL);

    //write to kpage from the block.
    for (block_sector_t idx = 0; idx < SECTORS_PER_PAGE; idx++)
    {   
        block_read(swap_block, sec_idx * SECTORS_PER_PAGE + idx, fte->kpage + idx * BLOCK_SECTOR_SIZE);
    }
    lock_acquire(&swap_lock);
    bitmap_flip(swap_table, sec_idx);
    lock_release(&swap_lock);

    if (!install_page (spte->upage, kpage, spte->writable)) 
    {   
        lock_acquire(&frame_lock);
        frame_table_free_frame(kpage);
        lock_release(&frame_lock);

        spte->is_loaded = false;
        return; 
    }

    spte->kpage = kpage;
    spte->is_loaded = true;
}

size_t 
swap_out (uint8_t *kpage)
{   
    ASSERT(lock_held_by_current_thread(&frame_lock));

    struct ft_entry *fte = frame_table_find(kpage);

    ASSERT (fte != NULL);
    
    lock_acquire(&swap_lock);
    size_t saved_idx = bitmap_scan_and_flip(swap_table, 0, 1, false);
    lock_release(&swap_lock);

    //write to block from kpage.
    for (block_sector_t idx = 0; idx < SECTORS_PER_PAGE; idx++)
    {
        block_write(swap_block, saved_idx * SECTORS_PER_PAGE + idx, fte->kpage + idx * BLOCK_SECTOR_SIZE);
    }

    return saved_idx;
}