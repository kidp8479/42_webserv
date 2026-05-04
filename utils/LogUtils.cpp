#include "LogUtils.hpp"

std::string LogUtils::pollToStr(short events) {
	std::string out;

	if (events & POLLIN)  out += "POLLIN|";
	if (events & POLLOUT) out += "POLLOUT|";
	if (events & POLLERR) out += "POLLERR|";
	if (events & POLLHUP) out += "POLLHUP|";
	if (events & POLLNVAL) out += "POLLNVAL|";

	if (!out.empty())
		out.erase(out.size() - 1);
	else
		out = "NONE";

	return out;
}
