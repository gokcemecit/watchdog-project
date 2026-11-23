#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define MAX_CHILDREN 2
#define MAX_RESTARTS 3

#include <sys/types.h>
#include <signal.h>

struct child {
    pid_t pid;
    char *mode;
    int restarts;
    int alive;
};

extern struct child children[MAX_CHILDREN];
extern volatile sig_atomic_t child_died;

void start_children(void);
void check_children(void);
void stop_restarting(void);

#endif
