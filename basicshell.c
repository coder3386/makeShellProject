#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main()
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    while (1) {
        printf("shell>");
        //gets(buf);
        if(scanf("%s", buf)==0) continue;
        //if (buf == "\0") continue;

        clearerr(stdin);
        narg = getargs(buf, argv);

        if (narg == 0) continue;

        pid = fork();

        if (pid == 0) {
            // 자식 프로세스
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            // 부모 프로세스
            wait((int *)0);
        } else {
            // fork 실패
            perror("fork failed");
        }
    }
}

int getargs(char *cmd, char **argv)
{
    int narg = 0;
    while (*cmd) {
        if (*cmd == ' ' || *cmd == '\t') {
            *cmd++ = '\0';
        } else {
            argv[narg++] = cmd++;
            while (*cmd != '\0' && *cmd != ' ' && *cmd != '\t') {
                cmd++;
            }
        }
    }
    argv[narg] = NULL;
    return narg;
}

