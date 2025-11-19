#include "vf_signals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

// Global flag defined in the header
volatile sig_atomic_t shutdown_requested = 0;

/**
 * @brief Generic function to configure a single signal handler using sigaction.
 * (Course concept: A deep and practical application of the entire Signal Handling module.)
 */
static int set_signal_handler(int signum, void (*handler)(int), int flags) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = flags;
    // Block all signals during the execution of the handler
    sigfillset(&sa.sa_mask); 
    
    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction failed");
        return -1;
    }
    return 0;
}

// --- Specific Signal Handler Implementations ---

// 1. Graceful Shutdown Handler (SIGINT, SIGTERM)
void vf_graceful_shutdown_handler(int signum) {
    // This handler sets the flag, allowing the main loop to exit cleanly (resource cleanup).
    shutdown_requested = 1; 
    fprintf(stderr, "\nSignal %d received. Initiating graceful shutdown...\n", signum);
}

// 2. SIGCHLD Handler (Server-Specific)
// Implementation: To automatically reap zombie child processes.
void vf_sigchld_handler(int signum) {
    int saved_errno = errno;
    // WNOHANG: returns immediately if no child has exited
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Child process reaped successfully.
    }
    errno = saved_errno;
}

// 3. Dynamic Logging Handler (Server-Specific)
// Implementation: To dynamically change the server's logging verbosity.
void vf_sigusr1_handler(int signum) {
    fprintf(stderr, "SIGUSR1 received. Dynamically adjusting server log level.\n");
    // [Logic to increment/decrement a global logging variable goes here]
}

// --- Main Setup Functions ---

int vf_client_signal_setup() {
    // SIGINT / SIGTERM: For graceful shutdown.
    if (set_signal_handler(SIGINT, vf_graceful_shutdown_handler, 0) != 0) return -1;
    if (set_signal_handler(SIGTERM, vf_graceful_shutdown_handler, 0) != 0) return -1;

    // SIGTSTP / SIGCONT: To support shell job control (Ctrl+Z).
    if (set_signal_handler(SIGTSTP, SIG_DFL, 0) != 0) return -1; // Default handling for stop
    if (set_signal_handler(SIGCONT, SIG_DFL, 0) != 0) return -1; // Default handling for continue
    
    return 0;
}

int vf_server_signal_setup() {
    // Graceful Shutdown (SIGTERM/SIGINT)
    if (set_signal_handler(SIGTERM, vf_graceful_shutdown_handler, 0) != 0) return -1;
    if (set_signal_handler(SIGINT, vf_graceful_shutdown_handler, 0) != 0) return -1;

    // SIGCHLD: To automatically reap zombie child processes.
    // SA_RESTART is used to prevent system calls from failing after the handler returns.
    if (set_signal_handler(SIGCHLD, vf_sigchld_handler, SA_RESTART) != 0) return -1;
    
    // SIGUSR1 / SIGUSR2: To dynamically change logging verbosity.
    if (set_signal_handler(SIGUSR1, vf_sigusr1_handler, 0) != 0) return -1;
    
    // SIGALRM: To implement network timeouts.
    // A specific vf_alarm_handler function would be implemented here.
    
    return 0;
}
