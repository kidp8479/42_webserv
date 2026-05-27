*This project has been created as part of the 42 curriculum by diade-so, cpollock and pafroidu.*

---

# webserv

HTTP/1.1 server written in C++98

Beyond building a functional web server, we focus on clean code, best practices and team organization: atomic commits, GitHub flow, code review via PRs, consistent style enforced with clang-format, and a modular architecture designed for parallel development.

---

## Description

webserv is an HTTP/1.1 server written from scratch in C++98. It handles multiple clients simultaneously using a non-blocking I/O event loop (poll), parses HTTP requests, serves static files, handles file uploads and deletions, and supports CGI execution for dynamic content.

The server is configured via a configuration file inspired by NGINX syntax. Multiple server blocks and location blocks are supported, allowing fine-grained routing and per-route access control.

---

## Instructions

### Compile

```bash
make
```

### Run

```bash
# with a configuration file
./webserv conf/default.conf

# without argument - uses conf/default.conf by default
./webserv
```

### Reset demo files (pre-populates files for the delete demo)

```bash
make demo
```

---

## Configuration

The server is configured via a `.conf` file. Each `server` block defines a virtual server, and each `location` block defines routing rules for a URI prefix.

### Supported directives

| directive | scope | description |
|---|---|---|
| `listen host:port` | server | address and port to bind |
| `client_max_body_size` | server | max request body (ex: `1M`, `512k`) |
| `error_page code path` | server | custom HTML page for a given HTTP error code |
| `methods` | location | allowed HTTP methods (GET, POST, DELETE) |
| `root path` | location | filesystem root for URI resolution |
| `index file` | location | default file for directory requests |
| `autoindex on\|off` | location | directory listing |
| `upload_path path` | location | where POST uploads are stored |
| `return code uri` | location | HTTP redirect |
| `cgi .ext /path/to/interpreter` | location | CGI execution by file extension |

### Demo configurations

Each demo conf is self-contained and focuses on one feature. Run the server and open the URL in a browser, or use the curl commands at the top of each file.

| file | port | feature |
|---|---|---|
| `conf/default.conf` | 8080 / 8081 | all features combined, two server blocks |
| `conf/demo_static_file.conf` | 8082 | static file serving |
| `conf/demo_redirect.conf` | 8083 | HTTP 301 redirect |
| `conf/demo_upload.conf` | 8084 | file upload via POST |
| `conf/demo_delete.conf` | 8085 | file deletion via DELETE |

---

## Resources

- [RFC 7230 - HTTP/1.1 Message Syntax](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 - CGI specification](https://datatracker.ietf.org/doc/html/rfc3875)
- [NGINX documentation](https://nginx.org/en/docs/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### AI usage

Claude (claude.ai / Claude Code CLI) was used throughout the project as a socratic tutor and technical assistant:

- Debugging: identifying root causes of keep-alive, fd leak, and request parsing bugs
- Documentation: doxygen comments, log message conventions, README
- Testing: designing curl test sequences, analyzing siege stress test results
