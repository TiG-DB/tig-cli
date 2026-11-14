#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct log_state {
  const cli_context *context;
  int count;
} log_state;

/* "inspected, replaced": the action types, so a line says what happened without the full event */
static void action_summary(const tig_node *event, char *out, size_t size) {
  const tig_node *actions = tig_node_get(event, "actions");
  size_t used = 0;
  size_t i;

  out[0] = '\0';
  if (actions == NULL || tig_node_type_of(actions) != TIG_NODE_LIST) {
    return;
  }
  for (i = 0; i < tig_node_count(actions); ++i) {
    const char *type = tig_node_text(tig_node_get(tig_node_at(actions, i), "type"));
    int wrote;

    if (type == NULL) {
      continue;
    }
    wrote = snprintf(out + used, size - used, "%s%s", used != 0 ? ", " : "", type);
    if (wrote < 0 || (size_t)wrote >= size - used) {
      break;
    }
    used += (size_t)wrote;
  }
}

static tig_status visit(void *context, const char *id, const tig_node *event) {
  log_state *state = (log_state *)context;
  ++state->count;

  if (state->context->json) {
    char *json = cli_event_to_json(id, event);

    if (json == NULL) {
      return TIG_ERROR_INVALID_ARGUMENT;
    }
    printf("%s%s", state->count > 1 ? ",\n " : "[", json);
    free(json);
  } else {
    const char *time = tig_node_text(tig_node_get(event, "time"));
    const char *type = tig_node_text(tig_node_get(event, "type"));
    char summary[256];

    action_summary(event, summary, sizeof(summary));
    printf("%-24s %-16s %-10s %s\n", time != NULL ? time : "?", id, type != NULL ? type : "", summary);
  }
  return TIG_OK;
}

int cmd_log(const cli_context *context, int argc, char **argv) {
  tig_repository *repository = NULL;
  log_state state = {context, 0};
  tig_status status;

  (void)argv;
  if (argc != 0) {
    return cli_fail(context, CLI_USAGE, "usage: tig log");
  }
  status = tig_repository_open(context->repository, &repository);
  if (status != TIG_OK) {
    return cli_fail_status(context, status, "cannot open repository");
  }
  status = tig_events_each(repository, visit, &state);
  tig_repository_close(repository);
  if (status != TIG_OK) {
    return cli_fail_status(context, status, "cannot read events");
  }
  if (context->json) {
    puts(state.count == 0 ? "[]" : "]");
  }
  return CLI_OK;
}
