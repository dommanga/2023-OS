#include <hash.h>
#include "threads/thread.h"
#include <stddef.h>
#include <off_t.h>

typedef struct supplemental_page_table_entry spt_entry;

enum location
{
    FILE,
    SWAP,
    ZERO
};

// spage table entry structure.
struct spt_entry
{
    uint8_t *upage;
    uint8_t *kpage;
    enum location loc;

    struct file *backed_file;

    off_t offset;
    size_t page_read_bytes;
    size_t page_zero_bytes;

    bool writable;
    struct hash_elem spage_elem;
};

void spt_init(void);
void spt_destroy(struct hash *spt);
struct spt_entry *spt_entry_init(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct spt_entry *spt_entry_init_zero(uint8_t *upage, bool writable);
bool spt_page_insert(struct spt_entry *fte);
bool spt_page_delete(struct spt_entry *fte);
struct spt_entry *spt_search_page(uint8_t *upage);
void spt_load_data_to_page(struct spt_entry *spte, uint8_t *kpage);