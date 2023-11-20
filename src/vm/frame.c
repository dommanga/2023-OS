#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/malloc.h"

//Global variable frame_table for eviction.
struct hash frame_table;

//Thread must acquire lock for frame_table when it approach.
struct lock frame_lock;

//Initialize frame table and frame lock.
void 
frame_table_init (void)
{   
    hash_init(&frame_table, kpage_hash, kpage_less, NULL);
    lock_init(&frame_lock);
}

// Get frame and store info for frame and if needed, do eviction. If success, return the ft_entry.
struct ft_entry *
frame_table_get_frame (uint8_t *upage)
{
    uint8_t *kpage = palloc_get_page (PAL_USER);

    if (kpage == NULL)
    {
        struct ft_entry *fte = find_victim();
        evict_victim(fte);
        kpage = palloc_get_page (PAL_USER);
    }

    ASSERT(kpage != NULL);

    struct ft_entry *fte = (struct ft_entry *)malloc(sizeof(struct ft_entry));

    fte->kpage = kpage;
    fte->t = thread_current();
    fte->upage = upage;

    //frame_table insert
    bool result = kpage_insert(fte);
    ASSERT (result == true);
    
    return fte;
}

//not yet
struct ft_entry *
find_victim ()
{
    return NULL;
}

//not yet
void 
evict_victim (struct ft_entry *fte)
{

}

//
void 
frame_table_free_all_frame (uint8_t *kpage)
{

}

//Return hash value corresponding to p.
unsigned
kpage_hash (const struct hash_elem *p_, void *aux UNUSED)
{
    const struct ft_entry *p = hash_entry(p_, struct ft_entry, frame_elem);
    return hash_bytes(&p->kpage, sizeof p->kpage);
}

//Return true if kpage a precedes kpage b.
bool
kpage_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct ft_entry *a = hash_entry(a_, struct ft_entry, frame_elem);
    const struct ft_entry *b = hash_entry(b_, struct ft_entry, frame_elem);

    return a->kpage < b->kpage;
}

bool
kpage_insert (struct ft_entry *fte)
{
    bool ret = hash_insert(&frame_table, &fte->frame_elem);
    
    if (ret == NULL)
        return true;
    else
        return false;
}

bool
kpage_delete (struct ft_entry *fte)
{
    bool ret = hash_delete(&frame_table, &fte->frame_elem);
    
    if (ret == NULL)
        return true;
    else
        return false;
}