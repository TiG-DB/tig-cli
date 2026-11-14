#ifndef TIG_CLI_H
#define TIG_CLI_H

#include "tig/tig.h"

#include <stddef.h>
#include <stdint.h>

/* Exit codes are part of the interface, so that scripts and agents can tell what went wrong:
   a write that the spec's checks reject can be fixed and retried, a usage error or git error cannot. */
enum {
  CLI_OK = 0,
  CLI_REJECTED = 1, /* the repository or the spec's checks said no, or there was nothing to find */
  CLI_USAGE = 2,
  CLI_ERROR = 3 /* git, file system or signing failure */
};

typedef struct cli_context {
  const char *program;
  const char *repository; /* -C, default "." */
  int json;
} cli_context;

/* Prints an error to stderr, as {"error": {"status", "message"}} with --json, and returns the exit code. */
int cli_fail(const cli_context *context, int exit_code, const char *format, ...);
int cli_fail_status(const cli_context *context, tig_status status, const char *what);

int cmd_init(const cli_context *context, int argc, char **argv);
int cmd_state(const cli_context *context, int argc, char **argv);
int cmd_log(const cli_context *context, int argc, char **argv);

/* json.c: writes a node as JSON. Returns a malloc'd NUL-terminated string, or NULL. Free with free(). */
char *cli_node_to_json(const tig_node *node);
/* the same, with "id" as the first key of the map */
char *cli_event_to_json(const char *id, const tig_node *event);
/* a JSON string literal, quoted and escaped */
char *cli_json_string(const char *text);

/* time.c: a time as the user gives it (latest, YYYY, YYYY-MM, YYYY-MM-DD or RFC 3339 UTC with Z) to the last
   millisecond it covers. Returns 0 on success. */
int cli_parse_at(const char *text, int64_t *unix_ms);

/* process.c: runs argv with stdin from /dev/null, capturing stdout and stderr. Returns the exit status, or -1. */
int cli_run(const char *const *argv, char *output, size_t output_size);

/* signing.c: a device that signs with an SSH key through ssh-keygen. */
typedef struct cli_signer {
  tig_device device;
  char key_path[1024];
  char allowed_signer[1024];
} cli_signer;
/* key is the private key's path, or a .pub path for a key held by ssh-agent; stays valid for the signer's life */
int cli_signer_load(cli_signer *signer, const char *key, char *error, size_t error_size);

#endif
