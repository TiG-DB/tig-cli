#include "cli.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cli_fail(const cli_context *context, int exit_code, const char *format, ...) {
  char message[1024];
  va_list args;

  va_start(args, format);
  vsnprintf(message, sizeof(message), format, args);
  va_end(args);

  if (context->json) {
    char *quoted = cli_json_string(message);

    fprintf(stderr, "{\"error\":{\"exit_code\":%d,\"message\":%s}}\n", exit_code, quoted != NULL ? quoted : "\"\"");
    free(quoted);
  } else {
    fprintf(stderr, "%s: %s\n", context->program, message);
  }
  return exit_code;
}

int cli_fail_status(const cli_context *context, tig_status status, const char *what) {
  const char *detail = tig_get_last_error();
  int code = (status == TIG_ERROR_REJECTED || status == TIG_ERROR_NOT_FOUND) ? CLI_REJECTED
             : status == TIG_ERROR_INVALID_ARGUMENT                           ? CLI_USAGE
                                                                              : CLI_ERROR;

  return cli_fail(context, code, "%s: %s", what, detail != NULL && *detail != '\0' ? detail : "unknown error");
}

static void usage(FILE *out) {
  fputs("usage: tig [-C <path>] [--json] <command> [options]\n"
        "\n"
        "commands:\n"
        "  init <path> --kind <kind>   create a repository (--vocabulary, --key <ssh-key>)\n"
        "  state [--at <time>]         replay events and print the state\n"
        "  log                         list events in replay order\n"
        "\n"
        "options:\n"
        "  -C <path>   repository to use (default: current directory)\n"
        "  --json      machine-readable output; errors are JSON on stderr\n"
        "  --version   print the version\n"
        "\n"
        "exit codes: 0 ok, 1 rejected or not found, 2 usage, 3 git or system error\n",
        out);
}

int main(int argc, char **argv) {
  cli_context context = {"tig", ".", 0};
  tig_options options = {0};
  const char *command = NULL;
  char **rest = argv + argc;
  int rest_count = 0;
  int result;
  int i;

  for (i = 1; i < argc; ++i) {
    if (command != NULL) {
      /* options after the command belong to it, except the global ones */
      if (strcmp(argv[i], "--json") == 0) {
        context.json = 1;
        memmove(&argv[i], &argv[i + 1], (size_t)(argc - i) * sizeof(char *));
        --argc;
        --i;
        continue;
      }
      if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
        context.repository = argv[i + 1];
        memmove(&argv[i], &argv[i + 2], (size_t)(argc - i - 1) * sizeof(char *));
        argc -= 2;
        --i;
        continue;
      }
      continue;
    }
    if (strcmp(argv[i], "--json") == 0) {
      context.json = 1;
    } else if (strcmp(argv[i], "-C") == 0 && i + 1 < argc) {
      context.repository = argv[++i];
    } else if (strcmp(argv[i], "--version") == 0) {
      printf("tig %s (libtig %s)\n", TIG_CLI_VERSION, tig_version());
      return CLI_OK;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      usage(stdout);
      return CLI_OK;
    } else if (argv[i][0] == '-') {
      return cli_fail(&context, CLI_USAGE, "unknown option %s (see --help)", argv[i]);
    } else {
      command = argv[i];
      rest = argv + i + 1;
    }
  }
  if (command == NULL) {
    usage(stderr);
    return CLI_USAGE;
  }
  rest_count = (int)((argv + argc) - rest);

  if (tig_init(&options) != TIG_OK) {
    return cli_fail(&context, CLI_ERROR, "cannot initialize libtig");
  }
  if (strcmp(command, "init") == 0) {
    result = cmd_init(&context, rest_count, rest);
  } else if (strcmp(command, "state") == 0) {
    result = cmd_state(&context, rest_count, rest);
  } else if (strcmp(command, "log") == 0) {
    result = cmd_log(&context, rest_count, rest);
  } else {
    result = cli_fail(&context, CLI_USAGE, "unknown command \"%s\" (see --help)", command);
  }
  tig_shutdown();
  return result;
}
