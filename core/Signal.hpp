#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <signal.h>

extern volatile sig_atomic_t g_running;

void handleSigInt(int);

#endif
