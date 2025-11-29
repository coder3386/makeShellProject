#include "shell.h"

int main()
{
    char buf[256];
    char *argv[50];
    int narg;
    pid_t pid;

    setup_signals();

    while (1) {
        printf("shell> ");
        fflush(stdout);
        
        if(fgets(buf, sizeof(buf), stdin) == NULL) {
            // 진짜 파일의 끝(Ctrl-D)이면 종료
            if (feof(stdin)) {
                printf("\n");
                exit(0);
            }
            // 시그널(Ctrl-C) 때문에 끊긴 거면 버퍼 비우고 다시 입력 대기
            clearerr(stdin);
            continue;
        }
        
        buf[strcspn(buf, "\n")] = 0;
        
        if (strlen(buf) == 0) continue;

        narg = getargs(buf, argv);
        if (narg == 0) continue;

        // --------------------[4번 파트] 파이프 처리

        int has_pipe = 0;
        for (int i = 0; i < narg; i++) {
            if (strcmp(argv[i], "|") == 0) {
                has_pipe = 1;
                break;
            }
        }

        if (has_pipe) {
            handle_pipe(argv, narg);
            continue;
        }

        //--------- [5번 파트] 내장 명령어 및 재지향 처리
       
        if (is_builtin(argv)) {
            // cd 명령어는 fork() 없이 부모 프로세스에서 직접 처리 (재지향 지원)
            if (strcmp(argv[0], "cd") == 0) {
                int saved_stdout = -1;
                int saved_stdin = -1;
                int redirect_error = 0;

                // 재지향 처리 (부모 프로세스)
                for (int i = 0; i < narg; i++) {
                    if (argv[i] == NULL) continue;

                    // 출력 재지향 (>)
                    if (strcmp(argv[i], ">") == 0) {
                        if (argv[i+1] == NULL) {
                            fprintf(stderr, "cd: syntax error expected file after >\n");
                            redirect_error = 1; break;
                        }
                        saved_stdout = dup(STDOUT_FILENO);
                        int fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                        if (fd < 0) {
                            perror("cd open failed");
                            redirect_error = 1; break;
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                        argv[i] = NULL; argv[i+1] = NULL;
                    }
                    // 입력 재지향 (<)
                    else if (strcmp(argv[i], "<") == 0) {
                        if (argv[i+1] == NULL) {
                            fprintf(stderr, "cd: syntax error expected file after <\n");
                            redirect_error = 1; break;
                        }
                        saved_stdin = dup(STDIN_FILENO);
                        int fd = open(argv[i+1], O_RDONLY);
                        if (fd < 0) {
                            perror("cd open failed");
                            redirect_error = 1; break;
                        }
                        dup2(fd, STDIN_FILENO);
                        close(fd);
                        argv[i] = NULL; argv[i+1] = NULL;
                    }
                }

                if (!redirect_error) {
                    execute_builtin(argv);
                }

                // 표준 입출력 원상 복구
                if (saved_stdout != -1) {
                    dup2(saved_stdout, STDOUT_FILENO);
                    close(saved_stdout);
                }
                if (saved_stdin != -1) {
                    dup2(saved_stdin, STDIN_FILENO);
                    close(saved_stdin);
                }
                continue;
            }

            // cd가 아닌 다른 내장 명령어는 fork 후 실행
            int has_redirection = 0;
            for (int i = 0; i < narg; i++) {
                if (strcmp(argv[i], ">") == 0 || strcmp(argv[i], "<") == 0) {
                    has_redirection = 1; break;
                }
            }
            
            if (!has_redirection) {
                execute_builtin(argv);
                continue;
            }
            
            pid = fork();
            if (pid == 0) {
                reset_signals();

		handle_redirection(argv, &narg); // 4번 재지향 처리
                execute_builtin(argv);
                exit(0);
            } else if (pid > 0) {
                wait(NULL);
            } else {
                perror("fork failed");
            }
            continue;
        }

 
        // --------일반 외부 명령어 실행
  
        pid = fork();

        if (pid == 0) {
            reset_signals();
	    
	    handle_redirection(argv, &narg); // 4번 재지향 처리
    
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            
            wait(NULL);
        } else {
            perror("fork failed");
        }
    }
}

// 인자 파싱 함수
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

// 내장 명령어 확인 
int is_builtin(char **argv)
{
if (argv[0] == NULL) return 0;

    if (strcmp(argv[0], "cd") == 0 ||
        strcmp(argv[0], "pwd") == 0 ||
        strcmp(argv[0], "ls") == 0 ||
        strcmp(argv[0], "cat") == 0 ||
	strcmp(argv[0], "ln") == 0 ||
        strcmp(argv[0], "cp") == 0 ||
        strcmp(argv[0], "grep") == 0) {
        return 1;
    }
    return 0;
}

// 내장 명령어 실행 (jms)
void execute_builtin(char **argv)
{
    if (strcmp(argv[0], "cd") == 0) builtin_cd(argv);
    else if (strcmp(argv[0], "pwd") == 0) builtin_pwd(argv);
    else if (strcmp(argv[0], "ls") == 0) builtin_ls(argv);
    else if (strcmp(argv[0], "cat") == 0) builtin_cat(argv);
    else if (strcmp(argv[0], "ln") == 0) builtin_ln(argv);
    else if (strcmp(argv[0], "cp") == 0) builtin_cp(argv);
    else if (strcmp(argv[0], "grep") == 0) builtin_grep(argv);
}
