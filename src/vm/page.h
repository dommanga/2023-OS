#include <hash.h>
#include "threads/thread.h"
#include <stddef.h>
#include "filesys/off_t.h"

typedef struct supplemental_page_table_entry spt_entry;
typedef struct mmap_table_entry mmapt_entry;

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

    bool is_loaded;

    struct file *backed_file;

    off_t offset;
    size_t page_read_bytes;
    size_t page_zero_bytes;

    bool writable;
    struct hash_elem spage_elem;
};

struct mmapt_entry
{
    mapid_t mapid;
    struct file *file;
    uint8_t *mpage;
    uint32_t content_size;
    struct hash_elem mmap_elem;
};

void spt_init(void);
void spt_destroy(struct hash *spt);
struct spt_entry *spt_entry_init(struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct spt_entry *spt_entry_init_zero(uint8_t *upage, bool writable);
bool spt_page_insert(struct spt_entry *spte);
bool spt_page_delete(struct spt_entry *spte);
struct spt_entry *spt_search_page(uint8_t *upage);
bool spt_load_data_to_page(struct spt_entry *spte, uint8_t *kpage);

void mmapt_init(void);
mapid_t mmapt_mapping_insert(struct file *f, uint8_t *start_page);
void mmapt_mapping_delete(mapid_t mapid);
struct mmapt_entry *mmapt_search_mapping(mapid_t mapid);