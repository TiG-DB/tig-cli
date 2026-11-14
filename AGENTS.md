# AGENTS.md

`tig-cli` builds the `tig` executable: a C99 command line tool over `libtig` (the `external/tig-lib` submodule). The
specification lives in `tig-docs`, checked out as `external/tig-lib/docs`; when the CLI and the spec disagree, raise it.

- `src/main.c`: option parsing, dispatch, exit codes. `src/cmd_*.c`: one file per command. `src/json.c`: node to JSON.
- Keep libgit2 out of this project; use only `tig/tig.h`. Logic that other consumers need (e.g. verification) belongs in
  `libtig`, not here.
- Exit codes and the `--json` shape are interface: change them deliberately and update the README.
- Build and test: `cmake -S . -B build && cmake --build build && ctest --test-dir build`.
- Version is in `project(tig-cli VERSION ...)` in `CMakeLists.txt`.
