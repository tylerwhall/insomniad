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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "common.h"
#include "policy.h"

/* Options */
static int dry_run = 0;
unsigned long hysteresis_ms = 10*1000;
int info = 0;
int debug = 0;

struct insomniad_ctx {
    int state_fd;
    int wakeup_count_fd;
    char wakeup_count[32]; /* More than enough to hold the %u format from the kernel */
};

static void insomniad_init(struct insomniad_ctx *ctx)
{
    /* Necessary to flush logging to systemd journal in a timely manner */
    setlinebuf(stdout);
    setlinebuf(stderr);

    ctx->state_fd = open("/sys/power/state", O_WRONLY, O_CLOEXEC);
    if (ctx->state_fd == -1) {
        perror("Error opening power state");
        exit(1);
    }
    ctx->wakeup_count_fd = -1;
}

static void usage(void)
{
    fprintf(stderr, "insomniad [OPTION...]\n"
           "    -n      dry run. Emit message instead of suspending\n"
           "    -t      hysteresis time in ms. Defaults to 10000 (10 seconds)\n"
           "            Will not sleep until at least this much time has elapsed since the last wakeup event\n"
           "    -v      Enable verbose message\n"
           "    -d      Enable debug messages (implies -v)\n"
           );
    exit(1);
}

static void get_opts(int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "hnt:vd")) != -1) {
        switch (c) {
        case 'n':
            dry_run = 1;
            break;
        case 't': {
            char *endptr;
            hysteresis_ms = strtoul(optarg, &endptr, 0);
            if (endptr == optarg || (hysteresis_ms == ULONG_MAX && errno == ERANGE)) {
                fprintf(stderr, "Error interpreting %s as integer ms\n", optarg);
                usage();
            }
            break;
        }
        case 'd':
            debug = 1;
            /* Fall through */
        case 'v':
            info = 1;
            break;
        case 'h':
        default:
            usage();
        }
    }
}

static int go_to_sleep(struct insomniad_ctx *ctx)
{
    int rc = 0;

    if (!dry_run) {
        rc = write(ctx->state_fd, "mem", 3);
        if (rc == -1) {
            if (errno == EBUSY) {
                /* EBUSY is acceptable */
                rc = -errno;
            } else {
                perror("Failed to write to power state file");
                exit(1);
            }
        } else {
            rc = 0;
        }
    } else {
        pr_notice("Would have attempted sleep\n");
        /* Prevent busy spin */
        sleep(1);
    }

    return rc;
}

static void open_wakeup_count(struct insomniad_ctx *ctx)
{
    assert(ctx->wakeup_count_fd == -1);
    ctx->wakeup_count_fd = open("/sys/power/wakeup_count", O_RDWR, O_CLOEXEC);
    if (!ctx->wakeup_count_fd) {
        perror("Error opening wakeup_count");
        exit(1);
    }
}

static void read_wakeup_count(struct insomniad_ctx *ctx)
{
    int rc;

    open_wakeup_count(ctx);
    memset(&ctx->wakeup_count, 0, sizeof(ctx->wakeup_count));
    rc = read(ctx->wakeup_count_fd, &ctx->wakeup_count, sizeof(ctx->wakeup_count));
    if (rc == -1) {
        perror("Error reading wakeup_count");
        exit(1);
    }
}

static int write_wakeup_count(struct insomniad_ctx *ctx)
{
    int rc;

    assert(ctx->wakeup_count_fd != -1);
    rc = write(ctx->wakeup_count_fd, ctx->wakeup_count, strlen(ctx->wakeup_count));
    if (rc == -1) {
        if (errno != EINVAL && errno != EBUSY) {
            perror("Unexpected error from wakeup_count");
            exit(1);
        }
    } else {
        rc = 0;
    }

    close(ctx->wakeup_count_fd);
    ctx->wakeup_count_fd = -1;

    return rc;
}

int main(int argc, char *argv[])
{
    struct insomniad_ctx ctx;

    get_opts(argc, argv);

    insomniad_init(&ctx);
    pr_info("Using hysteresis time of %lu ms\n", hysteresis_ms);

    while (1) {
        int rc;

        /* Blocks until no wakeup sources are active */
        read_wakeup_count(&ctx);

        /* Allow policy to futher delay */
        evaluate_policy();

        rc = write_wakeup_count(&ctx);
        if (rc) {
            /* This will fail when an event happens between read() and
             * write(). We raced with a wakeup event, so start over. */
            pr_info("Event occurred since reading wakeup_count\n");
            continue;
        }

        /* write successful. Going down now. */
        pr_info("Going to sleep\n");
        rc = go_to_sleep(&ctx);
        if (rc) {
            /* Non-fatal error. Try again. */
            pr_info("Non-fatal error %s writing to suspend state\n", strerror(-rc));
            usleep(1000);
            continue;
        }
        pr_info("Exited sleep\n");

        /*
         * Sleep for at least the requested timeout after wake, emulating a
         * wakeup source on resume. Normally this would be taken care of by the
         * kernel generating a wakeup event on resume and we would make sure
         * not to sleep until it is released + our timeout. This makes sure our
         * timeout is applied to spurious wakeups as well.
         */
        usleep(hysteresis_ms * 1000);
    };
    return 0;
}
