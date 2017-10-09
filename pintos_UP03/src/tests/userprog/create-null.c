/* Tries to create a file with the null pointer as its name.
   The process must be terminated with exit code -1. */

#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
	// exit(-1);
  msg ("create(NULL): %d", create (NULL, 0));
}
