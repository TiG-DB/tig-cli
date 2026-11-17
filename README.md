# tig-cli

The `tig` command line tool for [TiG](https://github.com/TiG-DB/tig-docs) (Timeline in Git) repositories. It is a thin
layer over [`libtig`](https://github.com/TiG-DB/tig-lib), pinned as a submodule in `external/tig-lib`, and is meant for
people and for scripts and agents alike.

```sh
tig init ./my-car --kind vehicle --vocabulary vehicle/1 --key ~/.ssh/tig_ed25519
tig -C ./my-car log
tig -C ./my-car state --at 2024-03-11
```

## Commands

| Command | Description |
| --- | --- |
| `tig init <path> --kind <kind> [--vocabulary <v>] [--key <ssh-key>]` | Create a repository with its initial state. With a key, every commit is signed. |
| `tig state [--at <time>]` | Replay events and print the state. `--at` takes `latest`, `YYYY`, `YYYY-MM`, `YYYY-MM-DD` or `YYYY-MM-DDTHH:MM:SSZ`; a partial date means the end of that period. |
| `tig log` | List events in replay order. |

Planned: `event add|correct|delete`, `telemetry append`, `verify`, `clone`.

## For scripts and agents

* `--json` gives JSON on stdout. Errors then go to stderr as `{"error": {"exit_code", "message"}}`.
* Nothing prompts. The signing key comes from `--key` or `TIG_SIGNING_KEY` and must be unencrypted or held by `ssh-agent`.
* Exit codes: `0` ok, `1` rejected by the repository or the spec's checks (or not found), `2` usage, `3` git or system error.
* `-C <path>` chooses the repository (default: the current directory).

## Build

```sh
git submodule update --init --recursive
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

The binary is `build/tig`. libgit2 is downloaded at configure time (pass `-DFETCHCONTENT_SOURCE_DIR_LIBGIT2=<dir>` to use a
local copy) and built into the executable.

## Licence

Licensed under the [Apache License, Version 2.0](LICENSE). See [NOTICE](NOTICE).

The names "TiG" and "TiG Notary" and the TiG logos are not covered by that
licence and may not be used without permission, except to say accurately that your
work is compatible with TiG.
