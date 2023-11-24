#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "filesys/file.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
int process_store_new_file (struct file *f);
struct file *process_get_file (int fd);
void process_close_file (int fd);
bool grow_stack (uint8_t *addr);
bool stack_access (void *esp, void *fault_addr);
bool install_page (void *upage, void *kpage, bool writable);
#endif /* userprog/process.h */
