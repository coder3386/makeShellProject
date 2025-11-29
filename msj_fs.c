#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

static int recursive_remove(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        perror("lstat");
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) { perror("opendir"); return -1; }
        struct dirent *ent;
        int ret = 0;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            size_t len = strlen(path) + 1 + strlen(ent->d_name) + 1;
            char *child = malloc(len);
            if (!child) { ret = -1; break; }
            snprintf(child, len, "%s/%s", path, ent->d_name);
            if (recursive_remove(child) != 0) ret = -1;
            free(child);
        }
        closedir(d);
        if (ret == 0) {
            if (rmdir(path) != 0) { perror("rmdir"); ret = -1; }
        }
        return ret;
    } else {
        if (unlink(path) != 0) { perror("unlink"); return -1; }
        return 0;
    }
}

static int do_mkdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "mkdir: missing operand\n");
        return -1;
    }
    int ok = 0;
    for (int i = 1; i < argc; ++i) {
        if (mkdir(argv[i], 0755) != 0) {
            perror("mkdir");
        } else ok++;
    }
    return ok == (argc-1) ? 0 : -1;
}

static int do_rmdir(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: missing operand\n");
        return -1;
    }
    int ok = 0;
    for (int i = 1; i < argc; ++i) {
        if (rmdir(argv[i]) != 0) {
            perror("rmdir");
        } else ok++;
    }
    return ok == (argc-1) ? 0 : -1;
}

static int do_rm(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "rm: missing operand\n");
        return -1;
    }
    int recursive = 0;
    int idx = 1;
    if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "-R") == 0) {
        recursive = 1; idx = 2;
        if (argc < 3) { fprintf(stderr, "rm: missing operand after '%s'\n", argv[1]); return -1; }
    }
    int ok = 0;
    for (int i = idx; i < argc; ++i) {
        struct stat st;
        if (lstat(argv[i], &st) != 0) { perror("lstat"); continue; }
        if (S_ISDIR(st.st_mode)) {
            if (!recursive) {
                fprintf(stderr, "rm: %s: is a directory\n", argv[i]);
                continue;
            }
            if (recursive_remove(argv[i]) != 0) continue;
            ok++;
        } else {
            if (unlink(argv[i]) != 0) { perror("unlink"); continue; }
            ok++;
        }
    }
    return ok > 0 ? 0 : -1;
}

static int do_mv(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "mv: usage: mv SOURCE DEST\n");
        return -1;
    }
    if (rename(argv[1], argv[2]) == 0) return 0;
    /* fallback for cross-filesystem simple files: try copy + unlink */
    if (errno == EXDEV) {
        /* try to copy file */
        FILE *src = fopen(argv[1], "rb");
        if (!src) { perror("fopen source"); return -1; }
        FILE *dst = fopen(argv[2], "wb");
        if (!dst) { perror("fopen dest"); fclose(src); return -1; }
        char buf[8192];
        size_t r;
        while ((r = fread(buf, 1, sizeof(buf), src)) > 0) {
            if (fwrite(buf, 1, r, dst) != r) { perror("fwrite"); fclose(src); fclose(dst); return -1; }
        }
        fclose(src); fclose(dst);
        if (unlink(argv[1]) != 0) { perror("unlink"); return -1; }
        return 0;
    }
    perror("mv");
    return -1;
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;

    while (1) {
        printf("msjfs> "); fflush(stdout);
        nread = getline(&line, &len, stdin);
        if (nread == -1) { printf("\n"); break; }
        if (nread > 0 && line[nread-1] == '\n') line[nread-1] = '\0';
        /* trim leading whitespace */
        char *p = line;
        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (*p == '\0') continue;
        /* parse tokens */
        char *argv[128]; int argc = 0;
        char *tok = strtok(p, " \t");
        while (tok && argc < (int)(sizeof(argv)/sizeof(argv[0]) - 1)) {
            argv[argc++] = tok; tok = strtok(NULL, " \t");
        }
        argv[argc] = NULL;
        if (argc == 0) continue;
        if (strcmp(argv[0], "exit") == 0) break;
        if (strcmp(argv[0], "mkdir") == 0) { do_mkdir(argc, argv); continue; }
        if (strcmp(argv[0], "rmdir") == 0) { do_rmdir(argc, argv); continue; }
        if (strcmp(argv[0], "rm") == 0) { do_rm(argc, argv); continue; }
        if (strcmp(argv[0], "mv") == 0) { do_mv(argc, argv); continue; }

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); continue; }
        if (pid == 0) {
            execvp(argv[0], argv);
            perror("exec"); _exit(127);
        } else {
            int status; waitpid(pid, &status, 0);
        }
    }
    free(line);
    return 0;
}
