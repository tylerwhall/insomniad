#pragma once

#include <stdio.h>
#include <inttypes.h>

struct wakeup_source {
    char *name;
    uint64_t last_change;
    int refcnt;
};

/* Called for each wakeup source read from wakeup_sources
 * Return 0 to continue, nonzero to break */
typedef int (*wakeup_source_handler)(struct wakeup_source *wup, void *data);

int parse_wakeup_sources(FILE *f, wakeup_source_handler handler, void *data);
void wakeup_source_ref(struct wakeup_source *wup);
void wakeup_source_unref(struct wakeup_source *wup);
