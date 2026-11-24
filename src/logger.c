#include <stdio.h>
#include <time.h>
#include "logger.h"

void log_event(const char *message) {
    FILE *file = fopen("watchdog.log", "a");

    if (file == NULL) {
        perror("Error opening log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *time_info = localtime(&now);

    if (time_info == NULL) {
        fclose(file);
        return;
    }

    char timestamp[30];

    if (strftime(timestamp, sizeof(timestamp),
                 "%Y-%m-%d %H:%M:%S", time_info) == 0) {
        fclose(file);
        return;
    }

    fprintf(file, "[%s] %s\n", timestamp, message);

    fclose(file);
}
