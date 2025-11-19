#ifndef VF_SIGNALS_H
#define VF_SIGNALS_H

#include <signal.h>

// Global flag to signal main loops to exit gracefully
extern volatile sig_atomic_t shutdown_requested;

/**
 * @brief Registers the general-purpose signal handlers for the client tool.
 * Includes SIGINT/SIGTERM for graceful shutdown and SIGTSTP/SIGCONT for job control.
 */
int vf_client_signal_setup();

/**
 * @brief Registers the comprehensive signal handlers for the server daemon.
 * Includes SIGCHLD, SIGUSR1/SIGUSR2, and standard shutdowns.
 */
int vf_server_signal_setup();

#endif // VF_SIGNALS_H
