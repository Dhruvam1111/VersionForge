# VersionForge

VersionForge is a compact, educational distributed version-control system written in C. It implements core VCS building blocks — repository initialization, object storage and hashing, commits, branching, checkouts, merging, and rebasing — plus a simple client/server push/pull protocol for sharing objects. The codebase is intentionally small and readable, making VersionForge well suited for learning, experimenting, and prototyping version-control ideas.

## Authors ✨
- Dhruvam Panchal (202401415)
- Dwij Patel (202401418)
- Shubh Patel (202401478)
- Yashrajsinh Solanki (202401481)

## Key Highlights ✨
- **Small, readable C codebase**: clear separation between client and server logic under `src/` and public headers in `include/`.
- **Client and server**: builds two executables: `version_forge` (client CLI) and `vf_server` (network server).
- **Core VCS features**: `init`, `commit`, `log`, `status`, `branch`, `checkout`, `merge`, `rebase` and object storage similar to common DVCS designs.
- **Network sync**: basic `push`, `pull`, `fork` operations using a custom TCP protocol.
- **Concurrency & robustness**: uses threadpool utilities, signal handlers, and careful socket handling in the server.

---

## Table of Contents 📚
- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Installation and Build](#installation-and-build)
- [Quickstart and Usage](#quickstart-and-usage)
- [Server and Network Usage](#server-and-network-usage)
- [Development and Testing](#development-and-testing)
- [Future Enhancements](#future-enhancements)
- [Contributing](CONTRIBUTING.md)

---

<a id="overview"></a>
## Overview 📖

VersionForge is an instructional, compact distributed version-control system that demonstrates the fundamental building blocks of modern VCS tools. The project models object storage and hashing, commit graphs, branch management, checkouts, merges, and rebases in a readable C codebase. It provides a command-line client (`version_forge`) for local repository operations and a minimal TCP server (`vf_server`) to demonstrate remote synchronization via `push`/`pull`/`fork`. VersionForge is designed for learning, experimentation, and extending VCS concepts.

<a id="features"></a>
## Features ⚙️
- Repository initialization: `init` creates the internal `.minivcs` storage.
- Configuration: `config --global <key> <value>` to store global settings (e.g., `user.name`).
- Object storage & hashing: write/read blob/tree/commit objects and compute SHA-based identifiers.
- Commit history and logging: `commit -m "message"` and `log` to examine history.
- Status & checkout: `status` and `checkout <branch|hash>` to move HEAD.
- Branching: `branch <name>` to create branches.
- Merge & Rebase: `merge <branch>` and `rebase -i <branch>` for integrating changes.
- Push/Pull/Fork: simple client/server network protocol to share object data between repositories.

<a id="architecture"></a>
## Architecture 🏗️
- Source layout:
	- `src/` — C source files for client, server and helpers.
	- `include/` — public headers for components like `network.h`, `database.h`, etc.
	- `version_forge` — CLI client executable (built from `src/*.c` except `server.c`).
	- `vf_server` — standalone server binary (built from server-related sources).
- Important modules:
	- `database.*` — object storage and object traversal.
	- `commit.*`, `branch.*`, `checkout.*` — repository operations.
	- `network*` and `network_client.*` — client/server communication and protocol.
	- `threadpool.*`, `vf_signals.*` — concurrency and graceful shutdown handling.

<a id="requirements"></a>
## Requirements 🧩
- A POSIX-like build environment (Linux/macOS recommended).
- Compiler: `gcc` (or any C compiler supporting C99/C11 features used in the project).
- Build tools: `make`.
- Libraries: `zlib` and OpenSSL (`libcrypto`) used for compression and hashing. On Debian/Ubuntu these are `zlib1g-dev` and `libssl-dev`.

Windows users: the project is written for POSIX. You can build on Windows via WSL (recommended) or adapt socket/unistd calls for native Windows builds.

<a id="installation-and-build"></a>
## Installation and Build 🛠️

1. Clone the repository (or download the sources):

	```bash
	git clone <repo-url>
	cd VersionForge
	```

2. Install dependencies (Debian/Ubuntu example):

	```bash
	sudo apt-get update
	sudo apt-get install build-essential libssl-dev zlib1g-dev -y
	```

3. Build using `make` (creates `version_forge` and `vf_server`):

	```bash
	make
	```

4. Alternatively, use the convenience script:

	```bash
	./setup.sh build
	```

5. Clean build artifacts:

	```bash
	make clean
	./setup.sh clean
	```

<a id="quickstart-and-usage"></a>
## Quickstart and Usage 🚦

After building, the main CLI is `./version_forge`. Basic usage follows the commands printed by the binary. Example workflows:

- Initialize a repository in a project folder:

	```bash
	./version_forge init
	```

- Configure user info:

	```bash
	./version_forge config --global user.name "Alice Dev"
	./version_forge config --global user.email "alice@example.com"
	```

- Make a commit:

- Create a new file (example) and show it before committing:

	```bash
	# create a new file using echo
	echo "Hello VersionForge" > hello.txt
	# show the file contents (quick check)
	cat hello.txt
	# show status to see new/untracked files
	./version_forge status
	# commit the new file
	./version_forge commit -m "Add hello.txt"
	```

- Edit an existing file and show changes before committing:

	```bash
	# append a line to an existing file
	echo "More content" >> hello.txt
	# show the updated file
	cat hello.txt
	# check status to confirm the modification
	./version_forge status
	# commit the update
	./version_forge commit -m "Update hello.txt"
	```

- See commit history and status:

	```bash
	./version_forge log
	./version_forge status
	```

- Branch and switch:

	```bash
	./version_forge branch feature-x
	./version_forge checkout feature-x
	```

- Merge or rebase:

	```bash
	./version_forge merge feature-x
	./version_forge rebase -i main
	```

<a id="server-and-network-usage"></a>
## Server & Network Usage 🌐

VersionForge ships a simple TCP server that accepts `PUSH`, `PULL`, and `FORK` commands. Server default port is `9090` (see `include/network.h`).

Start the server (on the machine that will host the remote repository):

```bash
./vf_server
# Server logs connection and handles client requests
```

By default the client expects to talk to `127.0.0.1:9090`; modify the code or add a simple configuration to point to a remote host.

Client-side push/pull:

```bash
./version_forge push   # sends missing objects to the remote server
./version_forge pull   # fetches objects from the remote server
./version_forge fork   # asks the server to create a copy/fork of the repository
```

Notes:
- The network protocol is basic and intended for demonstration. Objects are transmitted as text commands and the server stores received objects into the `.minivcs` storage area.
- For production use you should secure the transport (TLS), improve authentication, and harden concurrency.

<a id="development-and-testing"></a>
## Development & Testing 🧪

- Run `make` and exercise commands manually in a test directory.
- The `server.c` logs operations and expects well-formed commands from `network_client.c`.
- Use `./version_forge test-signals` to exercise signal handling (press Ctrl+C to trigger graceful shutdown in that test command).

<a id="future-enhancements"></a>
## Future Enhancements 🔭
- Authentication and encrypted transport (TLS).
- Better remote configuration (URL-based remotes similar to Git).
- Transfer delta/compressed pack files instead of per-object transfers to improve performance.
- Unit and integration tests with an automated test harness.
- Windows-native build support.
- Better user-friendly CLI parsing (argument parser) and help text improvements.

For contribution guidelines and the recommended pull-request workflow, see the project-level `CONTRIBUTING.md` at the repository root.

---