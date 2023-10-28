#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
void get_arg (void *esp, int *arg, int count);
void check_validation (void *p);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 
  check_validation(f->esp);
  int syscall_num = *(uint32_t *)f->esp;
  int arg[5];

 switch (syscall_num)
 {
  case SYS_HALT:
    halt();
    break;
  case SYS_EXIT:
    get_arg(f->esp, arg, 1);
    exit((int)arg[0]);
    break;
  case SYS_EXEC:
    get_arg(f->esp, arg, 1);
    check_validation((void *)arg[0]);
    f->eax = exec((const char *)arg[0]);
    break;
  case SYS_WAIT:
    get_arg(f->esp, arg, 1);
    f->eax = wait((pid_t)arg[0]);
    break;
  case SYS_CREATE:
    get_arg(f->esp, arg, 2);
    check_validation((void *)arg[0]);
    f->eax = create((const char *)arg[0], (unsigned int)arg[1]);
    break;
  case SYS_REMOVE:
    get_arg(f->esp, arg, 1);
    check_validation((void *)arg[0]);
    f->eax = remove((const char *)arg[0]);
    break;
  case SYS_OPEN:
    get_arg(f->esp, arg, 1);
    check_validation((void *)arg[0]);
    f->eax = open((const char *)arg[0]);
    break;
  case SYS_FILESIZE:
    get_arg(f->esp, arg, 1);
    f->eax = filesize((int)arg[0]);
    break;
  case SYS_READ:
    get_arg(f->esp, arg, 3);
    check_validation((void *)arg[1]);
    f->eax = read((int)arg[0], (void *)arg[1], (unsigned int)arg[2]);
    break;
  case SYS_WRITE:
    get_arg(f->esp, arg, 3);
    check_validation((void *)arg[1]);
    f->eax = write((int)arg[0], (const void *)arg[1], (unsigned int)arg[2]);
    break;
  case SYS_SEEK:
    get_arg(f->esp, arg, 2);
    seek((int)arg[0], (unsigned int)arg[1]);
    break;
  case SYS_TELL:
    get_arg(f->esp, arg, 1);
    f->eax = tell((int)arg[0]);
    break;
  case SYS_CLOSE:
    get_arg(f->esp, arg, 1);
    close((int)arg[0]);
    break;
 } 
}

//get argument data from user stack to kernel.
void get_arg (void *esp, int *arg, int count)
{
  for (int i = 0; i < count; i++)
  { 
    check_validation(esp + 4 * (i + 1)); //address of esp itself must be valid.
    arg[i] = *(uint32_t *)(esp + 4 * (i + 1));
  }
}

//check validation of pointer.
void check_validation (void *p)
{ 
  struct thread *t = thread_current();

  if (p == NULL || is_kernel_vaddr(p) || pagedir_get_page(t->pagedir, p) == NULL)
    exit(-1);
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{ 
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

pid_t exec (const char *cmd_line)
{ 
  tid_t child_tid = process_execute(cmd_line);
  
  struct thread *cur = thread_current();
  struct list_elem *e;

  if (!list_empty(&cur->child_list))
  {
    for (e = list_begin(&cur->child_list); e != list_end(&cur->child_list); e = list_next(e))
    {
      struct thread *t = list_entry(e, struct thread, child_elem);
      if (t->tid == child_tid)
      {
        sema_down(&t->loaded);
        if (t->load_success) //load success!
          return child_tid;
        else
          return -1;
      }
    }
  }
  return -1;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{

}

bool remove (const char *file)
{
  
}

int open (const char *file)
{

}

int filesize (int fd)
{

}

int read (int fd, void *buffer, unsigned size)
{
  
}

int write (int fd, const void *buffer, unsigned size)
{
  int ret = -1;

  if (fd == 1)
  {
    putbuf(buffer, size);
    ret = size;
  }
  return ret;
}

void seek (int fd, unsigned position)
{

}
unsigned tell (int fd)
{

}

void close (int fd)
{

}
