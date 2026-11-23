#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGTERM);

    // 자식이 sigwait()를 호출하기 전에 도착한 신호도 대기 상태로 둔다.
    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        int received_signal;
        int error = sigwait(&signals, &received_signal);
        if (error != 0) {
            fprintf(stderr, "sigwait: %s\n", strerror(error));
            return 1;
        }

        printf("자식: SIGTERM 수신 (신호 번호 %d)\n", received_signal);
        return 0;
    }

    printf("부모: 자식 PID %ld에 SIGTERM 전송\n", (long)child);
    fflush(stdout);

    if (kill(child, SIGTERM) == -1) {
        perror("kill");
        return 1;
    }

    int status;
    if (waitpid(child, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "부모: 자식이 비정상 종료했습니다.\n");
        return 1;
    }

    int exit_code = WEXITSTATUS(status);
    printf("부모: 자식의 종료 코드 %d 확인\n", exit_code);
    return exit_code;
}
