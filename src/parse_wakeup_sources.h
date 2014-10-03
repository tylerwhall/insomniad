#pragma once

#define WUP_SRC_NAMELEN 12
#define xstr(x) str(x)
#define str(x) #x
#define WUP_SRC_NAMELEN_STR xstr(WUP_SRC_NAMELEN)

struct wakeup_source {
    char name[WUP_SRC_NAMELEN];
    uint64_t last_change;
};

/* Called for each wakeup source read from wakeup_sources
 * Return 0 to continue, nonzero to break */
typedef int (*wakeup_source_handler)(struct wakeup_source *wup, void *data);

int parse_wakeup_sources(FILE *f, wakeup_source_handler handler, void *data);
