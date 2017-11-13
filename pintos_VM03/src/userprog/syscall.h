#include "lib/user/syscall.h"
#include "threads/synch.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock;

void syscall_init (void);
void munmap_kernel (mapid_t mid);

#endif /* userprog/syscall.h */
