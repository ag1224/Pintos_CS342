#include "threads/synch.h"
#include "threads/vaddr.h"
#include "threads/thread.h"
#include <list.h>
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "threads/synch.h"

void frame_tabl_init (void) ;
bool set_frame (struct thread* t, void *upage, void *kpage);
void *frame_get_page (enum palloc_flags flags);

void frame_free_all(struct thread* t);
