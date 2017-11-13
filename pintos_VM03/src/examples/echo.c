#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
  int i;

  for (i = 0; i < argc; i++)
    printf ("%s ", argv[i]);
  printf ("\n");

  int ex = wait(exec("echoe"));
	
  printf("This is after echoe, which exit with status %d\n", ex);
  int ex2 = wait(exec("echoe"));
  printf("This is after the second echoe, exit stat %d\n", ex2);
  return EXIT_SUCCESS;
}
