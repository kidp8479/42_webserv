# webserv - developer notes

Internal reference for the team. Not required for evaluation.

---

## Architecture

The project uses a domain-based folder structure rather than the traditional `srcs/includes` split.

```
webserv/
├── config/       # config file parsing (Tokenizer, Builder, Validator)
├── http/         # HTTP request and response structures
├── core/         # sockets, poll event loop, Client and Server classes
├── handlers/     # one handler per HTTP method + CGI handler
├── utils/        # shared utility functions
├── logger/       # Logger class, color macros
├── conf/         # configuration files
├── www/          # static files served by the server
└── tests/        # unit tests (gtest, C++14) 
```

Each folder contains both `.hpp` and `.cpp` files for its domain. This keeps related code together and reduces merge conflicts when teammates work in parallel.

### config/ - config file parsing

3 passes orchestrated by `ConfigParser`:

1. `ConfigTokenizer` - file validation + tokenization -> `vector<Token>`
2. `ConfigBuilder` - token sequence validation + fills `Config` object (normalizes `localhost` -> `127.0.0.1`)
3. `ConfigValidator` - semantic validation (duplicate host:port, port range, required directives)

### http/ - HTTP request and response

`Request` parses raw HTTP/1.1 request bytes: method, URI, headers, body. `Response` builds the raw HTTP response: status line, headers, body. Both classes are owned by the `Client` state machine in `core/`.

### core/ - event loop and connection management

`Server` creates and binds listen sockets. `EventLoop` runs the poll() loop and dispatches events. `Client` is the per-connection state machine: reads request bytes, hands off to handlers, writes the response back.

### handlers/ - HTTP method handlers

One handler class per HTTP method (GET, POST, DELETE) plus a `Router` that resolves the URI to the best matching location block using longest-prefix match. File upload via curl (raw POST body) works. Browser form upload (multipart/form-data) and CGI handler: WIP.

### utils/ - shared utilities

MIME type detection, path resolution helpers, string utilities shared across domains.

### logger/ - logging

`Logger` singleton with log levels (INFO, DEBUG, WARNING). Color macros defined in `logger/colors.hpp`.

### Bonus (WIP)

- Cookies and session management
- Multiple CGI interpreters - `.php` and `.py` already configured in `default.conf`, pending CGI handler completion

---

## Code style

Google C++ style. clang-format enforced via pre-commit hook and `make format`.

Key conventions:
- C++98 strict in all production code, C++14 allowed in tests only
- Always `{}` even for single-line `if` bodies
- `configError()` is the single throw point per class
- Anonymous namespace in `.cpp` preferred over `#define` or `static const` in header
- Class order: public first (constructors, methods), private last (methods, members)
- Doxygen on class and non-trivial methods only, not getters/setters

---

## Setup - clang-format

clang-format keeps code style consistent across the team. Set it up once per machine.

### At school (no sudo)

```bash
# check what version is available
ls /usr/bin/clang-format*

# create an alias (adjust version number as needed)
echo "alias clang-format=clang-format-12" >> ~/.zshrc
source ~/.zshrc

clang-format --version
```

### At home (with sudo)

```bash
sudo apt install clang-format
clang-format --version
```

---

## Setup - git pre-commit hook

Automatically formats staged `.cpp` and `.hpp` files before each commit. Set up once per machine.

```bash
cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}[WARN] clang-format not found - skipping formatting${NC}"
    echo "       see DEV.md setup section to install it"
    exit 0
fi

STAGED=$(git diff --cached --name-only --diff-filter=ACM | grep -E "\.(cpp|hpp)$")

if [ -z "$STAGED" ]; then
    exit 0
fi

echo "-> clang-format applied to :"
for FILE in $STAGED; do
    clang-format -i "$FILE"
    git add "$FILE"
    echo "   $FILE"
done

echo -e "${GREEN}[OK] formatting done${NC}"
exit 0
EOF

chmod +x .git/hooks/pre-commit
```

To format everything at once (useful after cloning or before opening a PR):

```bash
make format
```

---

## Build and tests

```bash
# main binary (C++98, -Werror)
make

# reset demo files (pre-populates files/ dir for delete demo)
make demo

# run all tests (C++14, gtest, run from tests/)
cd tests && make test

# single test suite
cd tests && make test_builder
cd tests && make test_tokenizer
cd tests && make test_validator
etc

# clean
make fclean && make
```

---

## Siege stress test

```bash
# single instance - baseline
siege -b -t 30S http://127.0.0.1:8080/

# two instances in parallel - concurrent load
# terminal 1
siege -b -t 30S http://127.0.0.1:8080/
# terminal 2
siege -b -t 30S http://127.0.0.1:8080/

# monitor memory during siege (third terminal)
watch -n1 'ps aux | grep webserv | grep -v grep'
```

Target: availability > 99.5%, no memory growth, server stays up between runs without restart.
