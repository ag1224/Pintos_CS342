#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"

struct t_status {
  tid_t tid;                 /* Child thread id */
  struct thread *t;          /* Child thread pointer */
  int status;                /* Exit status */
  struct condition cond;     /* Condition for signaling parent */
  struct lock l;             /* Lock for managing access to struct */
  struct list_elem elem;     /* List placement in parent */
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
bool t_stat_create(struct thread *);

#endif /* userprog/process.h */
