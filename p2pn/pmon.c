/* This is a simple process monitor,
   which automatically restart a dead process
 */

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


void
usage()
{
    printf("pmon -c [COMMAND]\n");
}

int
start_mon(char **argv, int arglen)
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
        /* child pid */
            ret = execv(argv[0], argv);
            if (ret < 0) {
                perror("execv");
                continue;
            }
            exit(0);
        }

        if (pid > 0) {
            pid = wait(&status);
            fprintf(stdout,
            "!!!!####################\n"
            "####wait returned, restart sub process\n"
            "!!!!####################\n");
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    int opt;
    char *command, *p;
    char *subopt[128] = {0};
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
    while (p != NULL)
    {
        subopt[soix++] = strdup(p);
        p = strtok(NULL, " ");
    }

    start_mon(subopt, soix);
    return 0;
}
