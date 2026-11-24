#include "process_manager.h"
#include "signal_manager.h"
#include "logger.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const char *signal_name(int signo) {
    switch (signo) {
        case SIGINT:
            return "SIGINT";
        case SIGTERM:
            return "SIGTERM";
        case SIGUSR1:
            return "SIGUSR1";
        default:
            return "UNKNOWN";
    }
}

int main(void) {
    if (setup_signal_handlers() == -1) {
        fprintf(stderr, "watchdog: failed to setup signal handlers\n");
        return EXIT_FAILURE;
    }

    sigset_t block_mask;
    sigset_t old_mask;

    if (sigemptyset(&block_mask) == -1 ||
        sigaddset(&block_mask, SIGINT) == -1 ||
        sigaddset(&block_mask, SIGTERM) == -1 ||
        sigaddset(&block_mask, SIGUSR1) == -1 ||
        sigaddset(&block_mask, SIGCHLD) == -1) {
        perror("sigemptyset/sigaddset");
        return EXIT_FAILURE;
    }

    printf("watchdog[%d]: started, launching children...\n", (int)getpid());
    fflush(stdout);

    start_children();
    fflush(stdout);

    while (!shutdown_requested) {
        if (sigprocmask(SIG_BLOCK, &block_mask, &old_mask) == -1) {
            perror("sigprocmask(SIG_BLOCK)");
            return EXIT_FAILURE;
        }

        while (!shutdown_requested && !report_requested && !child_died) {
            if (sigsuspend(&old_mask) == -1 && errno != EINTR) {
                perror("sigsuspend");
                sigprocmask(SIG_SETMASK, &old_mask, NULL);
                return EXIT_FAILURE;
            }
        }

        if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
            perror("sigprocmask(SIG_SETMASK)");
            return EXIT_FAILURE;
        }

        if (child_died) {
            check_children();
            fflush(stdout);
        }

        if (report_requested) {
            report_requested = 0;
            printf("watchdog[%d]: SIGUSR1 received (status report requested)\n",
                   (int)getpid());
            fflush(stdout);
            report_status();
        }
    }

    printf("watchdog[%d]: received %s, exiting main loop\n",
           (int)getpid(), signal_name(shutdown_signal));
    fflush(stdout);

    return EXIT_SUCCESS;
}
