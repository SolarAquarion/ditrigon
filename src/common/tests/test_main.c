/*
 * Core test suite - main entry point
 * Copyright (C) 2026 Ditrigon contributors
 * License: GPL-2.0-or-later
 */

#include <glib.h>

/* Declared in test_util.c */
void register_util_tests (void);
/* Declared in test_url.c */
void register_url_tests (void);

int
main (int argc, char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    register_util_tests ();
    register_url_tests ();

    return g_test_run ();
}
