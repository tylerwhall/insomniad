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
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "common.h"
#include "parse_wakeup_sources.h"
#include "policy.h"

static int sort_wakeup(struct wakeup_source *wup, void *data)
{
    struct wakeup_source **recent = data;

    if (*recent != NULL && (*recent)->last_change >= wup->last_change)
        return 0;

    if (*recent)
        wakeup_source_unref(*recent);
    wakeup_source_ref(wup);
    *recent = wup;
    return 0;
}

static struct wakeup_source *get_most_recent_event(void)
{
    int rc;
    struct wakeup_source *wup = NULL;
    FILE *f = fopen("/sys/kernel/debug/wakeup_sources", "r");

    if (!f) {
        perror("Error opening wakeup_sources in debugfs");
        exit(1);
    }

    rc = parse_wakeup_sources(f, sort_wakeup, &wup);
    if (rc < 0) {
        printf("Error %d parsing wakeup sources\n", rc);
        exit(1);
    }

    fclose(f);

    pr_info("Last wakeup event %s\n", wup->name);

    return wup;
}

/*
 * Sleep for the requested time, restarting if interrupted. Restarting is only
 * necessary if interrupted by a signal that is configured to invoke a
 * signal-catching function.
 */
int usleep_signal_safe(useconds_t usec)
{
    struct timespec ts;

    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = usec / 1000 / 1000;
    ts.tv_nsec = (usec % (1000 * 1000)) * 1000;
    return TEMP_FAILURE_RETRY(nanosleep(&ts, &ts));
}

static uint64_t get_time_ms(void)
{
    struct timespec tp;

    int rc = clock_gettime(CLOCK_MONOTONIC, &tp);
    if (rc) {
        perror("clock gettime");
        exit (1);
    }

    uint64_t time = tp.tv_sec;
    time *= 1000;
    time += tp.tv_nsec / 1000 / 1000;
    return time;
}

int evaluate_policy(void)
{
    struct wakeup_source *wup = get_most_recent_event();
    uint64_t current_time = get_time_ms();
    uint64_t delta;

    if (!wup)
        return -ENOMEM;
    assert(current_time >= wup->last_change);

    delta = current_time - wup->last_change;
    pr_info("Last wakeup event at %" PRIu64 "ms\n", wup->last_change);
    pr_info("Current time         %" PRIu64 "ms\n", current_time);
    pr_info("Delta                %" PRIu64 "ms\n", delta);

    wakeup_source_unref(wup);

    if (delta < hysteresis_ms) {
        uint64_t delay = hysteresis_ms - delta;

        pr_info("Delaying for         %" PRIu64 "ms\n", delay);
        usleep_signal_safe(delay * 1000);
    }

    return 1;
}
