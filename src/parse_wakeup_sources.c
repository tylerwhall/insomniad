/* 
 * parse_wakeup_sources.c - Routines for parsing wakeup_sources file
 *
 * Copyright (C) 2014 Tyler Hall <tylerwhall@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <malloc.h>
#include <assert.h>

#include "common.h"
#include "parse_wakeup_sources.h"

static int verify_header(const char *line)
{
    int rc;
    const char *header = "name\t\t"
        "active_count\tevent_count\twakeup_count\t"
        "expire_count\tactive_since\ttotal_time\t"
        "max_time\tlast_change\tprevent_suspend_time\n";

    rc = strcmp(line, header);

    if (rc) {
        fprintf(stderr, "wakeup_sources header differs from expected\n");
        fprintf(stderr, "Expected:\n%s\nActual:\n\%s\n", header, line);
        return -1;
    }

    return 0;
}

void wakeup_source_ref(struct wakeup_source *wup)
{
    assert(wup->refcnt > 0);
    wup->refcnt++;
}

void wakeup_source_unref(struct wakeup_source *wup)
{
    assert(wup->refcnt > 0);
    if (--wup->refcnt == 0) {
        free(wup->name);
        wup->name = NULL;
        free(wup);
    }
}

static struct wakeup_source *wakeup_source_create(void)
{
    struct wakeup_source *wup = malloc(sizeof(*wup));

    wup->name = 0;
    wup->last_change = 0;
    wup->refcnt = 1;

    return wup;
}

static int parse_wakeup_source(const char *line, struct wakeup_source **wup)
{
    const char *wup_src_str =
        "%ms \t"                    /* name */
        "%*" SCNu64 "\t"            /* active_count */
        "%*" SCNu64 "\t"            /* event_count */
        "%*" SCNu64 "\t"            /* wakeup_count */
        "%*" SCNu64 "\t"            /* expire_count */
        "%*" SCNu64 "\t"            /* active_since */
        "%*" SCNu64 "\t"            /* total_time */
        "%*" SCNu64 "\t"            /* max_time */
        "%"  SCNu64 "\t"            /* last_change */
        "%*" SCNu64 "\t";           /* prevent_suspend_time */

    *wup = wakeup_source_create();
    if (!wup)
        return -ENOMEM;

    int rc = sscanf(line, wup_src_str, &(*wup)->name, &(*wup)->last_change);

    if (rc != 2) {
        fprintf(stderr, "Unmatched line (count %d)\n%s\n", rc, line);
        fprintf(stderr, "Match string %s\n", wup_src_str);
        if (rc >= 1)
            wakeup_source_unref(*wup);
        return -1;
    }
    return 0;
}

int parse_wakeup_sources(FILE *f, wakeup_source_handler handler, void *data)
{
    char buf[4096 + 128]; /* wake_lock names can be up to a page in length +
                             room for the integer fields. This isn't portable
                             for larger page sizes, but let's hope no one is
                             using a name anywhere near this long. */
    char *ret;
    int rc;
    int count = 0;

    ret = fgets(buf, sizeof(buf), f);
    if (ret != buf) {
        if (!feof(f) && errno) {
            perror("fgets first line");
            return -errno;
        } else {
            fprintf(stderr, "Failed to read first line of wakeup_sources\n");
            return -1;
        }
    }
    rc = verify_header(buf);
    if (rc)
        return rc;

    while (fgets(buf, sizeof(buf), f) == buf) {
        struct wakeup_source *wup;

        rc = parse_wakeup_source(buf, &wup);
        if (rc)
            return rc;
        count++;
        pr_debug("name %-12s\ttime %" PRIu64 "\n", wup->name, wup->last_change);

        if (!handler) {
            wakeup_source_unref(wup);
            continue;
        }
        rc = handler(wup, data);
        wakeup_source_unref(wup);
        if (rc)
            return rc;
    };
    if (!feof(f) && errno) {
        perror("fgets ");
        return -errno;
    }

    return count;
}
