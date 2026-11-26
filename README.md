# Watchdog Project

SWE 301 – System Programming  
Fall 2026–2027 | Term Project 1

## Project Description

This project is a POSIX signal-based process monitoring and management system written in C. The watchdog starts child processes, monitors their status and restarts them when they terminate unexpectedly.

## Features

- Creating child processes using `fork()` and `exec()`
- Monitoring child processes using `SIGCHLD` and `waitpid()`
- Restarting failed processes up to 3 times
- Periodic process monitoring
- Status reporting using `SIGUSR1`
- Graceful shutdown using `SIGINT` and `SIGTERM`
- Event logging with timestamps

## Project Structure

| Directory | Description |
|---|---|
| `src/` | Main program, process management, signal handling and logging |
| `include/` | Header files |
| `workers/` | Worker program with normal, crash and loop modes |
| `tests/` | Automated test script |
| `docs/` | Project documentation |

## Requirements

- Linux environment
- GCC
- Make
- Bash

## Build

Run the following command in the project directory:

```bash
make
```

This creates two executables: `watchdog` and `worker`.

## Run

```bash
./watchdog
```

To request a status report, open another terminal and run:

```bash
kill -USR1 <watchdog_pid>
```

The watchdog PID is displayed when the program starts. Status reports and other events are saved in `watchdog.log`.

To stop the watchdog, press `Ctrl+C` or send `SIGTERM`:

```bash
kill -TERM <watchdog_pid>
```

## Testing

Run the automated test script:

```bash
./tests/test_scenarios.sh
```

The script checks the following scenarios:

1. Unexpected termination
2. Restart limit
3. Normal termination
4. SIGUSR1 status reporting
5. Graceful shutdown
6. Log verification

All six tests passed on Ubuntu.

## Clean

To remove the compiled files:

```bash
make clean
```