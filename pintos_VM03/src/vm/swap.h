#include "threads/synch.h"

#define SECT_TO_SLOT DISK_SECTOR_SIZE / PGSIZE
#define SLOT_TO_SECT PGSIZE / DISK_SECTOR_SIZE

void swap_table_init(void);
void swap_into_memory (void *kpage, int swap_slot_num);
int swap_into_disk (void *kpage);