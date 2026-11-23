#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// 메시지를 파이프로 전송한다.
static int send_message(FILE *out, const char *name)
{
    char *line = NULL;
    size_t capacity = 0;

    printf("%s 입력: ", name);
    fflush(stdout);

    if (getline(&line, &capacity, stdin) == -1) {
        free(line);
        fputs("/quit\n", out);
        fflush(out);
        return 0;
    }

    line[strcspn(line, "\r\n")] = '\0';
    int quit = strcmp(line, "/quit") == 0;

    int failed = fprintf(out, "%s\n", line) < 0;

    if (fflush(out) == EOF) {
        failed = 1;
    }

    free(line);

    if (failed) {
        fprintf(stderr, "메시지 전송 실패\n");
        return 0;
    }

    return !quit;
}

// 상대 프로세스가 보낸 메시지를 읽는다.
static int receive_message(FILE *in, const char *peer)
{
    char *line = NULL;
    size_t capacity = 0;

    if (getline(&line, &capacity, in) == -1) {
        printf("상대 연결이 종료되었습니다.\n");
        free(line);
        return 0;
    }

    line[strcspn(line, "\r\n")] = '\0';

    if (strcmp(line, "/quit") == 0) {
        printf("%s가 채팅을 종료했습니다.\n", peer);
        free(line);
        return 0;
    }

    printf("%s에게 받은 메시지: %s\n\n", peer, line);
    free(line);
    return 1;
}

static void chat(int read_fd, int write_fd,
                 const char *name, const char *peer,
                 int starts_first)
{
    FILE *in = fdopen(read_fd, "r");
    FILE *out = fdopen(write_fd, "w");

    if (in == NULL || out == NULL) {
        perror("fdopen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (!starts_first && !receive_message(in, peer)) {
            break;
        }

        if (!send_message(out, name)) {
            break;
        }

        if (starts_first && !receive_message(in, peer)) {
            break;
        }
    }

    fclose(in);
    fclose(out);
}

int main(void)
{
    int a_to_b[2];
    int b_to_a[2];

    // 두 프로세스가 터미널 입력을 미리 읽어두지 않도록 설정한다.
    setvbuf(stdin, NULL, _IONBF, 0);
    signal(SIGPIPE, SIG_IGN);

    if (pipe(a_to_b) == -1) {
        perror("pipe");
        return 1;
    }

    if (pipe(b_to_a) == -1) {
        perror("pipe");
        close(a_to_b[0]);
        close(a_to_b[1]);
        return 1;
    }

    printf("=== 파이프 채팅 ===\n");
    printf("A와 B가 번갈아 입력합니다.\n");
    printf("/quit를 입력하면 종료합니다.\n\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        close(a_to_b[0]);
        close(a_to_b[1]);
        close(b_to_a[0]);
        close(b_to_a[1]);
        return 1;
    }

    if (pid == 0) {
        // 자식 B: A의 메시지를 읽고 A에게 답장한다.
        close(a_to_b[1]);
        close(b_to_a[0]);

        chat(a_to_b[0], b_to_a[1], "B", "A", 0);
        return 0;
    }

    // 부모 A: B에게 메시지를 보내고 답장을 읽는다.
    close(a_to_b[0]);
    close(b_to_a[1]);

    chat(b_to_a[0], a_to_b[1], "A", "B", 1);

    waitpid(pid, NULL, 0);
    printf("채팅 종료\n");
    return 0;
}
