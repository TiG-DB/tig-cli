#include "cli.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct buffer {
  char *data;
  size_t length;
  size_t capacity;
  int failed;
} buffer;

static void put(buffer *out, const char *text, size_t length) {
  if (out->failed) {
    return;
  }
  if (out->length + length + 1 > out->capacity) {
    size_t capacity = out->capacity != 0 ? out->capacity : 256;
    char *grown;

    while (capacity < out->length + length + 1) {
      capacity *= 2;
    }
    grown = (char *)realloc(out->data, capacity);
    if (grown == NULL) {
      out->failed = 1;
      return;
    }
    out->data = grown;
    out->capacity = capacity;
  }
  memcpy(out->data + out->length, text, length);
  out->length += length;
  out->data[out->length] = '\0';
}

static void put_text(buffer *out, const char *text) {
  put(out, text, strlen(text));
}

static void put_string(buffer *out, const char *text) {
  const unsigned char *p;

  put_text(out, "\"");
  for (p = (const unsigned char *)text; *p != '\0'; ++p) {
    char escape[8];

    switch (*p) {
      case '"': put_text(out, "\\\""); break;
      case '\\': put_text(out, "\\\\"); break;
      case '\n': put_text(out, "\\n"); break;
      case '\r': put_text(out, "\\r"); break;
      case '\t': put_text(out, "\\t"); break;
      default:
        if (*p < 0x20) {
          snprintf(escape, sizeof(escape), "\\u%04x", *p);
          put_text(out, escape);
        } else {
          put(out, (const char *)p, 1);
        }
    }
  }
  put_text(out, "\"");
}

/* the JSON number grammar: -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)? */
static int is_json_number(const char *text) {
  const char *p = text;

  if (*p == '-') {
    ++p;
  }
  if (*p == '0') {
    ++p;
  } else if (*p >= '1' && *p <= '9') {
    while (isdigit((unsigned char)*p)) {
      ++p;
    }
  } else {
    return 0;
  }
  if (*p == '.') {
    ++p;
    if (!isdigit((unsigned char)*p)) {
      return 0;
    }
    while (isdigit((unsigned char)*p)) {
      ++p;
    }
  }
  if (*p == 'e' || *p == 'E') {
    ++p;
    if (*p == '+' || *p == '-') {
      ++p;
    }
    if (!isdigit((unsigned char)*p)) {
      return 0;
    }
    while (isdigit((unsigned char)*p)) {
      ++p;
    }
  }
  return *p == '\0';
}

static void put_node(buffer *out, const tig_node *node) {
  size_t i;
  size_t count;

  switch (tig_node_type_of(node)) {
    case TIG_NODE_LIST:
      put_text(out, "[");
      count = tig_node_count(node);
      for (i = 0; i < count; ++i) {
        if (i != 0) {
          put_text(out, ",");
        }
        put_node(out, tig_node_at(node, i));
      }
      put_text(out, "]");
      break;
    case TIG_NODE_MAP:
      put_text(out, "{");
      count = tig_node_count(node);
      for (i = 0; i < count; ++i) {
        if (i != 0) {
          put_text(out, ",");
        }
        put_string(out, tig_node_key_at(node, i));
        put_text(out, ":");
        put_node(out, tig_node_at(node, i));
      }
      put_text(out, "}");
      break;
    default: {
      const char *text = tig_node_text(node);

      if (text == NULL) {
        put_text(out, "null");
      } else if (tig_node_is_string(node)) {
        put_string(out, text);
      } else if (strcmp(text, "true") == 0 || strcmp(text, "false") == 0 || is_json_number(text)) {
        put_text(out, text);
      } else if (*text == '\0' || strcmp(text, "null") == 0 || strcmp(text, "~") == 0) {
        put_text(out, "null");
      } else {
        /* YAML scalars JSON has no form for, such as .inf or 0x1F */
        put_string(out, text);
      }
    }
  }
}

char *cli_node_to_json(const tig_node *node) {
  buffer out = {0};

  put_node(&out, node);
  if (out.failed) {
    free(out.data);
    return NULL;
  }
  return out.data;
}

char *cli_event_to_json(const char *id, const tig_node *event) {
  buffer out = {0};
  size_t i;
  size_t count = tig_node_count(event);

  put_text(&out, "{\"id\":");
  put_string(&out, id);
  for (i = 0; i < count; ++i) {
    put_text(&out, ",");
    put_string(&out, tig_node_key_at(event, i));
    put_text(&out, ":");
    put_node(&out, tig_node_at(event, i));
  }
  put_text(&out, "}");
  if (out.failed) {
    free(out.data);
    return NULL;
  }
  return out.data;
}

char *cli_json_string(const char *text) {
  buffer out = {0};

  put_string(&out, text);
  if (out.failed) {
    free(out.data);
    return NULL;
  }
  return out.data;
}
