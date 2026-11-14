#include "cli.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* days since 1970-01-01 of a civil date (Howard Hinnant's algorithm) */
static int64_t days_from_civil(int64_t y, int m, int d) {
  int64_t era;
  int64_t yoe;
  int64_t doy;
  int64_t doe;

  y -= m <= 2;
  era = (y >= 0 ? y : y - 399) / 400;
  yoe = y - era * 400;
  doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

static int days_in_month(int year, int month) {
  static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  if (month == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
    return 29;
  }
  return days[month - 1];
}

/* exactly count digits at *p, advancing it */
static int digits(const char **p, int count, int *value) {
  int i;

  *value = 0;
  for (i = 0; i < count; ++i) {
    if (!isdigit((unsigned char)**p)) {
      return -1;
    }
    *value = *value * 10 + (**p - '0');
    ++*p;
  }
  return 0;
}

#define DAY_MS INT64_C(86400000)

int cli_parse_at(const char *text, int64_t *unix_ms) {
  const char *p = text;
  int year;
  int month = 0;
  int day = 0;

  if (strcmp(text, "latest") == 0) {
    *unix_ms = TIG_TIME_LATEST;
    return 0;
  }
  if (digits(&p, 4, &year) != 0) {
    return -1;
  }
  if (*p == '\0') { /* the end of the year */
    *unix_ms = days_from_civil(year + 1, 1, 1) * DAY_MS - 1;
    return 0;
  }
  if (*p++ != '-' || digits(&p, 2, &month) != 0 || month < 1 || month > 12) {
    return -1;
  }
  if (*p == '\0') { /* the end of the month */
    *unix_ms = (month == 12 ? days_from_civil(year + 1, 1, 1) : days_from_civil(year, month + 1, 1)) * DAY_MS - 1;
    return 0;
  }
  if (*p++ != '-' || digits(&p, 2, &day) != 0 || day < 1 || day > days_in_month(year, month)) {
    return -1;
  }
  if (*p == '\0') { /* the end of the day */
    *unix_ms = (days_from_civil(year, month, day) + 1) * DAY_MS - 1;
    return 0;
  }

  /* an instant: THH:MM:SS[.fff]Z */
  {
    int hour;
    int minute;
    int second;
    int64_t millis = 0;

    if (*p++ != 'T' || digits(&p, 2, &hour) != 0 || *p++ != ':' || digits(&p, 2, &minute) != 0 || *p++ != ':' ||
        digits(&p, 2, &second) != 0 || hour > 23 || minute > 59 || second > 60) {
      return -1;
    }
    if (*p == '.') {
      int scale = 100;

      ++p;
      if (!isdigit((unsigned char)*p)) {
        return -1;
      }
      while (isdigit((unsigned char)*p)) {
        millis += (*p - '0') * scale;
        scale /= 10;
        if (scale == 0) {
          scale = 0; /* digits past the millisecond are ignored */
        }
        ++p;
      }
    }
    if (strcmp(p, "Z") != 0) {
      return -1;
    }
    *unix_ms = (days_from_civil(year, month, day) * 86400 + hour * 3600 + minute * 60 + second) * 1000 + millis;
    return 0;
  }
}
