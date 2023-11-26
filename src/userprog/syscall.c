#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "vm/page.h"
#include "vm/frame.h"

//synchronization of file system
struct lock file_sys;

static void syscall_handler (struct intr_frame *);
void get_arg (void *esp, int *arg, int count);
void check_validation (void *p);
void check_buffer_validation (void *esp, void *buffer, unsigned size);
void check_str_validation (void *str, unsigned size);
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
mapid_t mmap (int fd, void *addr);
void munmap (mapid_t mapid);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_sys); // init lock of file system
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 
  check_validation(f->esp);
  thread_current()->esp = f->esp;
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
    // check_validation((void *)arg[1]);
    check_buffer_validation(f->esp, (void *)arg[1], (unsigned int)arg[2]);
    f->eax = read((int)arg[0], (void *)arg[1], (unsigned int)arg[2]);
    break;
  case SYS_WRITE:
    get_arg(f->esp, arg, 3);
    // check_validation((void *)arg[1]);
    check_str_validation((void *)arg[1], (unsigned int)arg[2]);
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
  case SYS_MMAP:
    get_arg(f->esp, arg, 2);
    //check validation for what? buffer? string? or normal?
    f->eax = mmap ((int)arg[0], (void *)arg[1]);
    break;
  case SYS_MUNMAP:
    get_arg(f->esp, arg, 1);
    munmap ((mapid_t) arg[0]);
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

void check_buffer_validation (void *esp, void *buffer, unsigned size)
{ 
  if (buffer == NULL || is_kernel_vaddr(buffer))
    exit(-1);
  
  void* p = pg_round_down(buffer);

  for (p; p < buffer + size; p = pg_round_up((void *)(uintptr_t)p + 1))
  { 
    struct spt_entry *spte = spt_search_page(p);

    if (spte != NULL && !spte->is_loaded)
    { //I think I can delete this..? no need..?
      struct ft_entry *fte = frame_table_get_frame(p, PAL_USER);
      bool load = false;
      switch (spte->loc)
      {  
         case BIN:
            load = spt_load_data_to_page(spte, fte->kpage);
            spte->loc = ON_FRAME;
            break;
         case FILE:
            load = spt_load_data_to_page(spte, fte->kpage);
            break;
         case SWAP:
            swap_in(spte->swap_idx, fte->kpage);
            spte->loc = ON_FRAME;
            load = spte->is_loaded;
            break;
      }
      if (!load)
        exit(-1);
    }
    else if(spte == NULL && stack_access(esp, p))
    { 
      if(!grow_stack(p))
      { 
        exit(-1);
      }
      spte = spt_search_page(p);
    }
    else if (!spte->writable)
    { 
      exit(-1);
    }
    else if (spte == NULL)
    { 
      exit(-1);
    }
    ASSERT (spte != NULL);

    frame_table_pin(spte->kpage);
  }
}

void check_str_validation (void *str, unsigned size)
{ 
  if (str == NULL || is_kernel_vaddr(str))
    exit(-1);
  
  void* p = pg_round_down(str);

  for (p; p < str + size; p = pg_round_up((void *)(uintptr_t)p + 1))
  { 
    struct spt_entry *spte = spt_search_page(p);
    if(spte == NULL)
    { 
      exit(-1);
    }

    if (!spte->is_loaded)
    {
      struct ft_entry *fte = frame_table_get_frame(p, PAL_USER);
      bool load = spt_load_data_to_page(spte, fte->kpage);
      if (!load)
        exit(-1);
    }
    frame_table_pin(spte->kpage);
  }
}

void halt (void)
{
  shutdown_power_off();
}

void exit (int status)
{ 
  thread_current()->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  for (int fd = 2; fd < FDIDX_LIMIT; fd++)
  { 
    if (thread_current()->fdt[fd] != NULL)
      close(fd);
  }
  thread_exit();
}

pid_t exec (const char *cmd_line)
{ 
  tid_t child_tid = process_execute(cmd_line);
  
  struct thread *t = thread_get_child(child_tid);

  if (t != NULL) // exist child thread matching child_tid
  {
    sema_down(&t->loaded);
    if (t->load_success) //load success!
      return child_tid;
    else //load fail. parent should wait(회수) for child exited with -1.
      return wait(child_tid);
  }

  return -1;
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{ 
  lock_acquire(&file_sys);
  bool ret = filesys_create(file, initial_size);
  lock_release(&file_sys);

  return ret;
}

bool remove (const char *file)
{ 
  lock_acquire(&file_sys);
  bool ret = filesys_remove(file);
  lock_release(&file_sys);

  return ret;
}

int open (const char *file)
{ 
  lock_acquire(&file_sys);
  struct file *f = filesys_open(file);
  if (f == NULL)
  { 
    lock_release(&file_sys);
    return -1;
  }
  
  int fd = process_store_new_file(f);

  if (fd == -1)
    file_close(f);
  
  lock_release(&file_sys);
  return fd;
}

int filesize (int fd)
{ 
  struct file *f = process_get_file(fd);

  if (f == NULL)
    return -1;

  lock_acquire(&file_sys);
  int ret = file_length(f);
  lock_release(&file_sys);

  return ret;
}

int read (int fd, void *buffer, unsigned size)
{ 
  if (fd == FD_STDOUT)
    return -1;
  
  int read_cnt = 0;
  char *buf_arg = buffer;

  if (fd == FD_STDIN)
  { 
    while (read_cnt < size)
    {
      buf_arg[read_cnt] = input_getc();
      if (buf_arg[read_cnt] == '\0')
        break;
      read_cnt++;
    }
  }
  else
  {
    struct file *f = process_get_file(fd);
    if (f == NULL)
      return -1;
    lock_acquire(&file_sys);
    read_cnt = file_read(f, buffer, size);
    lock_release(&file_sys);
  }
  frame_table_unpin_all_frames(buffer, size);
  return read_cnt;
}

int write (int fd, const void *buffer, unsigned size)
{
  int written_size;
  
  if (fd == FD_STDIN)
  {
    return -1;
  }
  else if (fd == FD_STDOUT)
  {
    putbuf(buffer, size);
    written_size = size;
  }
  else
  {
    struct file *f = process_get_file(fd);
    if (f == NULL)
      return -1;
    
    lock_acquire(&file_sys);
    written_size = file_write(f, buffer, size);  
    lock_release(&file_sys);
  }
  frame_table_unpin_all_frames(buffer, size);
  return written_size;
}

void seek (int fd, unsigned position)
{ 
  struct file *f = process_get_file(fd);
  if (f == NULL)
    return;

  lock_acquire(&file_sys);
  file_seek(f, position);
  lock_release(&file_sys);
}
unsigned tell (int fd)
{
  struct file *f = process_get_file(fd);
  if (f == NULL)
    exit (-1);
  
  lock_acquire(&file_sys);
  int ret = file_tell(f);
  lock_release(&file_sys);

  return ret;
}

void close (int fd)
{ 
  lock_acquire(&file_sys);
  process_close_file(fd);
  lock_release(&file_sys);
}

mapid_t mmap (int fd, void *addr)
{
  if (!is_user_vaddr(addr))
    return MAP_FAILED;
  if ( addr == 0x0 || addr == NULL || pg_ofs(addr) != 0)
    return MAP_FAILED;

  if (fd == FD_STDIN || fd == FD_STDOUT)
    return MAP_FAILED;
  
  struct file *f = process_get_file(fd);

  if (f == NULL)
    return MAP_FAILED;

  lock_acquire(&file_sys);
  struct file *reopened = file_reopen(f);

  if (reopened == NULL)
    { 
      lock_release(&file_sys);
      return MAP_FAILED;
    }

  int32_t file_len = file_length(reopened);

  if ((int)file_len <= 0)
  { 
    lock_release(&file_sys);
    return MAP_FAILED;
  }
  lock_release(&file_sys);

  uint8_t *address = (uint8_t *)addr;
  off_t offset = 0;
  for(offset; offset < file_len; offset += PGSIZE)
  { 
      if(spt_search_page(address + offset))
    {
      return MAP_FAILED;
    }
    if(pagedir_get_page(thread_current()->pagedir, address + offset))
    {
      return MAP_FAILED;
    }
  }
    
  return mmapt_mapping_insert(reopened, fd, address);
}

void munmap (mapid_t mapid)
{
  mmapt_mapid_delete (mapid);
}
