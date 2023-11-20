#include <hash.h>
#include "threads/thread.h"

struct frame_table_entry
{
    void *kpage;
    void *upage;
    struct thread *t;
    struct hash_elem frame_elem;
};

void *get_frame(void *upage);
struct frame_table_entry *find_victim();
void evict_victim(struct frame_table_entry *fte);
void free_all_frame(void *kpage);