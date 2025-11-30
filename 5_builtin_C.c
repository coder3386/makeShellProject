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

// mkdir 명령어
void builtin_mkdir(char **argv)
{
    if (argv[1] == NULL) {
        fprintf(stderr, "usage: mkdir <dir>\n");
        return;
    }
    int i = 1;
    while (argv[i] != NULL) {
        if (mkdir(argv[i], 0755) != 0) {
            fprintf(stderr, "mkdir: cannot create directory '%s': %s\n", argv[i], strerror(errno));
        }
        i++;
    }
}

// rmdir 명령어 (비어있는 디렉터리만 제거)
void builtin_rmdir(char **argv)
{
    if (argv[1] == NULL) {
        fprintf(stderr, "usage: rmdir <dir>\n");
        return;
    }
    int i = 1;
    while (argv[i] != NULL) {
        if (rmdir(argv[i]) != 0) {
            fprintf(stderr, "rmdir: failed to remove '%s': %s\n", argv[i], strerror(errno));
        }
        i++;
    }
}

// 내부 헬퍼: 디렉터리/파일을 재귀적으로 삭제
static int remove_recursive(const char *path)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;
        struct dirent *entry;
        int ret = 0;
        while ((entry = readdir(d)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char child[4096];
            snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
            if (remove_recursive(child) != 0) ret = -1;
        }
        closedir(d);
        if (rmdir(path) != 0) ret = -1;
        return ret;
    } else {
        // 파일 또는 심볼릭 링크
        if (unlink(path) != 0) return -1;
        return 0;
    }
}

// rm 명령어: -r 옵션 지원
void builtin_rm(char **argv)
{
    if (argv[1] == NULL) {
        fprintf(stderr, "usage: rm [-r] <path>...\n");
        return;
    }

    int i = 1;
    int recursive = 0;
    if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "-R") == 0) {
        recursive = 1;
        i = 2;
    }
    if (argv[i] == NULL) {
        fprintf(stderr, "rm: missing operand\n");
        return;
    }

    for (; argv[i] != NULL; i++) {
        if (recursive) {
            if (remove_recursive(argv[i]) != 0) {
                fprintf(stderr, "rm: failed to remove '%s': %s\n", argv[i], strerror(errno));
            }
        } else {
            if (unlink(argv[i]) != 0) {
                // unlink fails for directories; give a helpful message
                fprintf(stderr, "rm: cannot remove '%s': %s\n", argv[i], strerror(errno));
            }
        }
    }
}

// mv 명령어: 기본적으로 rename 사용, 다른 파일시스템일 경우 복사 후 삭제
void builtin_mv(char **argv)
{
    if (argv[1] == NULL || argv[2] == NULL) {
        fprintf(stderr, "usage: mv <source> <dest>\n");
        return;
    }

    // 단순 rename 시도
    if (rename(argv[1], argv[2]) == 0) return;

    // rename 실패하면(예: EXDEV) 파일 복사 후 삭제 (단순 파일만 처리)
    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) {
        fprintf(stderr, "mv: cannot open '%s': %s\n", argv[1], strerror(errno));
        return;
    }
    int dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd < 0) {
        fprintf(stderr, "mv: cannot create '%s': %s\n", argv[2], strerror(errno));
        close(src_fd);
        return;
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, n) != n) {
            fprintf(stderr, "mv: write error: %s\n", strerror(errno));
            break;
        }
    }

    close(src_fd);
    close(dst_fd);

    // 원본 삭제
    if (unlink(argv[1]) != 0) {
        fprintf(stderr, "mv: cannot remove '%s' after copy: %s\n", argv[1], strerror(errno));
    }
}

