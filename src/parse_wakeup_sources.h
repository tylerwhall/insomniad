#pragma once

#include <stdio.h>
#include <inttypes.h>

struct wakeup_source {
    char *name;
    uint64_t last_change;
};

/* Called for each wakeup source read from wakeup_sources
 * Return 0 to continue, nonzero to break */
typedef int (*wakeup_source_handler)(struct wakeup_source *wup, void *data);

int parse_wakeup_sources(FILE *f, wakeup_source_handler handler, void *data);
void destroy_wakeup_source(struct wakeup_source *wup);
