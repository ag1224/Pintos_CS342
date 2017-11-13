#include "vm/page.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "vm/swap.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "lib/user/syscall.h"

struct sup_pt_entry
{ 
  uint32_t o_pte;     /* Contains the swap slot number (swap based) / page-zero bytes(file-based) */
  uint32_t file_offt;   /* byte offset for upage. */  
};

static uint32_t
set_in_mem_bit (uint32_t o_pte, bool bit) {
  uint32_t aux = (1 << IN_MEM_BIT) ^ (bit ? 0: -1);
  if (bit == false)
    return o_pte & aux;
  else 
    return o_pte | aux;
}

static uint32_t 
spte_create_user(int aux, enum spd_flags flags, bool writable, bool mmap)  // IN_MEM bit is zero
{
  if ( flags == PAG_SWAP || flags == PAG_FILE )
    return ((flags << SPT_FLAG_BITS) | (writable << (WRIT_BIT)) | (mmap << MMAP_BIT) | (aux & SECT_BITS)) ;
  else
    return (flags << SPT_FLAG_BITS) | (writable << (WRIT_BIT));
}


/* Creates a new page directory that has mappings for kernel
   virtual addresses, but none for user virtual addresses.
   Returns the new page directory, or a null pointer if memory
   allocation fails. */
uint32_t *
page_supp_create (void) 
{
  uint32_t *pd = palloc_get_page (0);
  if (pd != NULL)
    memset (pd, 0, PGSIZE);
  return pd;
}

/* Destroys page directory PD, freeing all the pages it
   references. */
void
page_supp_destroy (uint32_t *spd) 
{
  uint32_t *spde;

  if (spd == NULL)
    return;

  for (spde = spd; spde < spd + pd_no (PHYS_BASE); spde++)
    if (*spde != 0) 
      {
        uint32_t *spt = pde_get_pt (*spde);
        struct sup_pt_entry *spte;
        
        for (spte = spt; spte < spt + 2 * PGSIZE / sizeof *spte; spte++)
          if (SPT_FLAG(spte -> o_pte) == PAG_SWAP && !SPT_IN_MEM(spte-> o_pte) )
            swap_free(spte -> o_pte & SECT_BITS);
        palloc_free_multiple (spt, sizeof(struct sup_pt_entry) / sizeof(uint32_t));
      }
  palloc_free_page (spd);
}

/* Returns the address of the page table entry for virtual
   address VADDR in page directory PD.
   If PD does not have a page table for VADDR, behavior depends
   on CREATE.  If CREATE is true, then a new page table is
   created and a pointer into it is returned.  Otherwise, a null
   pointer is returned. */
static struct sup_pt_entry *
lookup_page (uint32_t *pd, const void *vaddr, bool create)
{
  uint32_t *pt, *pde;

  ASSERT (pd != NULL);

  /* Shouldn't create new kernel virtual mappings. */
  ASSERT (!create || is_user_vaddr (vaddr));

  /* Check for a page table for VADDR.
     If one is missing, create one if requested. */
  pde = pd + pd_no (vaddr);
  //test code

  if (*pde == 0) 
    {
      if (create)
        {
          pt = palloc_get_multiple (PAL_ZERO, (sizeof(struct sup_pt_entry) / sizeof(uint32_t)));
          if (pt == NULL) 
            return NULL; 
      
          *pde = pde_create (pt);
        }
      else
        return NULL;
    }

  /* Return the page table entry. */
  pt = pde_get_pt (*pde);
  return pt + (sizeof(struct sup_pt_entry) / sizeof(uint32_t))* pt_no (vaddr);
}

/* Adds a mapping in page directory PD from user virtual page
   UPAGE to the physical frame identified by kernel virtual
   address KPAGE.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   If WRITABLE is true, the new page is read/write;
   otherwise it is read-only.
   Returns true if successful, false if memory allocation
   failed. */

bool
page_supp_set_addr (uint32_t *spd, void *vaddr, int aux, 
                enum spd_flags flags, int file_offt, bool writable, bool mmap)
{
	return page_supp_set (spd, pg_round_down(vaddr), aux, 
                			flags, file_offt, writable, mmap);
}


bool
page_supp_set (uint32_t *spd, void *upage, int aux, 
                enum spd_flags flags, int file_offt, bool writable, bool mmap)
{
  struct sup_pt_entry *spte;

  ASSERT (pg_ofs (upage) == 0);  
  ASSERT (is_user_vaddr (upage));


  spte = lookup_page (spd, upage, true);

  if (spte != NULL) 
    {
      ASSERT (SPT_FLAG(spte -> o_pte) == PAG_INV);
      // ASSERT (!SPT_IN_MEM(spte -> o_pte))
      spte -> o_pte = spte_create_user (aux, flags, writable, mmap);

      if (flags == PAG_FILE)
        spte -> file_offt = file_offt;

      // if (upage == (void *)0x805c000)
      //     printf("PS upage: %p, o_pte: %x, offt:%d\n", upage, spte -> o_pte, spte -> file_offt);

      return true;
    }
  else
    return false;
}

/* When a frame is being written back to disk, if it is not a file_system page, the corresponding kpage is written 
to an empty swap slot and the entry in suplemental page table is updated */
bool 
page_to_disk (struct thread *t, void *upage, void* kpage)
{
  uint32_t *spd = t -> spd;
  struct sup_pt_entry *spte;

  ASSERT (pg_ofs (upage) == 0);  
  ASSERT (is_user_vaddr (upage));

  ASSERT (pagedir_get_page(t -> pagedir, upage) != NULL);

  spte = lookup_page (spd, upage, false);

  if (spte != NULL) 
    {
      ASSERT (SPT_FLAG(spte -> o_pte) != PAG_INV);
      if (SPT_FLAG(spte -> o_pte) != PAG_FILE)
      {
        enum spd_flags flag = PAG_SWAP;
        bool writable = SPT_WRITABLE(spte -> o_pte);
        int swap_slot = swap_into_disk(kpage);

        spte-> o_pte = spte_create_user(swap_slot, flag, writable, 0); // IN_MEM bit is 0
      }
      else if (pagedir_is_dirty(t -> pagedir, upage))
      {
        if (SPT_MMAPPED(spte -> o_pte))
        {
          mapid_t mid = spte -> o_pte & SECT_BITS;
          struct file *mf = (t -> mmap_table[mid]).mfile;

          lock_acquire(&filesys_lock);
          
          int write_size, flen = file_length(mf);
          write_size = (flen - spte -> file_offt) > PGSIZE ? PGSIZE : (flen - spte -> file_offt);
          file_seek(mf, spte -> file_offt);
          int len_written = file_write(mf, upage, write_size);
          
          lock_release(&filesys_lock);
          
          if (len_written != PGSIZE)
            return false;
        }
        else
        {
          /* Case where automatic variables were set as PAG_FILE by mistake while loading, need to correct */
          enum spd_flags flag = PAG_SWAP;
          bool writable = SPT_WRITABLE(spte -> o_pte);
          int swap_slot = swap_into_disk(kpage);

          spte-> o_pte = spte_create_user(swap_slot, flag, writable, 0); // IN_MEM bit is 0
          spte -> file_offt = 0;

        }
          
      }

      spte -> o_pte = set_in_mem_bit (spte -> o_pte, false);
      //if (upage == (void *)0x805c000)
      // printf("PSd upage: %p, o_pte: %x, offt:%d\n", upage, spte -> o_pte, spte -> file_offt);

      return true;

    }
  return false;
}


/* Looks up the physical address that corresponds to user virtual
   address UADDR in PD.  Returns the kernel virtual address
   corresponding to that physical address, or a null pointer if
   UADDR is unmapped. */
bool
page_supp_chkmap (uint32_t *spd, const void *uaddr) 
{
  struct sup_pt_entry *spte;

  ASSERT (is_user_vaddr (uaddr));
  
  spte = lookup_page (spd, uaddr, false);
  
  if (spte != NULL && SPT_FLAG(spte -> o_pte) != PAG_INV)
    return true;
  else
    return false;
}

void
page_supp_print (uint32_t *spd, const void *uaddr) 
{
  struct sup_pt_entry *spte;

  ASSERT (is_user_vaddr (uaddr));
  printf("In page_supp_print\n");
  spte = lookup_page (spd, uaddr, false);
  if (spte != NULL)
    printf("o_pte: %x, file_offt: %x\n", spte -> o_pte, spte -> file_offt);
  else
    printf("Invalid SPT entry for VA %x\n", uaddr);
}

bool
page_to_memory (uint32_t *spd, const void *uaddr)
{
  struct sup_pt_entry *spte; 
  ASSERT (page_supp_chkmap(spd, uaddr));


  spte = lookup_page (spd, uaddr, false);
  ASSERT (SPT_IN_MEM(spte -> o_pte) == 0);

  void *upage = pg_round_down(uaddr);
  void *kpage = frame_get_page(PAL_USER);
  int flag = SPT_FLAG(spte -> o_pte);
  struct thread* t= thread_current();
  bool writable = SPT_WRITABLE(spte -> o_pte);

  if (flag == PAG_ZERO) {
    memset(kpage, 0, PGSIZE);
  }

  else if (flag == PAG_SWAP) {
    int swap_slot = spte -> o_pte & SECT_BITS; 
    swap_into_memory(kpage, swap_slot);
  }

  else if (flag == PAG_FILE) {
    struct file *ef;
    int page_read_bytes;
    if (SPT_MMAPPED(spte -> o_pte)) 
    {
      mapid_t mid  = spte -> o_pte & SECT_BITS;
      ef = (t -> mmap_table[mid]).mfile;
      int len = file_length(ef);
      page_read_bytes = (len - spte -> file_offt) > (PGSIZE) ? PGSIZE : len - spte -> file_offt;
    }
    else
    {
      ef = thread_current() -> exec_file;
      page_read_bytes = PGSIZE - (spte -> o_pte & SECT_BITS);
    }

    lock_acquire(&filesys_lock);

    file_seek(ef, 0);    
    file_read_at(ef, kpage, page_read_bytes, spte -> file_offt);
    
    lock_release(&filesys_lock);

    memset (kpage + page_read_bytes, 0, PGSIZE - page_read_bytes);

  }

  spte -> o_pte = set_in_mem_bit(spte->o_pte, 1);  // setting the in_MEM bit
  // if (upage == (void *)0x805c000)
  // printf("PSm upage: %p, o_pte: %x, offt:%d\n", upage, spte -> o_pte, spte -> file_offt);

  pagedir_set_page(thread_current() -> pagedir, upage, kpage, writable);
  set_frame (thread_current(), upage, kpage);

  return true;
}



/* Marks user virtual page UPAGE "not present" in page
   directory PD.  Later accesses to the page will fault.  Other
   bits in the page table entry are preserved.
   UPAGE need not be mapped. */
void
page_supp_clear_page (uint32_t *spd, void *upage) 
{
  struct sup_pt_entry *spte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));

  spte = lookup_page (spd, upage, false);
  if (spte != NULL && SPT_FLAG(spte -> o_pte) != PAG_INV)
    {
      spte -> o_pte = 0;
    }
}

