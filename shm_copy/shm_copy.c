#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

// 자식 프로세스가 공유 메모리의 내용을 복사본에 저장한다.
static int save_copy(const char *path, const void *shared, size_t size)
{
    // 기존 파일을 실수로 덮어쓰지 않도록 새 파일만 생성한다.
    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0644);

    if (fd == -1) {
        perror("복사본 생성");
        return 1;
    }

    if (ftruncate(fd, (off_t)size) == -1) {
        perror("복사본 크기 설정");
        close(fd);
        unlink(path);
        return 1;
    }

    if (size > 0) {
        void *output = mmap(NULL, size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);

        if (output == MAP_FAILED) {
            perror("복사본 mmap");
            close(fd);
            unlink(path);
            return 1;
        }

        memcpy(output, shared, size);

        if (msync(output, size, MS_SYNC) == -1) {
            perror("복사본 동기화");
            munmap(output, size);
            close(fd);
            unlink(path);
            return 1;
        }

        munmap(output, size);
    }

    close(fd);
    printf("자식: 복사본 저장 완료 (%zu바이트)\n", size);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "사용법: %s 원본파일 복사본파일\n", argv[0]);
        return 1;
    }

    int source = open(argv[1], O_RDONLY);

    if (source == -1) {
        perror("원본 열기");
        return 1;
    }

    struct stat info;

    if (fstat(source, &info) == -1) {
        perror("원본 정보 확인");
        close(source);
        return 1;
    }

    if (!S_ISREG(info.st_mode)) {
        fprintf(stderr, "일반 파일을 원본으로 지정하세요.\n");
        close(source);
        return 1;
    }

    size_t size = (size_t)info.st_size;
    char name[64];

    snprintf(name, sizeof(name), "/lab2_copy_%ld", (long)getpid());

    int shm = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);

    if (shm == -1) {
        perror("shm_open");
        close(source);
        return 1;
    }

    // 이름은 제거하고 열린 공유 메모리 객체는 계속 사용한다.
    // 마지막 참조가 닫히면 객체도 해제된다.
    if (shm_unlink(name) == -1) {
        perror("shm_unlink");
        close(shm);
        close(source);
        return 1;
    }

    if (ftruncate(shm, info.st_size) == -1) {
        perror("공유 메모리 크기 설정");
        close(shm);
        close(source);
        return 1;
    }

    void *shared = NULL;

    // 빈 파일에는 길이 0의 mmap을 호출하지 않는다.
    if (size > 0) {
        shared = mmap(NULL, size, PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm, 0);

        if (shared == MAP_FAILED) {
            perror("공유 메모리 mmap");
            close(shm);
            close(source);
            return 1;
        }

        void *input = mmap(NULL, size, PROT_READ,
                           MAP_PRIVATE, source, 0);

        if (input == MAP_FAILED) {
            perror("원본 mmap");
            munmap(shared, size);
            close(shm);
            close(source);
            return 1;
        }

        memcpy(shared, input, size);
        munmap(input, size);
    }

    close(source);

    printf("부모: 원본을 공유 메모리에 저장 (%zu바이트)\n", size);
    fflush(stdout);

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        if (size > 0) {
            munmap(shared, size);
        }
        close(shm);
        return 1;
    }

    if (pid == 0) {
        int result = save_copy(argv[2], shared, size);

        if (size > 0) {
            munmap(shared, size);
        }

        close(shm);
        return result;
    }

    int status;
    pid_t waited;

    do {
        waited = waitpid(pid, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (size > 0) {
        munmap(shared, size);
    }
    close(shm);

    if (waited == -1) {
        perror("waitpid");
        return 1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "파일 복사 실패\n");
        return 1;
    }

    printf("파일 복사 완료 및 공유 메모리 정리 완료\n");
    return 0;
}
