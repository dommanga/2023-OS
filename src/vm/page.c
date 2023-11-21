#include "vm/page.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"


void free_spte (struct hash_elem *e, void *aux UNUSED);
unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);


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
    //get spte
    //write back???
    //spte is loaded (mapped with frame) => frame free
    //spte itself free (free(spte))
}

//not complete - location
struct spt_entry *
spt_entry_init (struct file *file, off_t ofs, uint8_t *upage, uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
    struct spt_entry *spte = (struct spt_entry *)malloc(sizeof(struct spt_entry));

    spte->backed_file = file;
    spte->offset = ofs;
    spte->upage = upage;
    spte->page_read_bytes = read_bytes;
    spte->page_zero_bytes = zero_bytes;
    spte->writable = writable;
    spte->loc = FILE; // not sure...
}

struct spt_entry *
spt_entry_init_zero (uint8_t *upage, bool writable)
{
    struct spt_entry *spte = (struct spt_entry *)malloc(sizeof(struct spt_entry));

    spte->upage = upage;
    spte->writable = writable;
    spte->loc = ZERO;
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
        return true;
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

void 
spt_load_data_to_page (struct spt_entry *spte, uint8_t *kpage)
{

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