/*
 * worker.c
 * Student 2 (k41uk) — sample worker process monitored by the watchdog.
 * Modes: normal (exit 0), crash (exit 1), loop (infinite).
 */
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
        return 0;  // successful exit; the watchdog will not restart it
    }

    if (strcmp(argv[1], "crash") == 0) {
        sleep(2);
        printf("worker %d is crashing\n", getpid());
        return 1;  // unexpected exit; the watchdog will restart it
    }

    if (strcmp(argv[1], "loop") == 0) {
        while (1) {
            sleep(1);  // dies on SIGTERM during shutdown
        }
    }

    fprintf(stderr, "unknown mode: %s\n", argv[1]);
    return 1;
}