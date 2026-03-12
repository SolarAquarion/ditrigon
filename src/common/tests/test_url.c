/*
 * Unit tests for src/common/url.c
 * Copyright (C) 2026 Ditrigon contributors
 * License: GPL-2.0-or-later
 *
 * Tests for: url_check_word (URL matching only)
 *
 * Note: Tests for IRC channels, nicks, and email are omitted because
 * url_check_word's match_channel/match_nick functions dereference
 * current_sess->server, which requires a full session context.
 */

#include <glib.h>
#include "url.h"

/* ===== url_check_word tests (URL types that don't need session) ===== */

static void
test_url_https (void)
{
    g_assert_cmpint (url_check_word ("https://example.com"), ==, WORD_URL);
}

static void
test_url_http (void)
{
    g_assert_cmpint (url_check_word ("http://example.com"), ==, WORD_URL);
}

static void
test_url_ftp (void)
{
    g_assert_cmpint (url_check_word ("ftp://files.example.com"), ==, WORD_URL);
}

static void
test_url_irc_scheme (void)
{
    g_assert_cmpint (url_check_word ("irc://irc.libera.chat/channel"), ==, WORD_URL);
}

static void
test_url_with_path (void)
{
    g_assert_cmpint (url_check_word ("https://example.com/path/to/page"), ==, WORD_URL);
}

static void
test_url_with_query (void)
{
    g_assert_cmpint (url_check_word ("https://example.com/search?q=test"), ==, WORD_URL);
}

static void
test_url_with_fragment (void)
{
    g_assert_cmpint (url_check_word ("https://example.com/page#section"), ==, WORD_URL);
}

static void
test_url_with_port (void)
{
    g_assert_cmpint (url_check_word ("http://localhost:8080/path"), ==, WORD_URL);
}

static void
test_url_ssh (void)
{
    g_assert_cmpint (url_check_word ("ssh://user@host.com"), ==, WORD_URL);
}

static void
test_url_magnet (void)
{
    g_assert_cmpint (url_check_word ("magnet:?xt=urn:btih:abc123"), ==, WORD_URL);
}

/* ===== Registration ===== */

void
register_url_tests (void)
{
    g_test_add_func ("/url/check_word/https", test_url_https);
    g_test_add_func ("/url/check_word/http", test_url_http);
    g_test_add_func ("/url/check_word/ftp", test_url_ftp);
    g_test_add_func ("/url/check_word/irc_scheme", test_url_irc_scheme);
    g_test_add_func ("/url/check_word/with_path", test_url_with_path);
    g_test_add_func ("/url/check_word/with_query", test_url_with_query);
    g_test_add_func ("/url/check_word/with_fragment", test_url_with_fragment);
    g_test_add_func ("/url/check_word/with_port", test_url_with_port);
    g_test_add_func ("/url/check_word/ssh", test_url_ssh);
    g_test_add_func ("/url/check_word/magnet", test_url_magnet);
}
