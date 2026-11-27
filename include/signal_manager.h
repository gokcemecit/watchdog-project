#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

#include <signal.h>

/* Async-signal-safe flags set by handlers and consumed in the main loop. */
extern volatile sig_atomic_t shutdown_requested;
extern volatile sig_atomic_t shutdown_signal;
extern volatile sig_atomic_t report_requested;

/* Registers sigaction() handlers for SIGINT, SIGTERM, and SIGUSR1. Returns 0 on success, -1 on error. */
int setup_signal_handlers(void);

#endif
