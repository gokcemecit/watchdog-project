#include "process_manager.h"
#include "signal_manager.h"
#include "logger.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHUTDOWN_TIMEOUT_SEC 3

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

static int has_alive_children(void) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (children[i].alive && children[i].pid > 0) {
            return 1;
        }
    }
    return 0;
}

static void reap_shutdown_children(void) {
    int status;
    pid_t pid;

    while (1) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            char log_msg[128];

            for (int i = 0; i < MAX_CHILDREN; i++) {
                if (children[i].pid == pid) {
                    children[i].alive = 0;
                    break;
                }
            }

            if (WIFEXITED(status)) {
                snprintf(log_msg, sizeof(log_msg),
                         "Reaped child %d during shutdown (exit code %d)",
                         (int)pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                snprintf(log_msg, sizeof(log_msg),
                         "Reaped child %d during shutdown (signal %d)",
                         (int)pid, WTERMSIG(status));
            } else {
                snprintf(log_msg, sizeof(log_msg),
                         "Reaped child %d during shutdown",
                         (int)pid);
            }
            log_event(log_msg);
            printf("watchdog[%d]: %s\n", (int)getpid(), log_msg);
            fflush(stdout);
        } else if (pid == 0) {
            break;
        } else {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ECHILD) {
                for (int i = 0; i < MAX_CHILDREN; i++) {
                    children[i].alive = 0;
                }
            } else {
                perror("waitpid");
            }
            break;
        }
    }
}

static void graceful_shutdown(void) {
    char log_msg[128];
    struct itimerval zero_timer = {{0, 0}, {0, 0}};

    if (setitimer(ITIMER_REAL, &zero_timer, NULL) == -1) {
        perror("setitimer(disarm)");
    }

    stop_restarting();

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

    struct timespec start_time;
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) == -1) {
        perror("clock_gettime");
        start_time.tv_sec = 0;
        start_time.tv_nsec = 0;
    }

    reap_shutdown_children();

    while (has_alive_children()) {
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            break;
        }

        double elapsed = (double)(now.tv_sec - start_time.tv_sec) +
                         (double)(now.tv_nsec - start_time.tv_nsec) / 1e9;
        if (elapsed >= (double)SHUTDOWN_TIMEOUT_SEC) {
            break;
        }

        struct timespec pause_ts = {0, 100000000L};
        if (nanosleep(&pause_ts, NULL) == -1 && errno != EINTR) {
            perror("nanosleep");
        }
        reap_shutdown_children();
    }

    if (has_alive_children()) {
        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (!children[i].alive || children[i].pid <= 0) {
                continue;
            }

            if (kill(children[i].pid, SIGKILL) == 0) {
                snprintf(log_msg, sizeof(log_msg),
                         "Timeout reached, sent SIGKILL to child %d (mode %s)",
                         (int)children[i].pid,
                         children[i].mode ? children[i].mode : "unknown");
                log_event(log_msg);
                printf("watchdog[%d]: %s\n", (int)getpid(), log_msg);
                fflush(stdout);
            } else if (errno == ESRCH) {
                children[i].alive = 0;
            } else {
                perror("kill(SIGKILL)");
            }
        }
    }

    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            for (int i = 0; i < MAX_CHILDREN; i++) {
                if (children[i].pid == pid) {
                    children[i].alive = 0;
                    break;
                }
            }

            if (WIFEXITED(status)) {
                snprintf(log_msg, sizeof(log_msg),
                         "Final reap: child %d exited with code %d",
                         (int)pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                snprintf(log_msg, sizeof(log_msg),
                         "Final reap: child %d terminated by signal %d",
                         (int)pid, WTERMSIG(status));
            } else {
                snprintf(log_msg, sizeof(log_msg),
                         "Final reap: child %d terminated",
                         (int)pid);
            }
            log_event(log_msg);
            printf("watchdog[%d]: %s\n", (int)getpid(), log_msg);
            fflush(stdout);
        } else if (pid == -1 && errno == EINTR) {
            continue;
        } else {
            if (pid == -1 && errno != ECHILD) {
                perror("waitpid(final)");
            }
            break;
        }
    }

    log_event("Graceful shutdown complete");
    printf("watchdog[%d]: Graceful shutdown complete\n", (int)getpid());
    fflush(stdout);
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
        sigaddset(&block_mask, SIGCHLD) == -1 ||
        sigaddset(&block_mask, SIGALRM) == -1) {
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
