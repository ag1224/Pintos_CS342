
#ifndef THREADS_ALARM_H
#define THREADS_ALARM_H

#include "devices/timer.h"
#include <list.h>
#include "threads/interrupt.h"

/* an alarm */
struct alarm
  {
    int64_t ticks;
    struct thread *thrd; /* the waiting thread */
    
    struct list_elem elem; /* list element */
    
    unsigned magic; /* alarm magic */
  };
  
void alarm_init (void); /* init the alarm list */

void thread_block_till(int64_t); /* set alarm for current thread */

void thread_set_check_wakeup (void); /* check all the alarms in the list, and wake'em up if time's up */

#endif /* THREADS_ALARM_H */
