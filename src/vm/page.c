#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"

extern struct lock file_sys;

void free_spte (struct hash_elem *e, void *aux UNUSED);
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
unsigned mmap_hash (const struct hash_elem *p_, void *aux UNUSED);
bool mmap_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);


void 
spt_init (void)
{   
    struct thread *cur = thread_current();
    hash_init(&cur->spage_table, page_hash, page_less, NULL);
}

void 
spt_destroy (struct hash *spt)
{
    hash_destroy(spt, free_spte);
}

//not yet - insert is first goal for implementation.
void 
free_spte (struct hash_elem *e, void *aux UNUSED)
{   
    struct spt_entry *spte = hash_entry(e, struct spt_entry, spage_elem);
    //write back???
    if (spte->is_loaded)
    {
        frame_table_free_frame(spte->kpage);
        pagedir_clear_page(thread_current()->pagedir, spte->upage);
    }
    free(spte);
}

//not complete - location
struct spt_entry *
spt_entry_init (struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    struct spt_entry *spte = (struct spt_entry *)malloc(sizeof(struct spt_entry));

    spte->is_loaded = false;
    spte->backed_file = file;
    spte->offset = ofs;
    spte->upage = upage;
    spte->page_read_bytes = read_bytes;
    spte->page_zero_bytes = zero_bytes;
    spte->writable = writable;
    spte->loc = FILE; // not sure...

    return spte;
}

struct spt_entry *
spt_entry_init_zero (uint8_t *upage, bool writable)
{
    struct spt_entry *spte = (struct spt_entry *)malloc(sizeof(struct spt_entry));

    spte->is_loaded = false;
    spte->upage = upage;
    spte->writable = writable;
    spte->loc = ZERO;

    return spte;
}

bool
spt_page_insert (struct spt_entry *spte)
{   
    struct thread *cur = thread_current();
    
    if (!hash_insert(&cur->spage_table, &spte->spage_elem))
        return true;
    else
        return false;
}

bool
spt_page_delete (struct spt_entry *spte)
{   
    struct thread *cur = thread_current();
    
    if (hash_delete(&cur->spage_table, &spte->spage_elem))
        {   
            free(spte);
            return true;
        }
    else
        return false;
}

struct spt_entry *
spt_search_page (uint8_t *upage)
{   
    struct thread *cur = thread_current();

    struct spt_entry *spte = (struct spt_entry *)malloc(sizeof(struct spt_entry));
    spte->upage = pg_round_down(upage);

    struct hash_elem *e = hash_find(&cur->spage_table, &spte->spage_elem);

    free(spte);

    if (!e)
        return NULL;
    return hash_entry(e, struct spt_entry, spage_elem);
}

bool 
spt_load_data_to_page (struct spt_entry *spte, uint8_t *kpage)
{   
    ASSERT (spte->is_loaded == false);

    lock_acquire(&file_sys);
    file_seek(spte->backed_file, spte->offset);

    if (file_read (spte->backed_file, kpage, spte->page_read_bytes) != (int) spte->page_read_bytes)
    { 
        lock_release(&file_sys);
        frame_table_free_frame(kpage);
        return false; 
    }
    lock_release(&file_sys);
    memset (kpage + spte->page_read_bytes, 0, spte->page_zero_bytes);
    
    /* Add the page to the process's address space. */
    if (!install_page (spte->upage, kpage, spte->writable)) 
    {
        frame_table_free_frame(kpage);
        return false; 
    }
    spte->kpage = kpage;
    spte->is_loaded = true;

    return true;
}

void 
mmapt_init (struct thread *t)
{   
    hash_init(&t->mmap_table, mmap_hash, mmap_less, NULL);
}

mapid_t
mmapt_mapping_insert(struct file *f, int fd, uint8_t *start_page)
{   
    ASSERT (f != NULL);
    struct thread *cur = thread_current();

    struct mmapt_entry *mapping = (struct mmapt_entry *)malloc(sizeof(struct mmapt_entry));
    if(mapping == NULL)
        return -1;
    
    mapping->mapid = fd;
    mapping->file = f;
    mapping->mpage = start_page;
    lock_acquire(&file_sys);
    mapping->content_size = file_length(f);
    lock_release(&file_sys);
    
    if (hash_insert(&cur->mmap_table, &mapping->mmap_elem))
        return -1;

    uint32_t read_len = mapping->content_size;
    off_t ofs = 0;
    uint8_t *upage = mapping->mpage;
    file_seek (f, ofs);

    while (ofs < mapping->content_size) 
    {
      size_t page_read_bytes = read_len < PGSIZE ? read_len : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      struct spt_entry *spte = spt_entry_init(f, ofs, upage, page_read_bytes, page_zero_bytes, true);
      spt_page_insert(spte);

      ASSERT (spte != NULL);

      /* Advance. */
      read_len -= page_read_bytes;
      upage += PGSIZE;
      ofs += PGSIZE;
    }
    
    return mapping->mapid;
}

void 
mmapt_mapping_delete (mapid_t mapid)
{   
    struct thread *cur = thread_current();
    struct mmapt_entry *mapping = mmapt_search_mapping(mapid);

    if (mapping == NULL)
        return;

    hash_delete(&cur->mmap_table, &mapping->mmap_elem);

    off_t ofs = 0;
    uint8_t *upage = mapping->mpage;

    while (ofs < mapping->content_size)
    {
        struct spt_entry *spte = spt_search_page(upage);
        if (spte != NULL)
        {
            if (pagedir_is_dirty(cur->pagedir, spte->upage))
            {   
                //write back
                lock_acquire(&file_sys);
                file_seek (spte->backed_file, spte->offset);
                file_write(spte->backed_file, (void *)spte->upage, spte->page_read_bytes);
                lock_release(&file_sys);
            }
            pagedir_clear_page(cur->pagedir, spte->upage);
            spt_page_delete(spte);
        }

        /* Advance. */
        upage += PGSIZE;
        ofs += PGSIZE;
    }
    
    lock_acquire(&file_sys);
    file_close(mapping->file);
    lock_release(&file_sys);
    
    free(mapping);
}

struct mmapt_entry *
mmapt_search_mapping(mapid_t mapid)
{
    struct thread *cur = thread_current();

    struct mmapt_entry *mapping = (struct mmapt_entry *)malloc(sizeof(struct mmapt_entry));
    mapping->mapid = mapid;

    struct hash_elem *e = hash_find(&cur->mmap_table, &mapping->mmap_elem);

    free(mapping);

    if (!e)
        return NULL;
    return hash_entry(e, struct mmapt_entry, mmap_elem);
}

//Return hash value corresponding to p.
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
    const struct spt_entry *p = hash_entry(p_, struct spt_entry, spage_elem);
    return hash_bytes(&p->upage, sizeof p->upage);
}

//Return true if kpage a precedes kpage b.
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct spt_entry *a = hash_entry(a_, struct spt_entry, spage_elem);
    const struct spt_entry *b = hash_entry(b_, struct spt_entry, spage_elem);

    return a->upage < b->upage;
}

unsigned
mmap_hash (const struct hash_elem *p_, void *aux UNUSED)
{
    const struct mmapt_entry *p = hash_entry(p_, struct mmapt_entry, mmap_elem);
    return hash_int(p->mapid);
}

bool
mmap_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct mmapt_entry *a = hash_entry(a_, struct mmapt_entry, mmap_elem);
    const struct mmapt_entry *b = hash_entry(b_, struct mmapt_entry, mmap_elem);

    return a->mapid < b->mapid;
}