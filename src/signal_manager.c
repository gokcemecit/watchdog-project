#include "signal_manager.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

volatile sig_atomic_t shutdown_requested = 0;
volatile sig_atomic_t shutdown_signal = 0;
volatile sig_atomic_t report_requested = 0;

static void handle_shutdown_signal(int signo) {
    int saved_errno = errno;

    if (!shutdown_requested) {
        shutdown_signal = signo;
        shutdown_requested = 1;
    }

    errno = saved_errno;
}

static void handle_sigusr1(int signo) {
    int saved_errno = errno;

    (void)signo;
    report_requested = 1;

    errno = saved_errno;
}

int setup_signal_handlers(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_shutdown_signal;
    if (sigemptyset(&sa.sa_mask) == -1 ||
        sigaddset(&sa.sa_mask, SIGINT) == -1 ||
        sigaddset(&sa.sa_mask, SIGTERM) == -1 ||
        sigaddset(&sa.sa_mask, SIGUSR1) == -1) {
        perror("sigemptyset/sigaddset");
        return -1;
    }
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction(SIGINT)");
        return -1;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM)");
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigusr1;
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        return -1;
    }
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction(SIGUSR1)");
        return -1;
    }

    return 0;
}
