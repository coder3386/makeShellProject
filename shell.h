#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>  

// 함수 선언
int getargs(char *cmd, char **argv);

// 3_6_signal.c에서 구현
void setup_signals();
void reset_signals();

// 4_redirection.c에서 구현
void handle_redirection(char **argv, int *narg);
void handle_pipe(char **argv, int narg);

// 5_builtin_C.c에서 구현 
void builtin_cd(char **argv);
void builtin_pwd(char **argv);
void builtin_ls(char **argv);
void builtin_cat(char **argv);
void builtin_ln(char **argv);
void builtin_cp(char **argv);
void builtin_grep(char **argv);

// 내장 명령어 확인 및 실행
int is_builtin(char **argv);
void execute_builtin(char **argv);

#endif
