/* 
 * check_parse_wakeup_sources.c - Tests for parsing wakeup_sources file
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

#include "parse_wakeup_sources.c"

#include <stdio.h>
#include <stdlib.h>
#include <check.h>

/* Enable all logging for tests */
int info = 1;
int debug = 1;

START_TEST(test_parse_wakeup_source)
{
    struct wakeup_source wup;
    int rc;

    rc = parse_wakeup_source("source\t0\t0\t0\t0\t0\t0\t0\t500\t0\n", &wup);
    ck_assert(!rc);
    ck_assert_str_eq(wup.name, "source");
    ck_assert_int_eq(wup.last_change, 500);
    destroy_wakeup_source(&wup);
}
END_TEST

static int increment_count(struct wakeup_source *wup, void *data)
{
    int *count = data;

    ck_assert(wup != NULL);
    (*count)++;

    return 0;
}

START_TEST(test_parse_wakeup_sources_callback)
    int count = 0;
    FILE *f = fopen(TCASE_DIR "wakeup_sources", "r");

    ck_assert_msg(f != NULL, "Failed to open test case.");

    /* Iterator function should be called once for each source */
    parse_wakeup_sources(f, increment_count, &count);
    ck_assert_int_eq(13, count);

    fclose(f);
END_TEST

START_TEST(test_parse_wakeup_sources)
    FILE *f = fopen(TCASE_DIR "wakeup_sources", "r");

    ck_assert_msg(f != NULL, "Failed to open test case.");

    /* Return value should be the number of sources in the file */
    ck_assert_int_eq(13, parse_wakeup_sources(f, NULL, NULL));

    fclose(f);
END_TEST

int main(void)
{
    Suite *s = suite_create("parse wakeup sources");
    TCase *tc = tcase_create("core");
    SRunner *sr;
    int failed;

    tcase_add_test(tc, test_parse_wakeup_source);
    tcase_add_test(tc, test_parse_wakeup_sources);
    tcase_add_test(tc, test_parse_wakeup_sources_callback);
    suite_add_tcase(s, tc);

    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);
    failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
