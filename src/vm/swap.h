#include <bitmap.h>
#include "threads/vaddr.h"
#include "devices/block.h"

#define SECTORS_PER_PAGE PGSIZE/BLOCK_SECTOR_SIZE

void swap_table_init (size_t block_size);
void swap_in (size_t sec_idx, uint8_t *kpage);
size_t swap_out (uint8_t *kpage);
