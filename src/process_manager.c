#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_manager.h"
#include <signal.h>
#include <sys/wait.h>

struct child children[MAX_CHILDREN];

volatile sig_atomic_t child_died = 0;

static void sigchld_handler(int sig) {
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
        _exit(1);
    }

    children[i].pid = pid;
    children[i].alive = 1;
    printf("started child %d (mode %s)\n", pid, children[i].mode);
}

void start_children(void) {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char *modes[MAX_CHILDREN] = {"crash", "loop"};

    for (int i = 0; i < MAX_CHILDREN; i++) {
        children[i].mode = modes[i];
        children[i].restarts = 0;
        start_child(i);
    }   
}

void check_children(void) {
    int status;
    pid_t pid;

    child_died = 0;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (children[i].pid == pid) {
                children[i].alive = 0;
                printf("child %d terminated\n", pid);
            }
        }
    }
}
