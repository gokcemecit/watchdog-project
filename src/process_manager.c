/*
 * process_manager.c
 * Student 2 (k41uk) — starts, monitors, and restarts child processes with fork/exec.
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_manager.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "logger.h"

struct child children[MAX_CHILDREN];

volatile sig_atomic_t child_died = 0;

static int restart_enabled = 1;  // when 0, check_children will not restart

void stop_restarting(void) {
    restart_enabled = 0;  // graceful shutdown has started
}

// Flag only; no printf/log (async-signal-safe)
static void sigchld_handler(int sig) {
    (void)sig;
    child_died = 1;  // a child exited; the main loop will reap it
}

// Use the same flag for periodic checks
static void sigalrm_handler(int sig) {
    (void)sig;
    child_died = 1;
}

static void start_child(int i) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execl("./worker", "worker", children[i].mode, NULL);
        perror("execl");
        _exit(1);  // exec failed; use _exit, not exit
    }

    children[i].pid = pid;
    children[i].alive = 1;
    printf("started child %d (mode %s)\n", pid, children[i].mode);
    char message[256];

    snprintf(message, sizeof(message),
         "Started child %d (mode %s)", (int)pid, children[i].mode);

    log_event(message);
}

void start_children(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {  // child termination
        perror("sigaction(SIGCHLD)");
    }

    struct sigaction sa_alrm;
    sa_alrm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_alrm.sa_mask);
    sa_alrm.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1) {  // periodic check
        perror("sigaction(SIGALRM)");
    }

    struct itimerval timer;
    timer.it_value.tv_sec = 5;       // first alarm after 5 seconds
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 5;    // repeat every 5 seconds
    timer.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
    }

    char *modes[MAX_CHILDREN] = {"crash", "loop"};  // default worker modes

    for (int i = 0; i < MAX_CHILDREN; i++) {
        children[i].mode = modes[i];
        children[i].restarts = 0;
        start_child(i);
    }   
}

void check_children(void) {
    int status;
    pid_t pid;

    child_died = 0;  // clear the flag; reap below
    
    while (1) {
        pid = waitpid(-1, &status, WNOHANG);  // collect zombies if any, don't block
        if (pid == -1 && errno == EINTR) {
            continue;  // retry if interrupted by a signal
        }
        if (pid <= 0) {
            break;  // no more dead children
        }

        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (children[i].pid == pid) {
                children[i].alive = 0;
                int normal_exit = 0;
                char message[256];
                
                if (WIFEXITED(status)) {
                    int code = WEXITSTATUS(status);
                    printf("child %d exited with code %d\n", pid, code);
                    snprintf(message, sizeof(message),
                           "Child %d exited with code %d", (int)pid, code);
                    log_event(message);
                    if (code == 0) { normal_exit = 1; }  // exit 0: normal termination
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);
                    printf("child %d killed by signal %d\n", pid, sig);
                    snprintf(message, sizeof(message),
                         "Child %d killed by signal %d", (int)pid, sig);
                    log_event(message);
                }

                if (normal_exit) {
                    printf("child (mode %s) finished normally, not restarting\n", children[i].mode);
		    snprintf(message, sizeof(message),
        	           "Child (mode %s) finished normally, not restarting",
                           children[i].mode);
                           log_event(message);
                } else if (restart_enabled && children[i].restarts < MAX_RESTARTS) {
                    children[i].restarts++;
                    printf("restarting child (mode %s), attempts %d\n", children[i].mode, children[i].restarts);
                    snprintf(message, sizeof(message),
                              "Restarting child (mode %s), attempts %d",
                               children[i].mode, children[i].restarts);
                    log_event(message);
                    start_child(i);  // restart
                } else {
                    // restart limit reached or shutdown; give up
                    printf("child (mode %s) reached max restarts, giving up\n", children[i].mode);
                    snprintf(message, sizeof(message),
                            "Child (mode %s) reached max restarts, giving up",
                            children[i].mode);
                    log_event(message);
                }
            }
        }
    }
}
