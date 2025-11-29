#include "shell.h"

// 시그널 핸들러: Ctrl-C 등을 눌렀을 때 실행되는 함수
void sig_handler(int signo)
{
    if (signo == SIGINT) {
        // Ctrl-C 입력 시: 줄바꿈만 하고 쉘은 죽지 않음
        printf("\n"); 
    }
    else if (signo == SIGQUIT) {
        // Ctrl-Z (또는 Ctrl-\) 입력 시
        printf("\n");
    }
}

// 쉘(부모 프로세스)에서 호출: 시그널을 가로채도록 설정
void setup_signals()
{
    signal(SIGINT, sig_handler);
    signal(SIGQUIT, sig_handler);
    // 리눅스 실제 Ctrl-Z는 SIGTSTP입니다. 필요하면 아래 주석 해제
    signal(SIGTSTP, sig_handler);
}

// 자식 프로세스에서 호출: 시그널 처리를 기본값(OS 기본)으로 되돌림
// 이걸 안 하면 자식 프로세스도 Ctrl-C에 안 죽는 문제가 발생함
void reset_signals()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
}
