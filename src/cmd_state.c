#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_state(const cli_context *context, int argc, char **argv) {
  const char *at = "latest";
  int64_t unix_ms;
  tig_repository *repository = NULL;
  tig_node *state = NULL;
  tig_status status;
  int i;

  for (i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "--at") == 0 && i + 1 < argc) {
      at = argv[++i];
    } else {
      return cli_fail(context, CLI_USAGE, "usage: tig state [--at <time>]");
    }
  }
  if (cli_parse_at(at, &unix_ms) != 0) {
    return cli_fail(context, CLI_USAGE,
                    "invalid --at \"%s\": use latest, YYYY, YYYY-MM, YYYY-MM-DD or YYYY-MM-DDTHH:MM:SSZ", at);
  }

  status = tig_repository_open(context->repository, &repository);
  if (status != TIG_OK) {
    return cli_fail_status(context, status, "cannot open repository");
  }
  status = tig_state_replay(repository, unix_ms, &state);
  if (status != TIG_OK) {
    tig_repository_close(repository);
    return cli_fail_status(context, status, "replay failed");
  }

  if (context->json) {
    char *json = cli_node_to_json(state);

    if (json == NULL) {
      status = TIG_ERROR_INVALID_ARGUMENT;
    } else {
      puts(json);
      free(json);
    }
  } else {
    char *yaml = NULL;

    status = tig_node_render(state, &yaml);
    if (status == TIG_OK) {
      fputs(yaml, stdout);
      tig_free(yaml);
    }
  }
  tig_node_free(state);
  tig_repository_close(repository);
  return status == TIG_OK ? CLI_OK : cli_fail_status(context, status, "cannot render state");
}
