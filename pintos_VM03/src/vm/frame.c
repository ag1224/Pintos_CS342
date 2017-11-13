#include "frame.h"
#include "swap.h"
#include <debug.h>
#include <stdbool.h>
#include "page.h"

struct frame_tabl_elem {
	struct list_elem elem;
	struct thread *t;
	void *kpage;
	void *upage;
	bool second_chance;
};

static struct list frame_tabl;
static struct lock frame_lock;
static void *second_chance();

void 
frame_tabl_init (void) 
{
	list_init(&frame_tabl);
	lock_init(&frame_lock);
	return;
}

bool
set_frame (struct thread* t, void *upage, void *kpage)
{
	lock_acquire(&frame_lock);
	struct frame_tabl_elem *fte = malloc(sizeof(struct frame_tabl_elem));

	fte -> t = t;
	fte -> kpage = kpage;
	fte -> upage = upage;
	list_push_front(&frame_tabl, &fte -> elem);
	lock_release(&frame_lock);
	return true;
}

void *
frame_get_page (enum palloc_flags flags)
{	
	void *kpage = palloc_get_page(flags);
	if (kpage == NULL) {
		//PANIC ("palloc_get: out of pages");
		lock_acquire(&frame_lock);

		struct frame_tabl_elem* evicted = second_chance();
		kpage = evicted -> kpage;
		if(!page_to_disk(evicted -> t, evicted->upage, evicted->kpage))
			PANIC("SWAP SLOT ERROR");
		pagedir_clear_page(evicted -> t -> pagedir, evicted->upage); 
		free(evicted);

		lock_release(&frame_lock);
	}	

	return kpage;
}


static void *
second_chance()
{
	struct list_elem *e =NULL, *e1=NULL;
	for(e = list_begin (&frame_tabl); e != list_end (&frame_tabl);
       e = e1)
	{
		struct frame_tabl_elem *frame_elem = list_entry (e, struct frame_tabl_elem, elem);
		if(! pagedir_is_accessed(frame_elem -> t->pagedir, frame_elem->upage))
		{
			e1 = list_remove(e);
			return frame_elem;

		}
		else
		{
			pagedir_set_accessed(frame_elem -> t->pagedir, frame_elem->upage, false);
			e1 = list_remove(e);
			list_push_back(&frame_tabl, e);
		}
	}

}

void
frame_free_all(struct thread* t)
{
	lock_acquire(&frame_lock);

	struct list_elem *e =NULL, *e1=NULL;

	for (e = list_begin (&frame_tabl); e != list_end (&frame_tabl);
       e = e1)
	{
		
		struct frame_tabl_elem *frame_elem = list_entry (e, struct frame_tabl_elem, elem);

		if( frame_elem -> t == t)
		{
			e1 = list_remove(e);
			free(frame_elem);
		}
		else
			e1 = e->next;
	}

	lock_release(&frame_lock);
}