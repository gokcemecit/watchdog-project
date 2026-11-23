#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_manager.h"

struct child children[MAX_CHILDREN];

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
    char *modes[MAX_CHILDREN] = {"crash", "loop"};

    for (int i = 0; i < MAX_CHILDREN; i++) {
        children[i].mode = modes[i];
        children[i].restarts = 0;
        start_child(i);
    }
}
