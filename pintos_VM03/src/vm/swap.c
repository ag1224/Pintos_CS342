#include "swap.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include <bitmap.h>
#include <stdbool.h>
#include <round.h>

static struct bitmap *swap_table;            /* Bitmap of free swap slots. */
static struct disk* swap_disk;
static struct lock swap_lock;

void
swap_table_init(void)
{
	swap_disk = disk_get(1,1);   /* disk corresponding to swap */
	int count = disk_size(swap_disk) * SECT_TO_SLOT;  // number of swap slots
	size_t bm_pages = DIV_ROUND_UP (bitmap_buf_size (count), PGSIZE);

	void *base = palloc_get_multiple(bm_pages);
	swap_table = bitmap_create_in_buf(count, base, bm_pages*PGSIZE );
	lock_init(&swap_lock);
}

void
swap_into_memory (void *kpage, int swap_slot_num)
{
	int i, start_slot = swap_slot_num * SLOT_TO_SECT;
	lock_acquire(&swap_lock);
	for(i=0; i< SLOT_TO_SECT; i++)
	{
		disk_read (swap_disk, start_slot + i, kpage + i*(DISK_SECTOR_SIZE));	
	}
	bitmap_reset (swap_table, swap_slot_num); 
	lock_release(&swap_lock);
}

int
swap_into_disk (void *kpage)
{
	lock_acquire(&swap_lock);
	int swap_slot_num = bitmap_scan_and_flip (swap_table, 0, 1, false);
	int i, start_slot = swap_slot_num * SLOT_TO_SECT;

	for(i=0; i< SLOT_TO_SECT; i++)
	{
		disk_write (swap_disk, start_slot + i, kpage + i*(DISK_SECTOR_SIZE));	
	}
	lock_release(&swap_lock);

	return swap_slot_num;
}

void
swap_free (int swap_slot_num)
{
	lock_acquire(&swap_lock);
	bitmap_reset (swap_table, swap_slot_num); 
	lock_release(&swap_lock);
	return;
}
