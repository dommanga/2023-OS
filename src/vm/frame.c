#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

//Global variable frame_table for eviction.
struct hash frame_table;

//Thread must acquire lock for frame_table when it approach.
struct lock frame_lock;

unsigned frame_hash (const struct hash_elem *p_, void *aux UNUSED);
bool frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED);
bool frame_insert (struct ft_entry *fte);
bool frame_delete (struct ft_entry *fte);

//Initialize frame table and frame lock.
void 
frame_table_init (void)
{   
    hash_init(&frame_table, frame_hash, frame_less, NULL);
    lock_init(&frame_lock);
}

// Get frame and store info for frame and if needed, do eviction. If success, return the ft_entry.
struct ft_entry*
frame_table_get_frame (uint8_t *upage, enum palloc_flags flag)
{   struct thread *cur = thread_current();
    uint8_t *kpage = palloc_get_page (flag);

    if (kpage == NULL)
    {
        struct ft_entry *fte = find_victim();
        evict_victim(fte);
        kpage = palloc_get_page (flag);
    }

    ASSERT(kpage != NULL);

    struct ft_entry *fte = (struct ft_entry *)malloc(sizeof(struct ft_entry));

    fte->kpage = kpage;
    fte->t = cur;
    fte->upage = upage;

    list_push_back(&cur->frame_own_list, &fte->list_elem);

    lock_acquire(&frame_lock);
    bool result = frame_insert(fte);
    lock_release(&frame_lock);

    ASSERT (result == true);
    
    return fte;
}

void
frame_table_free_frame (uint8_t *kpage)
{   
    lock_acquire(&frame_lock);
    struct ft_entry *fte = frame_table_find(kpage);

    palloc_free_page(fte->kpage);
    frame_delete(fte);

    lock_release(&frame_lock);
    list_remove(&fte->list_elem);
    free(fte);
}

struct ft_entry *
frame_table_find (uint8_t *kpage)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));

    struct ft_entry *fte = (struct ft_entry *)malloc(sizeof(struct ft_entry));
    fte->kpage = pg_round_down(kpage);

    struct hash_elem *e = hash_find(&frame_table, &fte->frame_elem);

    free(fte);

    if (!e)
        return NULL;
    return hash_entry(e, struct ft_entry, frame_elem);
}

//not yet
struct ft_entry *
find_victim (void)
{
    return NULL;
}

//not yet
void 
evict_victim (struct ft_entry *fte)
{

}

//free all frames held by current thread.
void 
frame_table_free_all_frames (void)
{
    struct thread *cur = thread_current();
    
    //find frame, thread must maintain own frame_list.
    for (struct list_elem *e = list_begin(&cur->frame_own_list); e != list_end(&cur->frame_own_list); e = list_next(e))
    {
        struct ft_entry *fte = list_entry(e, struct ft_entry, list_elem);
        frame_table_free_frame(fte->kpage);
    }

}

//Return hash value corresponding to p.
unsigned
frame_hash (const struct hash_elem *p_, void *aux UNUSED)
{
    const struct ft_entry *p = hash_entry(p_, struct ft_entry, frame_elem);
    return hash_bytes(&p->kpage, sizeof p->kpage);
}

//Return true if kpage a precedes kpage b.
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_, void *aux UNUSED)
{
    const struct ft_entry *a = hash_entry(a_, struct ft_entry, frame_elem);
    const struct ft_entry *b = hash_entry(b_, struct ft_entry, frame_elem);

    return a->kpage < b->kpage;
}

bool
frame_insert (struct ft_entry *fte)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));
    
    if (!hash_insert(&frame_table, &fte->frame_elem))
        return true;
    else
        return false;
}

bool
frame_delete (struct ft_entry *fte)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));
    
    if (hash_delete(&frame_table, &fte->frame_elem))
        return true;
    else
        return false;
}