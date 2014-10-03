/* 
 * insomniad - user space policy for Linux's autosleep mechanism
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"
#include "policy.h"

/* Options */
static int dry_run = 0;

struct insomniad_ctx {
    int state_fd;
};

static void insomniad_init(struct insomniad_ctx *ctx)
{
    ctx->state_fd = open("/sys/power/state", O_WRONLY, O_CLOEXEC);
    if (ctx->state_fd == -1) {
        perror("Error power state");
        exit(1);
    }
}

static void usage(void)
{
    printf("insomniad [OPTION...]\n"
           "    -n      dry run. Emit message instead of suspending\n"
           );
    exit(1);
}

static void get_opts(int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "hn")) != -1) {
        switch (c) {
        case 'n':
            dry_run = 1;
            break;
        case 'h':
        default:
            usage();
        }
    }
}

static void go_to_sleep(struct insomniad_ctx *ctx)
{
    if (!dry_run) {
        int rc = write(ctx->state_fd, "mem", 3);
        if (rc == -1) {
            perror("Failed to write to power state file");
            exit(1);
        }
    } else {
        pr_notice("Would have attempted sleep\n");
        /* Prevent busy spin */
        sleep(1);
    }
}

static FILE *open_wakeup_count(void)
{
    FILE *f = fopen("/sys/power/wakeup_count", "r+");
    if (!f) {
        perror("Error opening wakeup_count");
        exit(1);
    }
    return f;
}

static unsigned int read_wakeup_count(FILE *wakeup_count)
{
    unsigned int count;
    int rc;

    rc = fscanf(wakeup_count, "%u", &count);
    if (rc != 1 || rc == EOF) {
        fprintf(stderr, "Error reading wakeup_count");
        exit(1);
    }

    return count;
}

static int write_wakeup_count(FILE *wakeup_count, unsigned count)
{
    int rc = fprintf(wakeup_count, "%u", count);

    if (rc == -1) {
        if (errno != EINVAL && errno != EBUSY) {
            perror("Unexpected error from wakeup_count");
            exit(1);
        }
    }

    return (rc == -1) ? -1 : 0;
}

int main(int argc, char *argv[])
{
    struct insomniad_ctx ctx;

    get_opts(argc, argv);

    insomniad_init(&ctx);

    while (1) {
        int rc;
        unsigned int count;
        FILE *wakeup_file = open_wakeup_count();

        count = read_wakeup_count(wakeup_file);
        pr_debug("wakeup_count = %u\n", count);

        if (!evaluate_policy(count)) {
            fclose(wakeup_file);
            continue;
        }

        rc = write_wakeup_count(wakeup_file, count);
        fclose(wakeup_file);
        if (rc) {
            /* This will fail when an event happens between read() and
             * write(). We raced with a wakeup event, so start over. */
            continue;
        }

        /* write successful. Going down now. */
        go_to_sleep(&ctx);
    };
    return 0;
}
