#include <hash.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>

typedef struct frame_table_entry ft_entry;

//structure for frame table entry. It must offer information about frame to support page eviction.
struct ft_entry
{
    uint8_t *kpage;
    uint8_t *upage;
    struct thread *t;
    struct list_elem frame_elem;
};

void frame_table_init (void);
struct ft_entry *frame_table_get_frame(uint8_t *upage, enum palloc_flags flag);
void frame_table_free_frame (uint8_t *kpage);
struct ft_entry *frame_table_find (uint8_t *kpage);
struct ft_entry *find_victim(void);
void evict_victim(struct ft_entry *fte);
void frame_table_free_all_frames(void);