/*
 * process_manager.h
 * Student 2 (k41uk) — child process table and process manager API.
 */
#ifndef PROCESS_MANAGER_H
#define PROCESS_MANAGER_H

#define MAX_CHILDREN 2  // number of children monitored at once
#define MAX_RESTARTS 3  // max restarts after unexpected termination

#include <sys/types.h>
#include <signal.h>

struct child {
    pid_t pid;     // child's PID
    char *mode;    // worker mode: crash, loop, or normal
    int restarts;  // how many times it has been restarted
    int alive;     // 1: running, 0: dead
};

extern struct child children[MAX_CHILDREN];
extern volatile sig_atomic_t child_died;  // handler only sets this flag

void start_children(void);   // install signals and start children
void check_children(void);   // reap dead children, restart if needed
void stop_restarting(void);  // disable new restarts during shutdown

#endif
