#include <hash.h>
#include "threads/thread.h"

typedef struct frame_table_entry ft_entry;

//structure for frame table entry
struct ft_entry
{
    uint8_t *kpage;
    uint8_t *upage;
    struct thread *t;
    struct hash_elem frame_elem;
};

void init_frame_table (void);
struct ft_entry *get_frame(uint8_t *upage);
struct ft_entry *find_victim();
void evict_victim(struct ft_entry *fte);
void free_all_frame(uint8_t *kpage);