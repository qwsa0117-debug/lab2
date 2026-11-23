#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// ls의 기본 동작처럼 숨김 파일과 숨김 폴더는 제외한다.
static int visible(const struct dirent *entry)
{
    return entry->d_name[0] != '.';
}

static int list_directory(const char *path)
{
    struct dirent **entries;
    int count = scandir(path, &entries, visible, alphasort);

    if (count < 0) {
        perror(path);
        return 1;
    }

    int failed = 0;

    printf("%s:\n", path);

    // 현재 폴더의 파일과 디렉토리 이름을 출력한다.
    for (int i = 0; i < count; i++) {
        printf("%s\n", entries[i]->d_name);
    }
    printf("\n");

    // 하위 디렉토리는 같은 함수를 다시 호출하여 탐색한다.
    for (int i = 0; i < count; i++) {
        size_t length = strlen(path)
                      + strlen(entries[i]->d_name) + 2;

        char *full_path = malloc(length);

        if (full_path == NULL) {
            fprintf(stderr, "메모리 할당 실패\n");
            failed = 1;
        } else {
            snprintf(full_path, length, "%s/%s",
                     path, entries[i]->d_name);

            struct stat info;

            if (lstat(full_path, &info) == -1) {
                perror(full_path);
                failed = 1;
            } else if (S_ISDIR(info.st_mode)) {
                if (list_directory(full_path) != 0) {
                    failed = 1;
                }
            }

            free(full_path);
        }

        free(entries[i]);
    }

    free(entries);
    return failed;
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "사용법: %s [디렉토리]\n", argv[0]);
        return 1;
    }

    const char *path = argc == 2 ? argv[1] : ".";
    return list_directory(path);
}
