#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

//Global variable frame_table for eviction.
struct list frame_table;

//Thread must acquire lock for frame_table when it approach.
struct lock frame_lock;

//Initialize frame table and frame lock.
void 
frame_table_init (void)
{   
    list_init(&frame_table);
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

    lock_acquire(&frame_lock);
    list_push_back(&frame_table, &fte->frame_elem);
    lock_release(&frame_lock);
    
    return fte;
}

void
frame_table_free_frame (uint8_t *kpage)
{   
    lock_acquire(&frame_lock);

    struct ft_entry *fte = frame_table_find(kpage);
    palloc_free_page(fte->kpage);
    list_remove(&fte->frame_elem);

    lock_release(&frame_lock);
    
    free(fte);
}

struct ft_entry *
frame_table_find (uint8_t *kpage)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));

    struct list_elem *e = list_begin(&frame_table);
    for(e; e != list_end(&frame_table); e = list_next(e))
    {
        struct ft_entry *fte = list_entry(e, struct ft_entry, frame_elem);

        if(fte->kpage == kpage)
        {
            return fte;
        }
    }

    return NULL;
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
    
    for (struct list_elem *e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
    {
        struct ft_entry *fte = list_entry(e, struct ft_entry, frame_elem);

        if(fte->t == cur)
        {
            frame_table_free_frame(fte->kpage);
        }
    }

}