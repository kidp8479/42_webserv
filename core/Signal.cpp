#include "Signal.hpp"

volatile sig_atomic_t g_running = 1;

void handleSigInt(int) {
    g_running = 0;
}

void setupSignals() {
    signal(SIGINT, handleSigInt);
    signal(SIGPIPE, SIG_IGN);

    struct sigaction sa_chld;
    sa_chld.sa_handler = SIG_DFL;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_NOCLDWAIT;  // auto-reap children, no zombies
    sigaction(SIGCHLD, &sa_chld, NULL);
}
