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