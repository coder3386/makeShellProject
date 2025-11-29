#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

static void sigchld_handler(int sig) {
    int saved_errno = errno;
    /* Reap all exited children without blocking */
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
    errno = saved_errno;
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    /* reap background children to avoid zombies */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf("msj> ");
        fflush(stdout);

        nread = getline(&line, &len, stdin);
        if (nread == -1) { /* EOF (Ctrl-D) */
            printf("\n");
            break;
        }

        /* strip trailing newline */
        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        /* skip leading whitespace */
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0') continue; /* empty line */

        /* check for trailing '&' (background) */
        int background = 0;
        /* find end of meaningful text */
        char *end = p + strlen(p) - 1;
        while (end >= p && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }
        if (end >= p && *end == '&') {
            background = 1;
            *end = '\0';
            /* trim any spaces before where & was */
            while (end >= p && isspace((unsigned char)*end)) { *end = '\0'; end--; }
        }

        /* built-in: exit */
        if (strcmp(p, "exit") == 0) break;

        /* tokenize (simple split on spaces/tabs) */
        char *argv[100];
        int argc = 0;
        char *tok = strtok(p, " \t");
        while (tok != NULL && argc < (int)(sizeof(argv)/sizeof(argv[0]) - 1)) {
            argv[argc++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        if (argc == 0) continue;

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            /* child */
            execvp(argv[0], argv);
            /* if execvp returns, there was an error */
            perror("exec");
            _exit(127);
        } else {
            /* parent */
            if (background) {
                printf("[bg] pid %d started\n", pid);
                /* do not wait for background process */
            } else {
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid");
                }
            }
        }
    }

    free(line);
    return 0;
}
