#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/off_t.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/input.h"
#include "vm/page.h"

//for the list of system call handlers
typedef void (*call_handler) (void **,struct intr_frame *);
static call_handler syscall_list[30];
static int syscall_no_args[30];


static void syscall_handler (struct intr_frame *);

void exit(int status);

//system call handlers
void sys_halt_handler(void **args, struct intr_frame *f);
void sys_exit_handler(void **args, struct intr_frame *f);
void sys_exec_handler(void **args, struct intr_frame *f);
void sys_wait_handler(void **args, struct intr_frame *f);
void sys_create_handler(void **args, struct intr_frame *f);
void sys_remove_handler(void **args, struct intr_frame *f);
void sys_open_handler(void **args, struct intr_frame *f);
void sys_filesize_handler(void **args, struct intr_frame *f);
void sys_read_handler(void **args, struct intr_frame *f);
void sys_write_handler(void **args, struct intr_frame *f);
void sys_seek_handler(void **args, struct intr_frame *f);
void sys_tell_handler(void **args, struct intr_frame *f);
void sys_close_handler(void **args, struct intr_frame *f);
void sys_exec_handler(void **args, struct intr_frame *f);
void sys_wait_handler(void **args, struct intr_frame *f);
void sys_mmap_handler(void **args, struct intr_frame *f);
void sys_munmap_handler(void **args, struct intr_frame *f);

void get_arguments(void **,void *,int);

bool check_buf(char*, unsigned);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  
  syscall_list[SYS_HALT] = &sys_halt_handler;
  syscall_list[SYS_EXIT] = &sys_exit_handler;
  syscall_list[SYS_EXEC] = &sys_exec_handler;
  syscall_list[SYS_WAIT] = &sys_wait_handler;
  syscall_list[SYS_CREATE] = &sys_create_handler;
  syscall_list[SYS_REMOVE] = &sys_remove_handler;
  syscall_list[SYS_OPEN] = &sys_open_handler;
  syscall_list[SYS_FILESIZE] = &sys_filesize_handler;
  syscall_list[SYS_READ] = &sys_read_handler;
  syscall_list[SYS_WRITE] = &sys_write_handler;
  syscall_list[SYS_SEEK] = &sys_seek_handler;
  syscall_list[SYS_TELL] = &sys_tell_handler;
  syscall_list[SYS_CLOSE] = &sys_close_handler;
  syscall_list[SYS_MMAP] = &sys_mmap_handler;
  syscall_list[SYS_MUNMAP] = &sys_munmap_handler;

  syscall_no_args[SYS_HALT] = 0;
  syscall_no_args[SYS_EXIT] = 1;
  syscall_no_args[SYS_EXEC] = 1;
  syscall_no_args[SYS_WAIT] = 1;
  syscall_no_args[SYS_CREATE] = 2;
  syscall_no_args[SYS_REMOVE] = 1;
  syscall_no_args[SYS_OPEN] = 1;
  syscall_no_args[SYS_FILESIZE] = 1;
  syscall_no_args[SYS_READ] = 3;
  syscall_no_args[SYS_WRITE] = 3;
  syscall_no_args[SYS_SEEK] = 2;
  syscall_no_args[SYS_TELL] = 1;
  syscall_no_args[SYS_CLOSE] = 1;
  syscall_no_args[SYS_MMAP] = 2;
  syscall_no_args[SYS_MUNMAP] = 1;


  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f ) 
{
  
  void *stack_ptr = f -> esp;
  int syscall_no,no_args;
  void (*function)(void **,struct intr_frame *); 
  void * args[3];
  struct thread *t;
  
  t = thread_current();

  if(!is_user_vaddr(stack_ptr)) // checking if the pointer given by user is below PHY_BASE
      exit(-1);

  t -> stack = (uint32_t *) stack_ptr;

  if(!pagedir_get_page(t->pagedir,stack_ptr)) // checking if given memory has been mapped
       exit(-1);
    
  syscall_no = *(int*)(stack_ptr);

  function = syscall_list[syscall_no];
  no_args = syscall_no_args[syscall_no];
  
  int m;
  
  for(m = 1; m <= no_args; m++)
  {
      //printf("ARG %x\n",stack_ptr+m*sizeof );
      if(!is_user_vaddr(stack_ptr + (m * sizeof(void *))))
          exit(-1);
  }


  for(m = 1; m <= no_args; m++)
  {
      if(!pagedir_get_page(t->pagedir,f->esp+(m*sizeof(void *)) ))
           exit(-1);
      
  }

  get_arguments(args, stack_ptr, no_args);
  (*function) ( args, f);


}

/* function to get the arguements pushed on user stack into args*/
void
get_arguments(void **args,void *stack_ptr, int no_args)
{
  int i;
  for(i=0;i<no_args;i++)
     args[i] = stack_ptr+(i+1)*sizeof(void*);
}


/* handles the system call halt */
void sys_halt_handler(void **args, struct intr_frame *f)
{
  power_off();
}

/* handles the system call exit */
void sys_exit_handler (void **args, struct intr_frame *f)
{
    int status = *((int*)args[0]);
    exit(status);
}

/* closes the open files and acquired locks and exits the user thread */
void exit(int status)
{
    struct thread *t;

    t = thread_current();
    t -> exit_code = status;
    //handle locks and open files

    printf("%s: exit(%d)\n",t->name,status);

    // printf ("Execution of '%s' complete.\n", t->name);
    //power_off();
    // printf("After power_off");
    thread_exit();

}

/* function to check if buffer is valid */
bool
check_buf(char* buffer, unsigned length){

  bool valid = false;

  struct thread *t = thread_current();
  if(buffer != NULL && is_user_vaddr(buffer + length) && is_user_vaddr(buffer))
    valid = true;
    
    
  else 
    exit(-1);

  return valid;
}

 // hex_dump(f->esp,f->esp,512,true);
void sys_write_handler(void **args, struct intr_frame *f)
{
    //printf("Inside write handler\n");
    int fd = *((int*)(args[0]));
    char *buffer = *((char**)(args[1]));
    unsigned length = *(unsigned *)(args[2]), len_written = -1;
    
    struct thread* t = thread_current();
    //printf("%d %p %d %p %p %p\n", fd, buffer, length, f->esp, pagedir_get_page(t->pagedir, 122299), pagedir_get_page(t->pagedir,(f->esp)) );
    if(check_buf(buffer, length))
    {

      if(fd == 1)
      {
      putbuf(buffer,length);
      //printf("%c %c %c",buffer[0],buffer[1],buffer[2]); 
      len_written = length;
      }

      else if(fd>=2 && fd < 128)
      {
      	lock_acquire(&filesys_lock);
        struct file* file = t->fd_table[fd];
        
        if( file != NULL)
          len_written = file_write (file, buffer, length);

      	lock_release(&filesys_lock);
      }

    }

    
    f->eax = len_written;
   // printf("Are you here?\n");
}


void sys_read_handler(void **args, struct intr_frame *f)
{
   // printf("Inside read handler\n");
    int fd = *((int*)(args[0]));
    char *buffer = *((char**)(args[1]));
    unsigned length = *(unsigned *)(args[2]), len_written = -1, i;
    //printf("%d %d %d\n", fd, buffer, length);
    struct thread* t = thread_current();

    if(check_buf(buffer, length))
    {
      if(fd == 0)
      {
      for(i=0; i<length; i++)
        buffer[i] = (char) input_getc();

      len_written = length;
      }

      else if(fd>=2 && fd < 128)
      {
      	lock_acquire(&filesys_lock);
        struct file* file = t->fd_table[fd];
        
        if( file != NULL)
          len_written = file_read (file, buffer, length);

      	lock_release(&filesys_lock);
      }

    }
    
    //printf("Read successful, %d bytes read\n", len_written);
    f->eax = len_written;
}




void sys_create_handler(void **args, struct intr_frame *f)
{
	char *name = *((char **)(args[0]));
	off_t initial_size = *((int*) args[1]);
  	struct thread* t= thread_current();

  	if(check_buf(name,0) && strlen(name) >0 )
  	{
  		lock_acquire(&filesys_lock);
    	f->eax = filesys_create(name, initial_size);
  		lock_release(&filesys_lock);
  	}

  	else
    	f->eax =false;

}

void sys_remove_handler(void **args, struct intr_frame *f)
{
	char * name = *((char **) args[0]);

  if(name !=NULL && strlen(name)>0)
  {
  	lock_acquire(&filesys_lock);
  	f->eax = filesys_remove(name);
  	lock_release(&filesys_lock);
  }
    
  else
    f->eax = false;
// check what exactly happens on trying to close an open file, i.e, if anything needs to be done here

}



void sys_open_handler(void **args, struct intr_frame *f)
{
	char * name= *((char **) args[0]);
	int fd = -1;
	struct thread *t  = thread_current();

	struct file * file = NULL;

	if( check_buf(name,0) && strlen(name) >0)
	{
		lock_acquire(&filesys_lock);
		file = filesys_open(name);
		lock_release(&filesys_lock);
	}
	// struct inode * inode = file_get_inode(file);

  if(file != NULL)
  {
    //printf("%u ",file->pos);

    //if((file->inode)->removed)
      //file_close(file);

      fd = get_fd(t);

      if(fd == -1)
        file_close(file);
      
      else
        t->fd_table[fd]=file;
    
  }

	f->eax=fd;

}



void sys_close_handler(void **args, struct intr_frame *f)
{
	struct thread *t;
	int fd = *((int *) args[0]);

	t = thread_current();
  if(fd >=2 && fd<=127)
  {
  	struct file* file = t->fd_table[fd];

    if(file!=NULL)
    {
    t->fd_table[fd] = NULL;
    lock_acquire(&filesys_lock);
    file_close(file);
    lock_release(&filesys_lock);
    }
  
  }
	
}


void sys_filesize_handler(void **args, struct intr_frame *f)
{
	struct thread *t;
	int fd = *((int *) args[0]);

	t = thread_current();

	struct file* file = t->fd_table[fd];

	lock_acquire(&filesys_lock);
	f->eax = file_length(file);
	lock_release(&filesys_lock);

}


void sys_seek_handler(void **args, struct intr_frame *f)
{
	struct thread *t;
	int fd = *((int*) args[0]);
	off_t pos = *((unsigned *) args[1]);

	t = thread_current();
	struct file* file = t->fd_table[fd];
	
	lock_acquire(&filesys_lock);
	file_seek(file, pos);
	lock_release(&filesys_lock);
}

void sys_tell_handler(void **args, struct intr_frame *f)
{
	struct thread *t;
	int fd = *((int*) args[0]);

	t = thread_current();
	struct file* file = t->fd_table[fd];

	lock_acquire(&filesys_lock);
	f->eax= file_tell(file);
	lock_release(&filesys_lock);
}

void
sys_exec_handler(void **args, struct intr_frame *f) {

    char *cmdline = *((char **)args[0]);
    tid_t child_tid;
    struct thread *t = thread_current();

    if(!is_user_vaddr(cmdline))
        exit(-1);

    if(!pagedir_get_page(t -> pagedir, cmdline))
        exit(-1);

    /* invalid cmdline */
    if ((cmdline == NULL) || (!strlen(cmdline))) {
        f -> eax = -1;
        return;
    }

    /* process_execute() queues the thread and 
        returns a value based on the success of thread_create()
        and load() operations */
    child_tid = process_execute(cmdline);
    if (child_tid == TID_ERROR)
        f -> eax = -1;
    else 
        f -> eax = child_tid;
    return;
}


void
sys_wait_handler(void **args, struct intr_frame *f) {

    tid_t child_tid = *((int *)args[0]);
    if (child_tid < 0) {
      f -> eax = -1;
      return;
    }
    int status = process_wait(child_tid);

    f -> eax = status;
    
    return;
}

void
sys_mmap_handler(void **args, struct intr_frame *f)
{
  int fd = *((int *)args[0]), j;
  void* vaddr = *((char**)args[1]);
  void* i;
  struct thread* t = thread_current();

  struct file* file = t -> fd_table[fd];
  if (file != NULL && is_user_vaddr(vaddr) && pg_ofs (vaddr) == 0 && vaddr != 0)
  {
    lock_acquire(&filesys_lock);
    int length = file_length(file);
    lock_release(&filesys_lock);

    if (length == 0 || (vaddr + length >= STK_LIM_ADDR))
    {
      f -> eax = -1;
      return;
    }

    for (i = vaddr; i <vaddr + length; i += PGSIZE)
      if (page_supp_chkmap(t -> spd, i))
      {
        f -> eax = -1;
        return;
      } 

    mapid_t mid;

    /* Get the first free mmap id */
    for (j = 2; j < NO_FILE_MAX; j++)
      if ((t->mmap_table[j]).start_vaddr == NULL)
        {
          mid = j;
          break;
        }

    /* Set the mmap table entries */
    (t->mmap_table[mid]).mfile = file_reopen(file);
    (t->mmap_table[mid]).start_vaddr = vaddr;

    /* Set the supp page table entries */
    for (i = vaddr; i < vaddr + length; i += PGSIZE)
    {
      page_supp_set (t -> spd, i, mid, PAG_FILE, i - vaddr, true, true);
    }

    f -> eax = mid;
    return;
  }

  f -> eax = -1;
  return;
}

void
sys_munmap_handler(void **args, struct intr_frame *f)
{
  mapid_t mid = *((int *)args[0]);
  struct thread *t = thread_current();

  if (mid > 128 || mid < 0) {
    printf("In unmap, with addr %p\n", (void *)mid);
    page_supp_print(t -> spd, mid);
    return;
  }
  
  if ((t -> mmap_table[mid]).start_vaddr != NULL)
  {
    munmap_kernel(mid);
  }

  return;
}

void munmap_kernel (mapid_t mid)
{
  struct thread *t = thread_current();
  void *vaddr = (t -> mmap_table[mid]).start_vaddr;
  void *lpa;
  struct file *mf = (t -> mmap_table[mid]).mfile;
  int flen = file_length(mf);

  /* Write the dirty pages back to filesystem */
  for (lpa = vaddr; lpa < vaddr + flen; lpa += PGSIZE)
  {
    void *kpage = pagedir_get_page(t -> pagedir, lpa);
    if (kpage != NULL && pagedir_is_dirty(t -> pagedir, lpa))
    {
      int offt = lpa - vaddr;
      int write_size = (flen - offt) > PGSIZE ? PGSIZE : (flen - offt);
      
      lock_acquire(&filesys_lock);

      file_seek(mf, offt);
      file_write(mf, lpa, write_size);

      lock_release(&filesys_lock);
    }
  }

  /* Clear the spd and pagedir entries(including mem) */
  for (lpa = vaddr; lpa < vaddr + flen; lpa += PGSIZE)
  {
    page_supp_clear_page(t -> spd, lpa);
    if (pagedir_get_page(t -> pagedir, lpa))
      pagedir_clear_page(t -> pagedir, lpa);
  } 

  /* Close extra instance of file for map */
  lock_acquire(&filesys_lock);
  file_close(mf);
  lock_release(&filesys_lock);

  /* Unset entry in mmap table */
  (t -> mmap_table[mid]).mfile = NULL;
  (t -> mmap_table[mid]).start_vaddr = NULL;

  return;
}
