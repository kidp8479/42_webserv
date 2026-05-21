#include "CgiProcess.hpp"

CgiProcess::CgiProcess(pid_t pid, int ready_fd, Client& client, EventLoop& loop) :
	pid_(pid),
	read_fd_(read_fd),
	client_(client),
	loop_(loop),
	timeout_(TimeoutSeconds::kCGI),
	done_(false)
{}

int CgiProcess::getFd() const {
	return read_fd_.getFd();
}

void CgiProcess::handle(short revents) {
	// event loop checks isTimedout() in cleanup(), but handle() may get
	// called before clenaup() runs
	if (timeout_.expired()) {
		kill(pid_, SIGKILL);
		waitpid(pid_, NULL, 0);
		read_fd_.reset();
		client_.receiveError(
			"HTTP/1.1 504 Gateway Timeout\r\n"
            "Content-Length: 0\r\n\r\n");
		done_ = true;
		return;
	}
	char buffer[4096];
	ssize_t n = read(read_fd_.getFd(), buffer, sizeof(buffer));

	if (n > 0) {
		output_.append(buffer, n);

	} else if (n == 0) {
		// EOF. cgi script finished, we have all output
		finish();
	} else if ( errno != EAGAIN && errno != EWOULDBLOCK) {
		// real read error
		kill(pid_, SIGKILL);
		waitpid(pid_, NULL, 0);
		read_fd_.reset();
		client_.receiveError(
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 0\r\n\r\n");
		done_ = true;
	}
}

bool CgiProcess::isDone() const {
	return done_;
}

bool CgiProcess::isTimedOut() const {
	return timeout_.expired();
}

const char* CgiProcess::name() const {
	return "CgiProcess";
}

void CgiProcess::finish() {
	waitpid(pid_, NULL, 0);
	read_fd_.reset();

	// cgi output format
	size_t separator = output_.find("\r\n\r\n");
	if (separator == std::string::npos) {
		separator = output_.find("\n\n");
	}
	if (separator == std::string::npos) {
		client_.receiveError(
		    "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Length: 0\r\n\r\n");
		done_ = true;
		return;
	}

	std::string cgi_headers = output_.substr(0, separator);
	std::string cgi_body = output_.substr(separator + 2);

	std::ostringstream oss;
	oss << cgi_body.size();

	std::string response =
		"HTTP/1.1 200 OK\r\n" +
        cgi_headers + "\r\n" +
        "Content-Length: " + oss.str() + "\r\n"
        "\r\n" +
        cgi_body;

	client_.receiveResponse(response);
	done_ = true;
}
