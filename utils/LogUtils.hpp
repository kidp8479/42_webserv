#ifndef LOGUTILS_HPP
#define LOGUTILS_HPP

#include <poll.h>

#include <string>

namespace LogUtils {
std::string pollToStr(short events);
}

#endif
