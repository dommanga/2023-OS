#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

//synchronization of file system
static struct lock file_sys;

void syscall_init (void);

#endif /* userprog/syscall.h */
