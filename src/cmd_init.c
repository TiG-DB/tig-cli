#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_init(const cli_context *context, int argc, char **argv) {
  const char *path = NULL;
  const char *kind = NULL;
  const char *vocabulary = NULL;
  const char *key = getenv("TIG_SIGNING_KEY");
  cli_signer signer;
  char error[256];
  tig_repository *repository = NULL;
  tig_node *initial;
  tig_status status;
  int i;

  for (i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "--kind") == 0 && i + 1 < argc) {
      kind = argv[++i];
    } else if (strcmp(argv[i], "--vocabulary") == 0 && i + 1 < argc) {
      vocabulary = argv[++i];
    } else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc) {
      key = argv[++i];
    } else if (argv[i][0] != '-' && path == NULL) {
      path = argv[i];
    } else {
      path = NULL;
      break;
    }
  }
  if (path == NULL || kind == NULL || *kind == '\0') {
    return cli_fail(context, CLI_USAGE,
                    "usage: tig init <path> --kind <kind> [--vocabulary <name/version>] [--key <ssh-key>]");
  }
  if (key != NULL && *key == '\0') {
    key = NULL;
  }
  if (key != NULL && cli_signer_load(&signer, key, error, sizeof(error)) != 0) {
    return cli_fail(context, CLI_ERROR, "%s", error);
  }

  status = tig_repository_create(path, key != NULL ? &signer.device : NULL, &repository);
  if (status != TIG_OK) {
    return cli_fail_status(context, status, "cannot create repository");
  }

  initial = tig_node_map();
  if (initial == NULL || tig_node_set(initial, "kind", tig_node_string(kind)) != TIG_OK ||
      (vocabulary != NULL && tig_node_set(initial, "vocabulary", tig_node_string(vocabulary)) != TIG_OK)) {
    tig_node_free(initial);
    tig_repository_close(repository);
    return cli_fail(context, CLI_ERROR, "out of memory");
  }
  status = tig_state_write_initial(repository, initial);
  tig_node_free(initial);
  if (status == TIG_OK) {
    status = tig_repository_commit(repository, "Initial state");
  }
  tig_repository_close(repository);
  if (status != TIG_OK) {
    return cli_fail_status(context, status, "cannot write initial state");
  }

  if (context->json) {
    char *quoted_path = cli_json_string(path);
    char *quoted_kind = cli_json_string(kind);

    printf("{\"path\":%s,\"kind\":%s,\"signed\":%s}\n", quoted_path != NULL ? quoted_path : "null",
           quoted_kind != NULL ? quoted_kind : "null", key != NULL ? "true" : "false");
    free(quoted_path);
    free(quoted_kind);
  } else {
    printf("created %s repository at %s%s\n", kind, path, key != NULL ? " (signed)" : " (unsigned)");
  }
  return CLI_OK;
}
