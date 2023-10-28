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
    exec((const char *)arg[0]);
    break;
  case SYS_WAIT:
    get_arg(f->esp, arg, 1);
    wait((pid_t)arg[0]);
    break;
  case SYS_CREATE:
    get_arg(f->esp, arg, 2);
    create((const char *)arg[0], (unsigned int)arg[1]);
    break;
  case SYS_REMOVE:
    get_arg(f->esp, arg, 1);
    remove((const char *)arg[0]);
    break;
  case SYS_OPEN:
    get_arg(f->esp, arg, 1);
    open((const char *)arg[0]);
    break;
  case SYS_FILESIZE:
    get_arg(f->esp, arg, 1);
    filesize((int)arg[0]);
    break;
  case SYS_READ:
    get_arg(f->esp, arg, 3);
    read((int)arg[0], (void *)arg[1], (unsigned int)arg[2]);
    break;
  case SYS_WRITE:
    get_arg(f->esp, arg, 3);
    write((int)arg[0], (const void *)arg[1], (unsigned int)arg[2]);
    break;
  case SYS_SEEK:
    get_arg(f->esp, arg, 2);
    seek((int)arg[0], (unsigned int)arg[1]);
    break;
  case SYS_TELL:
    get_arg(f->esp, arg, 1);
    tell((int)arg[0]);
    break;
  case SYS_CLOSE:
    get_arg(f->esp, arg, 1);
    close((int)arg[0]);
    break;
 } 
}

void get_arg (void *esp, int *arg, int count)
{
  for (int i = 0; i < count; i++)
  {
    arg[i] = *(uint32_t *)(esp + 4 * (i + 1));
  }
}

void check_validation (void *p)
{ 
  struct thread *t = thread_current();
  bool success = false;

  if (p != NULL && pagedir_get_page(t->pagedir, p) != NULL && !is_kernel_vaddr(p))
    success = true;

  if (!success)
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
  check_validation((void *)cmd_line);

}

int wait (pid_t pid)
{

}

bool create (const char *file, unsigned initial_size)
{
  check_validation((void *)file);
}

bool remove (const char *file)
{
  check_validation((void *)file);
}

int open (const char *file)
{
  check_validation((void *)file);
}

int filesize (int fd)
{

}

int read (int fd, void *buffer, unsigned size)
{
  check_validation(buffer);
}

int write (int fd, const void *buffer, unsigned size)
{
  check_validation((void *)buffer);

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
