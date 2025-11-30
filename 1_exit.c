#include "shell.h"
#include <stdlib.h>
#include <stdio.h>

// exit 명령어: optional numeric argument를 허용
void builtin_exit(char **argv)
{
    int status = 0;
    if (argv[1] != NULL) {
        char *endptr = NULL;
        long v = strtol(argv[1], &endptr, 10);
        if (endptr != argv[1] && *endptr == '\0') {
            status = (int)v;
        } else {
            fprintf(stderr, "exit: numeric argument required\n");
            status = 1;
        }
    }
    exit(status);
}
