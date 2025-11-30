#include "shell.h"
#include <string.h>

// 명령어 마지막 인자가 "&"이면 백그라운드 실행으로 처리하고
// argv에서 해당 토큰을 제거한 뒤 narg를 업데이트합니다.
// 반환값: 1이면 백그라운드, 0이면 포그라운드
int check_and_strip_background(char **argv, int *narg)
{
    if (argv == NULL || narg == NULL) return 0;
    if (*narg == 0) return 0;

    int last = *narg - 1;
    if (argv[last] != NULL && strcmp(argv[last], "&") == 0) {
        argv[last] = NULL;
        *narg = last;
        return 1;
    }
    return 0;
}
