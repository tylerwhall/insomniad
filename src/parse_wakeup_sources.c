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

#define WUP_SRC_NAMELEN 12
#define xstr(x) str(x)
#define str(x) #x
#define WUP_SRC_NAMELEN_STR xstr(WUP_SRC_NAMELEN)

struct wakeup_source {
    char name[WUP_SRC_NAMELEN];
    uint64_t last_change;
};

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

static int parse_wakeup_source(const char *line, struct wakeup_source *wup)
{
    const char *wup_src_str =
        "%" WUP_SRC_NAMELEN_STR "s \t"  /* name */
        "%*" SCNu64 "\t"            /* active_count */
        "%*" SCNu64 "\t"            /* event_count */
        "%*" SCNu64 "\t"            /* wakeup_count */
        "%*" SCNu64 "\t"            /* expire_count */
        "%*" SCNu64 "\t"            /* active_since */
        "%*" SCNu64 "\t"            /* total_time */
        "%*" SCNu64 "\t"            /* max_time */
        "%"  SCNu64 "\t"            /* last_change */
        "%*" SCNu64 "\t";           /* prevent_suspend_time */

    int rc = sscanf(line, wup_src_str, wup->name, &wup->last_change);

    if (rc != 2) {
        fprintf(stderr, "Unmatched line (count %d)\n%s\n", rc, line);
        fprintf(stderr, "Match string %s\n", wup_src_str);
        return -1;
    }
    return 0;
}

static int parse_wakeup_sources(FILE *f)
{
    char buf[1024];
    char *ret;
    int rc;
    int count = 0;

    ret = fgets(buf, sizeof(buf), f);
    if (ret != buf && errno) {
        perror("fgets");
        return -errno;
    }
    rc = verify_header(buf);
    if (rc)
        return rc;

    struct wakeup_source wup;
    while (fgets(buf, sizeof(buf), f) == buf) {
        rc = parse_wakeup_source(buf, &wup);
        if (rc)
            return rc;
        printf("name %-12s\ttime %" PRIu64 "\n", wup.name, wup.last_change);
        count++;
    };
    if (ferror(f)) {
        perror("fgets");
        return -errno;
    }

    return count;
}
