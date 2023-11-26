#include "vm/frame.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/page.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"

//Global variable frame_table for eviction.
struct list frame_table;

//Thread must acquire lock for frame_table when it approach.
struct lock frame_lock;

extern struct lock file_sys;

//Initialize frame table and frame lock.
void 
frame_table_init (void)
{   
    list_init(&frame_table);
    lock_init(&frame_lock);
}

// Get frame and store info for frame and if needed, do eviction. If success, return the ft_entry. NOTE THAT this function always PIN the current allocating kpage.
struct ft_entry*
frame_table_get_frame (uint8_t *upage, enum palloc_flags flag)
{   struct thread *cur = thread_current();

    lock_acquire(&frame_lock);

    uint8_t *kpage = palloc_get_page (flag);

    if (kpage == NULL)
    {   
        struct ft_entry *fte = find_victim();
        ASSERT (fte != NULL);

        evict_victim(fte);

        kpage = palloc_get_page (flag);
    }

    lock_release(&frame_lock);

    ASSERT(kpage != NULL);

    struct ft_entry *fte = (struct ft_entry *)malloc(sizeof(struct ft_entry));

    fte->kpage = kpage;
    fte->t = cur;
    fte->upage = upage;
    fte->pin = true;

    lock_acquire(&frame_lock);
    list_push_back(&frame_table, &fte->frame_elem);
    lock_release(&frame_lock);

    return fte;
}

//All operations should acquire frame lock before free frame.
void
frame_table_free_frame (uint8_t *kpage)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));

    struct ft_entry *fte = frame_table_find(kpage);

    ASSERT (fte != NULL);

    palloc_free_page(fte->kpage);
    list_remove(&fte->frame_elem);

    free(fte);
}

struct ft_entry *
frame_table_find (uint8_t *kpage)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));
    ASSERT (pg_ofs(kpage) == 0);

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


struct ft_entry *
find_victim (void)
{   
    ASSERT (lock_held_by_current_thread(&frame_lock));

    struct list_elem *e = list_begin(&frame_table);

    while(true)
    {   
        if (e == list_end(&frame_table))
            e = list_begin(&frame_table);
        
        struct ft_entry *fte = list_entry(e, struct ft_entry, frame_elem);
        struct thread *t = fte->t;

        if(pagedir_is_accessed(t->pagedir, fte->upage))
        {
            pagedir_set_accessed(t->pagedir, fte->upage, false);
        }
        else
        {   
            if (fte->pin)
                continue;
            return fte;
        }
    }
}

void 
evict_victim (struct ft_entry *fte)
{   
    ASSERT (fte != NULL);
    ASSERT (lock_held_by_current_thread(&frame_lock));

    struct thread *t = fte->t;
    struct spt_entry *spte = spt_search_page_from_thread(t, fte->upage);

    ASSERT (spte != NULL);
    
    if (spte->loc == FILE) //mmapped
    {   
        if (pagedir_is_dirty(t->pagedir, spte->upage))
        {
            //write back
            lock_acquire(&file_sys);
            file_seek (spte->backed_file, spte->offset);
            file_write(spte->backed_file, (void *)spte->upage, spte->page_read_bytes);
            lock_release(&file_sys);
        }
    }
    else //ON_FRAME
    {   
        spte->loc = SWAP;
        spte->swap_idx = swap_out(fte->kpage);
    }
    
    spte->is_loaded = false;

    pagedir_clear_page(t->pagedir, spte->upage);
    frame_table_free_frame(fte->kpage);
}

void 
frame_table_unpin (uint8_t *kpage)
{   
    lock_acquire(&frame_lock);
    struct ft_entry *fte = frame_table_find(kpage);
    ASSERT (fte != NULL);
    lock_release(&frame_lock);

    fte->pin = false;
}

void 
frame_table_unpin_all_frames (void *buffer, unsigned size)
{   
    void* p = pg_round_down(buffer);
    for (p; p < buffer + size; p = pg_round_up((void *)(uintptr_t)p + 1))
  { 
    struct spt_entry *spte = spt_search_page(p);
    frame_table_unpin(spte->kpage);
  }
}