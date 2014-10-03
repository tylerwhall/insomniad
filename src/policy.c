/* 
 * policy.c - Decide whether or not to go to sleep now
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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "common.h"
#include "parse_wakeup_sources.h"

#define TIMEOUT 10

static int sort_wakeup(struct wakeup_source *wup, void *data)
{
    struct wakeup_source *recent = data;

    if (recent->last_change < wup->last_change) {
        *recent = *wup;
    }

    return 0;
}

static uint64_t get_most_recent_event(void)
{
    int rc;
    struct wakeup_source wup;
    FILE *f = fopen("/sys/kernel/debug/wakeup_sources", "r");

    if (!f) {
        perror("Error opening wakeup_sources in debugfs");
        exit(1);
    }

    memset(&wup, 0, sizeof(wup));
    rc = parse_wakeup_sources(f, sort_wakeup, &wup);
    if (rc < 0) {
        printf("Error %d parsing wakeup sources\n", rc);
        exit(1);
    }

    fclose(f);

    return wup.last_change;
}

int evaluate_policy(unsigned int count)
{
    (void)count;
    uint64_t last_event = get_most_recent_event();

    pr_debug("Last wakeup event at %" PRIu64 "\n", last_event);
    sleep(TIMEOUT);

    return 1;
}
