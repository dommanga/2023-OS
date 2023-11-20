#include <hash.h>
#include "threads/thread.h"

typedef struct frame_table_entry ft_entry;

struct ft_entry
{
    void *kpage;
    void *upage;
    struct thread *t;
    struct hash_elem frame_elem;
};

void init_frame_table (void);
void *get_frame(void *upage);
struct ft_entry *find_victim();
void evict_victim(struct ft_entry *fte);
void free_all_frame(void *kpage);