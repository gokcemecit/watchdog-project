#include <stdio.h>
#include <time.h>
#include "logger.h"
#include "process_manager.h"

// writes an event to the log file
void log_event(const char *message) {
    FILE *file = fopen("watchdog.log", "a");

    if (file == NULL) {
        perror("Error opening log file");
        return;
    }
    
    // get the current date and time
    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);

    if (time_info == NULL) {
        fclose(file);
        return;
    }

    char timestamp[30];

    // format the timestamp
    if (strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", time_info) == 0) {
        fclose(file);
        return;
    }

    fprintf(file, "[%s] %s\n", timestamp, message);

    fclose(file);
}

// reports the status of all child processes
void report_status(void) {
    char message[256];

    log_event("Child process status report:");

    // collect and log each child's information
    for (int i = 0; i < MAX_CHILDREN; i++) {
        snprintf(message, sizeof(message),
                 "Child %d: PID=%d, Mode=%s, State=%s, Restarts=%d",
                 i + 1,
                 (int)children[i].pid,
                 children[i].mode,
                 children[i].alive ? "Running" : "Not running",
                 children[i].restarts);

        log_event(message);
    }
}