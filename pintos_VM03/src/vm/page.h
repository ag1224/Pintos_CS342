#include "swap.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "threads/vaddr.h"
#include "threads/synch.h"

#define SECT_BITS 0xfffff
#define SPT_FLAG_BITS 30
#define WRIT_BIT 29
#define MMAP_BIT 27
#define IN_MEM_BIT 28

#define SPT_MMAPPED(x) (((x) >> MMAP_BIT) & 1) 
#define SPT_FLAG(x) ((x) >> SPT_FLAG_BITS)
#define SPT_WRITABLE(x) (((x) >> WRIT_BIT) & 1)
#define SPT_IN_MEM(x) (((x) >> IN_MEM_BIT) & 1)

#define STK_LIM_SIZE (8 * 1024 * 1024)		/* stack size limit in bytes */
#define STK_LIM_ADDR (void *)(PHYS_BASE - STK_LIM_SIZE)



enum spd_flags
  {
  	PAG_INV = 000,
    PAG_ZERO = 001,           /* All zero page */
  	PAG_SWAP = 002,			  /* Page on filesys */
    PAG_FILE = 003           /* Page on filesys. */
  };

uint32_t * page_supp_create (void);
void page_supp_destroy (uint32_t *spd);
bool page_supp_set_addr (uint32_t *spd, void *vaddr, int aux, 
                		enum spd_flags flags, int file_offt, bool writable, bool mmap);
bool page_supp_set (uint32_t *spd, void *upage, int aux, 
					enum spd_flags flags, int file_offt, bool writable, bool mmap);
bool page_supp_chkmap (uint32_t *spd, const void *uaddr); 
void page_supp_print (uint32_t *spd, const void *uaddr); 
bool page_to_memory (uint32_t *spd, const void *uaddr);
void page_supp_clear_page (uint32_t *spd, void *upage);
bool page_to_disk (struct thread *t, void *upage, void* kpage);
