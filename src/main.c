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

static void graceful_shutdown(void) {
    char log_msg[128];

    snprintf(log_msg, sizeof(log_msg),
             "Received %s, initiating graceful shutdown",
             signal_name(shutdown_signal));
    log_event(log_msg);
    printf("watchdog[%d]: %s\n", (int)getpid(), log_msg);
    fflush(stdout);

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (!children[i].alive || children[i].pid <= 0) {
            continue;
        }

        if (kill(children[i].pid, SIGTERM) == 0) {
            snprintf(log_msg, sizeof(log_msg),
                     "Sent SIGTERM to child %d (mode %s)",
                     (int)children[i].pid,
                     children[i].mode ? children[i].mode : "unknown");
            log_event(log_msg);
            printf("watchdog[%d]: %s\n", (int)getpid(), log_msg);
            fflush(stdout);
        } else if (errno == ESRCH) {
            children[i].alive = 0;
        } else {
            perror("kill(SIGTERM)");
        }
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

        if (shutdown_requested) {
            break;
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

    graceful_shutdown();

    return EXIT_SUCCESS;
}
