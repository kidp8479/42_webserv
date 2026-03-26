*This project has been created as part of the 42 curriculum by diade-so, cpollock and pafroidu.*

---

# webserv

HTTP/1.1 server written in C++98

Beyond building a functional web server, we focus on clean code, best practices and team organization : atomic commits, GitHub flow, code review via PRs, consistent style enforced with clang-format, and a modular architecture designed for parallel development.

---

## Description

webserv is an HTTP/1.1 server written from scratch in C++98. It handles multiple clients simultaneously using a non-blocking I/O event loop (poll), parses HTTP requests, serves static files, handles file uploads and deletions, and supports CGI execution for dynamic content.

The server is configured via a configuration file inspired by NGINX syntax.

---

## Instructions

### Clone the repo (from github for now, not from the vogosphere yet)

> NOTE : you need to have been added as contributors to have the rights to commit and push.

> NOTE : also for Charlie, let me know if you have troubles with github, you might have some set up to do at school to be rightly identified with your github credentials

```bash
git clone git@github.com:kidp8479/42_webserv.git
or
git clone https://github.com/kidp8479/42_webserv.git

cd 42_webserv
```

### Compile

```bash
make
```
> nothing to compile yet obviously, but the `make format` rule has been set up.

### Run

```bash
# with a config file passed as argument
./webserv conf/some_conf_files.conf

# without argument - uses default config from conf/default.conf
./webserv 
```

---

## Setup - clang-format

We use clang-format to keep the code style consistent across the team.
You need to set it up once on each machine you work on. We chose Google coding style, as it is popular in open source projects. 

### clang-format - two ways to use it

We have two ways to run clang-format, and they complement each other.

### pre-commit hook - automatic

Once the hook is set up on your machine, it runs automatically every time you commit. It only formats the files you staged, so it is fast and easy. You do not have to think about it, it just does it.

This is the recommended way for day-to-day development.

### make format - manual

Formats all .cpp and .hpp files in the project at once.
```bash
make format
```

This is useful when :
- you just cloned the repo and want to format everything at once
- you do not have the hook installed on your current machine
- you want to double-check everything is clean before opening a PR

### why both ?

The hook is convenient but local: if a teammate has not set it up, their code arrives in the PR unformatted. 
`make format` is the safety net that anyone can run at any time, regardless of their setup.

In practice :
- day-to-day : let the hook handle it
- before a PR : run make format to be sure
- new machine : set up the hook first, it will save you time

### At school (no sudo)

clang-format is already installed on 42 machines but under a versioned name.
Check what's available :

```bash
ls /usr/bin/clang-format*
```

You should see something like `clang-format-12`. Create an alias so you can use it as `clang-format` 
(important to have the right name because it's how it's called in the set up files):

```zsh
echo "alias clang-format=clang-format-12" >> ~/.zshrc
source ~/.zshrc
```

Verify it works :

```zsh
clang-format --version
```
> use "bash" everywhere instead of "zsh" if your shell is bash

### At home (with sudo)

```bash
sudo apt install clang-format
```

Verify :

```bash
clang-format --version
```

---

## Setup - git hook (pre-commit)

The pre-commit hook automatically formats your staged `.cpp` and `.hpp` files before each commit.
You need to set it up once per machine.

```bash
cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}[WARN] clang-format not found - skipping formatting${NC}"
    echo "       see README setup section to install it"
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

If clang-format is not available on your machine, you can also format manually before committing :

```bash
make format
```

---

## Project architecture

> The project uses a **domain-based folder structure** rather than the traditional `srcs/includes` split you are used to from other 42 projects.

### What it looks like

```
webserv/
├── config/       => config file parsing
├── http/         => HTTP request and response structures
├── server/       => core network logic (sockets, poll event loop)
├── handlers/     => one handler per HTTP method + CGI
├── utils/        => shared utility functions
├── conf/         => configuration files
├── www/          => static files served by the server
└── tests/        => test programs and scripts
```

Each folder contains both `.hpp` and `.cpp` files for its domain.

> During dev time : check the `.gitkeep` file inside each folder for an example description of what could go there. This is totally an example and everything can be 100% changed according to your needs. I've just put some example here as part of my reseearch about the project architecture.

### Why this structure instead of srcs/includes ?

The traditional `srcs/includes` split organizes files **by type** (all headers together, all sources together).
The domain-based structure organizes files **by what they do**.

**Easier to navigate** - when you are working on the HTTP parser, everything you need is in `http/`. You do not have to jump between two separate folders to find `Request.hpp` and `Request.cpp`.

**Better for parallel work** - each person owns a domain folder. Merge conflicts are less likely because your work stays in your corner of the codebase.

**Scales better** - if the project grows, adding a new feature means adding a new folder, not dumping more files into already crowded `srcs/` and `includes/` directories.

**More modern** - this is how most real-world C++ projects are structured today. Getting comfortable with it now is a good habit for the future.

> This architecture is a **proposal**. The final structure will be decided together as a team. If you think something should be organized differently, bring it up as nothing is set in stone yet.

---

## Resources

- [RFC 7230 - HTTP/1.1 Message Syntax](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 - HTTP/1.1 Semantics](https://datatracker.ietf.org/doc/html/rfc7231)
- [NGINX documentation](https://nginx.org/en/docs/)
- [CGI specification](https://datatracker.ietf.org/doc/html/rfc3875)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### AI usage

TO COMPLETE as we go