#include "Signal.hpp"

volatile sig_atomic_t g_running = 1;

void handleSigInt(int) {
    g_running = 0;
}
