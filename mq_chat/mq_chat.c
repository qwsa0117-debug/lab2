#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/wait.h>

#define MESSAGE_SIZE 1024

static int send_message(mqd_t queue, const char *name)
{
    char *line = NULL;
    size_t capacity = 0;

    while (1) {
        printf("%s 입력: ", name);
        fflush(stdout);

        if (getline(&line, &capacity, stdin) == -1) {
            free(line);
            if (mq_send(queue, "/quit", 6, 0) == -1) {
                perror("mq_send");
            }
            return 0;
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) >= MESSAGE_SIZE) {
            printf("메시지는 %d바이트 미만으로 입력하세요.\n",
                   MESSAGE_SIZE);
            continue;
        }

        int quit = strcmp(line, "/quit") == 0;

        if (mq_send(queue, line, strlen(line) + 1, 0) == -1) {
            perror("mq_send");
            free(line);
            return 0;
        }

        free(line);
        return !quit;
    }
}

static int receive_message(mqd_t queue, const char *peer)
{
    char message[MESSAGE_SIZE + 1];

    ssize_t length = mq_receive(queue, message, MESSAGE_SIZE, NULL);

    if (length == -1) {
        perror("mq_receive");
        return 0;
    }

    message[length] = '\0';

    if (strcmp(message, "/quit") == 0) {
        printf("%s가 채팅을 종료했습니다.\n", peer);
        return 0;
    }

    printf("%s에게 받은 메시지: %s\n\n", peer, message);
    return 1;
}

static void chat(mqd_t incoming, mqd_t outgoing,
                 const char *name, const char *peer,
                 int starts_first)
{
    while (1) {
        if (!starts_first && !receive_message(incoming, peer)) {
            break;
        }

        if (!send_message(outgoing, name)) {
            break;
        }

        if (starts_first && !receive_message(incoming, peer)) {
            break;
        }
    }
}

int main(void)
{
    char name_ab[64];
    char name_ba[64];

    snprintf(name_ab, sizeof(name_ab), "/lab2_ab_%ld", (long)getpid());
    snprintf(name_ba, sizeof(name_ba), "/lab2_ba_%ld", (long)getpid());

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MESSAGE_SIZE;

    mqd_t ab = mq_open(name_ab, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);

    if (ab == (mqd_t)-1) {
        perror("mq_open A-B");
        return 1;
    }

    mqd_t ba = mq_open(name_ba, O_CREAT | O_EXCL | O_RDWR, 0600, &attr);

    if (ba == (mqd_t)-1) {
        perror("mq_open B-A");
        mq_close(ab);
        mq_unlink(name_ab);
        return 1;
    }

    setvbuf(stdin, NULL, _IONBF, 0);

    printf("=== POSIX 메시지 큐 채팅 ===\n");
    printf("A와 B가 번갈아 입력합니다.\n");
    printf("/quit를 입력하면 종료합니다.\n\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        mq_close(ab);
        mq_close(ba);
        mq_unlink(name_ab);
        mq_unlink(name_ba);
        return 1;
    }

    if (pid == 0) {
        chat(ab, ba, "B", "A", 0);
        mq_close(ab);
        mq_close(ba);
        return 0;
    }

    chat(ba, ab, "A", "B", 1);
    waitpid(pid, NULL, 0);

    mq_close(ab);
    mq_close(ba);
    mq_unlink(name_ab);
    mq_unlink(name_ba);

    printf("채팅 종료 및 메시지 큐 삭제 완료\n");
    return 0;
}
