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

void //where to use?? - when load file or stack, find the location if it is in swap.
swap_in (size_t idx, uint8_t *kpage)
{

}

size_t 
swap_out (uint8_t *kpage)
{   
    lock_acquire(&frame_lock);
    struct ft_entry *fte = frame_table_find(kpage);
    lock_release(&frame_lock);

    struct spt_entry *spte = spt_search_page(fte->upage);

    lock_acquire(&swap_table);
    size_t ret = bitmap_scan_and_flip(swap_table, 0, 1, false);
    block_write(swap_block, ret * SECTORS_PER_PAGE, spte->upage); //need to check
    lock_release(&swap_table);

    return ret;
}