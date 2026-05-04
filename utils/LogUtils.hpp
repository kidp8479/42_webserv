#ifndef LOGUTILS_HPP
#define LOGUTILS_HPP

#include <string>
#include <poll.h>

namespace LogUtils {
	std::string pollToStr(short events);
}

#endif
