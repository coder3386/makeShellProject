#include "shell.h"

// 파일 재지향 처리
void handle_redirection(char **argv, int *narg)
{
    int i;
    int new_narg = *narg;
    
    for (i = 0; i < *narg; i++) {
        if (argv[i] == NULL) continue;
        
        // 출력 재지향 '>'
        if (strcmp(argv[i], ">") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "syntax error: expected filename after '>'\n");
                exit(1);
            }
            int fd = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                fprintf(stderr, "open %s: %s\n", argv[i+1], strerror(errno));
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            
            argv[i] = NULL;
            argv[i+1] = NULL;
            if (i < new_narg) new_narg = i;
        }
        // 입력 재지향 '<'
        else if (strcmp(argv[i], "<") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(stderr, "syntax error: expected filename after '<'\n");
                exit(1);
            }
            int fd = open(argv[i+1], O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "open %s: %s\n", argv[i+1], strerror(errno));
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
            
            argv[i] = NULL;
            argv[i+1] = NULL;
            if (i < new_narg) new_narg = i;
        }
    }
    
    *narg = new_narg;
}

// 단일 명령어 실행 (파이프 내부용)
void execute_command(char **cmd, int cmd_narg)
{
    // 재지향 처리
    handle_redirection(cmd, &cmd_narg);
    
    // 내장 명령어 확인
    if (is_builtin(cmd)) {
        execute_builtin(cmd);
        exit(0);
    }
    
    // 외부 명령어 실행
    execvp(cmd[0], cmd);
    perror("execvp failed");
    exit(1);
}

// 파이프 처리 - 다중 파이프 지원
void handle_pipe(char **argv, int narg)
{
    int pipe_positions[50];
    int pipe_count = 0;
    int i, j;
    
    // 모든 파이프 위치 찾기
    for (i = 0; i < narg; i++) {
        if (strcmp(argv[i], "|") == 0) {
            pipe_positions[pipe_count++] = i;
        }
    }
    
    if (pipe_count == 0) return;
    
    // 파이프 개수 + 1 = 명령어 개수
    int num_commands = pipe_count + 1;
    int pipefds[pipe_count][2];
    pid_t pids[num_commands];
    
    // 모든 파이프 생성
    for (i = 0; i < pipe_count; i++) {
        if (pipe(pipefds[i]) < 0) {
            perror("pipe failed");
            return;
        }
    }
    
    // 각 명령어 실행
    int start = 0;
    for (i = 0; i < num_commands; i++) {
        // 명령어 범위 결정
        int end = (i < pipe_count) ? pipe_positions[i] : narg;
        
        // 명령어 분리
        char *cmd[50];
        int cmd_idx = 0;
        for (j = start; j < end; j++) {
            cmd[cmd_idx++] = argv[j];
        }
        cmd[cmd_idx] = NULL;
        
        // fork 및 실행
        pids[i] = fork();
        if (pids[i] == 0) {
            // 자식 프로세스
            
            // 입력 재지향 (첫 번째 명령어가 아니면)
            if (i > 0) {
                dup2(pipefds[i-1][0], STDIN_FILENO);
            }
            
            // 출력 재지향 (마지막 명령어가 아니면)
            if (i < num_commands - 1) {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }
            
            // 모든 파이프 닫기
            for (j = 0; j < pipe_count; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            
            // 명령어 실행
            execute_command(cmd, cmd_idx);
        }
        
        start = end + 1;  // 다음 명령어 시작 위치
    }
    
    // 부모 프로세스: 모든 파이프 닫기
    for (i = 0; i < pipe_count; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    
    // 모든 자식 프로세스 대기
    for (i = 0; i < num_commands; i++) {
        waitpid(pids[i], NULL, 0);
    }
}