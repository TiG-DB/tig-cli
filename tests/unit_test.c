#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                    \
  do {                                                                      \
    if (!(condition)) {                                                     \
      fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
      ++failures;                                                           \
    }                                                                       \
  } while (0)

static int64_t at(const char *text) {
  int64_t value = -1;

  return cli_parse_at(text, &value) == 0 ? value : -1;
}

static void test_times(void) {
  CHECK(at("latest") == TIG_TIME_LATEST);
  CHECK(at("1970-01-01") == 86400000 - 1);
  CHECK(at("1970") == 365 * 86400000LL - 1);
  CHECK(at("1970-02") == 59 * 86400000LL - 1);
  CHECK(at("1970-12") == 365 * 86400000LL - 1);
  CHECK(at("2024-02-29") == 1709251199999LL);
  CHECK(at("2026-11-10T12:00:00Z") == 1794312000000LL);
  CHECK(at("2026-11-10T12:00:00.250Z") == 1794312000250LL);
  CHECK(at("2023-02-29") == -1);
  CHECK(at("2024-13") == -1);
  CHECK(at("2024-03-11T07:12:31") == -1);
  CHECK(at("yesterday") == -1);
  CHECK(at("") == -1);
}

static void test_json(void) {
  tig_node *node = NULL;
  const char *yaml = "kind: vehicle\nitems:\n  entity:\n    vin: \"A1\"\n    mileage: 65807\n    ok: true\n    ratio: 57.5\n"
                     "    none: null\n    tags: [a, \"b\\\"c\"]\n";
  char *json;

  CHECK(tig_node_parse(yaml, strlen(yaml), &node) == TIG_OK);
  json = cli_node_to_json(node);
  CHECK(json != NULL);
  CHECK(strcmp(json, "{\"kind\":\"vehicle\",\"items\":{\"entity\":{\"vin\":\"A1\",\"mileage\":65807,\"ok\":true,"
                     "\"ratio\":57.5,\"none\":null,\"tags\":[\"a\",\"b\\\"c\"]}}}") == 0);
  if (json != NULL) {
    fprintf(stderr, "%s\n", json);
  }
  free(json);
  tig_node_free(node);

  yaml = "time: \"2024-03-11\"\n";
  CHECK(tig_node_parse(yaml, strlen(yaml), &node) == TIG_OK);
  json = cli_event_to_json("7fd2ma", node);
  CHECK(json != NULL && strcmp(json, "{\"id\":\"7fd2ma\",\"time\":\"2024-03-11\"}") == 0);
  free(json);
  tig_node_free(node);

  json = cli_json_string("a\"b\\c\n\x01");
  CHECK(json != NULL && strcmp(json, "\"a\\\"b\\\\c\\n\\u0001\"") == 0);
  free(json);
}

int main(void) {
  tig_options options = {0};

  CHECK(tig_init(&options) == TIG_OK);
  test_times();
  test_json();
  tig_shutdown();
  return failures == 0 ? 0 : 1;
}
