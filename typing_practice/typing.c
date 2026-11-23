#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 같은 위치의 다른 문자와 길이 차이를 오타로 계산한다.
static size_t count_errors(const char *answer, const char *input)
{
    size_t a = strlen(answer);
    size_t b = strlen(input);
    size_t common = a < b ? a : b;
    size_t errors = a > b ? a - b : b - a;

    for (size_t i = 0; i < common; i++) {
        if (answer[i] != input[i]) {
            errors++;
        }
    }

    return errors;
}

int main(void)
{
    const char *sentences[] = {
        "Hello Linux",
        "Practice makes perfect",
        "Open source project"
    };

    const size_t count = sizeof(sentences) / sizeof(sentences[0]);

    char *input = NULL;
    size_t capacity = 0;
    size_t total_chars = 0;
    size_t total_errors = 0;
    double total_seconds = 0.0;

    printf("=== 타자 연습 ===\n");
    printf("대소문자와 공백을 구분하여 입력하세요.\n\n");

    for (size_t i = 0; i < count; i++) {
        printf("[%zu/%zu] 제시 문장: %s\n",
               i + 1, count, sentences[i]);
        printf("입력: ");
        fflush(stdout);

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        if (getline(&input, &capacity, stdin) == -1) {
            fprintf(stderr, "\n입력이 종료되었습니다.\n");
            free(input);
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        // Enter로 입력된 줄바꿈 문자를 제거한다.
        input[strcspn(input, "\r\n")] = '\0';

        size_t errors = count_errors(sentences[i], input);

        double seconds =
            (double)(end.tv_sec - start.tv_sec)
            + (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;

        total_chars += strlen(input);
        total_errors += errors;
        total_seconds += seconds;

        printf("오타 수: %zu개 / 소요 시간: %.2f초\n\n",
               errors, seconds);
    }

    double cpm = total_seconds > 0.0
               ? (double)total_chars * 60.0 / total_seconds
               : 0.0;

    printf("=== 최종 결과 ===\n");
    printf("전체 입력 문자 수: %zu자\n", total_chars);
    printf("전체 오타 수: %zu개\n", total_errors);
    printf("전체 소요 시간: %.2f초\n", total_seconds);
    printf("평균 분당 타자수: %.2f타/분\n", cpm);

    free(input);
    return 0;
}
