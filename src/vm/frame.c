#include "vm/frame.h"
#include "threads/synch.h"

//Global variable frame_table for eviction.
struct hash frame_table;

//Thread must acquire lock for frame_table when it approach.
struct lock frame_lock;

//Initialize frame table and frame lock.
void 
init_frame_table (void)
{   
    hash_init(&frame_table, kpage_hash, kpage_less, NULL);
    lock_init(&frame_lock);
}

// Get frame and store info for frame and if needed, do eviction. If success, return the kpage and else, return NULL.
struct ft_entry *
get_frame(uint8_t *upage)
{

}

//
struct ft_entry *
find_victim()
{

}

//
void 
evict_victim(struct ft_entry *fte)
{

}

//
void 
free_all_frame(uint8_t *kpage)
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