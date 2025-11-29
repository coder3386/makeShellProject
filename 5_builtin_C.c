#include "shell.h"

// cd 명령어 (~) 확장 지원
void builtin_cd(char **argv)
{
    char path[1024];
    char *target_path;
    
    if (argv[1] == NULL) {
        // 인자가 없으면 홈 디렉토리로
        char *home = getenv("HOME");
        if (home != NULL) {
            chdir(home);
        }
    } else {
        // ~ 확장 처리
        if (argv[1][0] == '~') {
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
                return;
            }
            
            if (argv[1][1] == '\0') {
                // "~" 만 있는 경우
                target_path = home;
            } else if (argv[1][1] == '/') {
                // "~/..." 형태인 경우
                snprintf(path, sizeof(path), "%s%s", home, argv[1] + 1);
                target_path = path;
            } else {
                // "~username" 형태는 지원X
                target_path = argv[1];
            }
        } else {
            target_path = argv[1];
        }
        
        if (chdir(target_path) != 0) {
            perror("cd");
        }
    }
}

// pwd 명령어
void builtin_pwd(char **argv)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

// ls 명령어
void builtin_ls(char **argv)
{
    DIR *dir;
    struct dirent *entry;
    char *path = (argv[1] == NULL) ? "." : argv[1];
    
    dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // 숨김 파일 제외
        if (entry->d_name[0] == '.') continue;
        printf("%s\n", entry->d_name);
    }
    closedir(dir);
}

// cat 명령어
void builtin_cat(char **argv)
{
    char buffer[4096];
    size_t bytes;
    
    // 인자가 없으면 표준 입력에서 읽기
    if (argv[1] == NULL) {
        while ((bytes = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
            fwrite(buffer, 1, bytes, stdout);
        }
        return;
    }
    
    // 파일 이름이 주어진 경우
    FILE *fp;
    int i = 1;
    
    while (argv[i] != NULL) {
        fp = fopen(argv[i], "r");
        if (fp == NULL) {
            fprintf(stderr, "cat: %s: No such file or directory\n", argv[i]);
            i++;
            continue;
        }
        
        while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            fwrite(buffer, 1, bytes, stdout);
        }
        
        fclose(fp);
        i++;
    }
}


// [추가] ln 명령어 (하드 링크 및 심볼릭 링크 -s 지원)
void builtin_ln(char **argv)
{
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "usage: ln [-s] <target> <link_name>\n");
        return;
    }

    // 심볼릭 링크 처리 (ln -s target linkname)
    if (strcmp(argv[1], "-s") == 0) {
        if (argv[3] == NULL) {
            fprintf(stderr, "usage: ln -s <target> <link_name>\n");
            return;
        }
        if (symlink(argv[2], argv[3]) != 0) {
            perror("ln -s");
        }
    }
    // 하드 링크 처리 (ln target linkname)
    else {
        if (link(argv[1], argv[2]) != 0) {
            perror("ln");
        }
    }
}

// [추가] cp 명령어 (파일 복사)
void builtin_cp(char **argv)
{
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "usage: cp <source> <dest>\n");
        return;
    }

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        perror("cp: source open error");
        return;
    }

    // 대상 파일 열기 (쓰기전용 | 없으면생성 | 있으면내용삭제, 권한 0644)
    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        perror("cp: dest open error");
        close(src_fd);
        return;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, n) != n) {
            perror("cp: write error");
            break;
        }
    }

    close(src_fd);
    close(dst_fd);
}

// [추가] grep 명령어 (단순 문자열 매칭)
void builtin_grep(char **argv)
{
    if (argv[1] == NULL) {
        fprintf(stderr, "usage: grep <pattern> [file]\n");
        return;
    }

    char *pattern = argv[1];
    FILE *fp;
    char buffer[4096];

    // 인자가 2개뿐이면(grep pattern) -> 표준 입력(파이프)에서 읽음
    if (argv[2] == NULL) {
        fp = stdin;
    }
    // 인자가 3개면(grep pattern filename) -> 파일에서 읽음
    else {
        fp = fopen(argv[2], "r");
        if (fp == NULL) {
            perror("grep");
            return;
        }
    }

    // 한 줄씩 읽어서 패턴 검색
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        // strstr: 문자열 안에 문자열이 있는지 찾음 (NULL이 아니면 찾은 것)
        if (strstr(buffer, pattern) != NULL) {
            printf("%s", buffer);
        }
    }

    if (fp != stdin) {
        fclose(fp);
    }
}

