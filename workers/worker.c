#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "usage: %s normal|crash|loop\n", argv[0]);
        return 1;
    }

    printf("worker %d started in mode %s\n", getpid(), argv[1]);

    if (strcmp(argv[1], "normal") == 0) {
        sleep(3);
        printf("worker %d exiting normally\n", getpid());
        return 0;
    }

    if (strcmp(argv[1], "crash") == 0) {
        sleep(2);
        printf("worker %d is crashing\n", getpid());
        return 1;
    }

    if (strcmp(argv[1], "loop") == 0) {
        while (1) {
            sleep(1);
        }
    }

    fprintf(stderr, "unknown mode: %s\n", argv[1]);
    return 1;
}