/* This is a simple process monitor which automatically
 * re-runs a given command if it dies.
 */

/* needed for getopt() and strdup() */
#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_CMDLINE_PART 128

void usage()
{
  printf("pmon -c [COMMAND]\n");
}

int start_mon(char *argv[])
{
  pid_t pid;
  int ret, status;

  for (;;) {
    pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork() error %d, %s\n",
              errno, strerror(errno));
      break;
    }

    if (pid == 0) {
      /* child process, run the 'real' process */
      ret = execv(argv[0], argv);
      if (ret < 0) {
        perror("execv");
        exit(0);
      }
    }

    if (pid > 0) { 
      pid = wait(&status);
      printf("pmon: child %d returned %d, restarting.\n", pid, status);
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int opt;
  char *command, *p;
  char *subopt[MAX_CMDLINE_PART] = { NULL };
  int soix;

  command = NULL;
  while ((opt = getopt(argc, argv, "c:")) != -1) {
    switch (opt) {
      case 'c':
        command = optarg;
        break;

      default:
        usage();
        exit(1);
    }
  }

  if (command == NULL) {
    usage();
    exit(1);
  }

  soix = 0;
  p = strtok(command, " ");
  while (p != NULL && soix < (MAX_CMDLINE_PART-1)) {
    subopt[soix] = strdup(p);
    p = strtok(NULL, " ");
    soix++;
  }
  subopt[soix] = NULL;
  start_mon(subopt);

  return 0;
}
