#include "Signal.hpp"

/**
 * @brief Global running flag for graceful shutdown.
 * Set to 0 when SIGINT is received to stop the main loop.
 */
volatile sig_atomic_t g_running = 1;

/**
 * @brief SIGINT handler.
 * Triggers graceful shutdown by updating the global flag.
 */
void handleSigInt(int) {
    g_running = 0;
}

/**
 * @brief Configures process-level signal handling.
 *
 * - SIGINT: graceful shutdown request
 * - SIGPIPE: ignored to prevent crashes on broken pipes
 * - SIGCHLD: auto-reaps child processes (no zombies)
 */
void setupSignals() {
    signal(SIGINT, handleSigInt);
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa_chld;
    sa_chld.sa_handler = SIG_DFL;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa_chld, NULL);
}
