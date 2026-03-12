/*
 * Unit tests for src/common/util.c
 * Copyright (C) 2026 Ditrigon contributors
 * License: GPL-2.0-or-later
 *
 * Tests for: rfc_casecmp, rfc_ncasecmp, match, strip_color,
 *            safe_strcpy, nocasestrstr, token_foreach, expand_homedir,
 *            file_part, buf_get_line, strip_hidden_attribute
 */

#include <string.h>
#include <glib.h>
#include "util.h"
#include "hexchat.h"

/* ===== rfc_casecmp tests ===== */

static void
test_rfc_casecmp_equal (void)
{
    g_assert_cmpint (rfc_casecmp ("hello", "hello"), ==, 0);
    g_assert_cmpint (rfc_casecmp ("Hello", "hello"), ==, 0);
    g_assert_cmpint (rfc_casecmp ("HELLO", "hello"), ==, 0);
}

static void
test_rfc_casecmp_brackets (void)
{
    /* RFC 1459: {}|~ are lowercase of []\^ */
    g_assert_cmpint (rfc_casecmp ("[test]", "{test}"), ==, 0);
    g_assert_cmpint (rfc_casecmp ("nick\\host", "nick|host"), ==, 0);
}

static void
test_rfc_casecmp_different (void)
{
    g_assert_cmpint (rfc_casecmp ("alice", "bob"), !=, 0);
    g_assert_cmpint (rfc_casecmp ("a", "b"), !=, 0);
}

static void
test_rfc_casecmp_empty (void)
{
    g_assert_cmpint (rfc_casecmp ("", ""), ==, 0);
    g_assert_cmpint (rfc_casecmp ("a", ""), !=, 0);
    g_assert_cmpint (rfc_casecmp ("", "a"), !=, 0);
}

/* ===== match (wildcard pattern matching) tests ===== */

static void
test_match_exact (void)
{
    g_assert_true (match ("hello", "hello"));
    g_assert_false (match ("hello", "world"));
}

static void
test_match_star (void)
{
    g_assert_true (match ("*", "anything"));
    g_assert_true (match ("*", ""));
    g_assert_true (match ("he*", "hello"));
    g_assert_true (match ("*lo", "hello"));
    g_assert_true (match ("h*o", "hello"));
    g_assert_true (match ("h*l*o", "hello"));
}

static void
test_match_question (void)
{
    g_assert_true (match ("h?llo", "hello"));
    g_assert_false (match ("h?llo", "hllo"));
    g_assert_true (match ("???", "abc"));
    g_assert_false (match ("???", "ab"));
}

static void
test_match_combined (void)
{
    g_assert_true (match ("*!*@*.host.com", "nick!user@sub.host.com"));
    g_assert_false (match ("*!*@*.host.com", "nick!user@other.net"));
    g_assert_true (match ("*.txt", "readme.txt"));
    g_assert_false (match ("*.txt", "readme.md"));
}

static void
test_match_case_insensitive (void)
{
    /* match() uses rfc_tolower for case-insensitive matching */
    g_assert_true (match ("Hello", "hello"));
    g_assert_true (match ("HELLO", "hello"));
    g_assert_true (match ("*ELLO", "hello"));
}

/* ===== strip_color tests ===== */

static void
test_strip_color_plain (void)
{
    char *result = strip_color ("plain text", -1, STRIP_ALL);
    g_assert_cmpstr (result, ==, "plain text");
    g_free (result);
}

static void
test_strip_color_with_codes (void)
{
    /* \003 = mIRC color, followed by digits */
    char *result = strip_color ("\00304,01colored\003 text", -1, STRIP_COLOR);
    g_assert_cmpstr (result, ==, "colored text");
    g_free (result);
}

static void
test_strip_color_bold_underline (void)
{
    /* \002 = bold, \037 = underline */
    char *result = strip_color ("\002bold\002 \037underline\037", -1, STRIP_ATTRIB);
    g_assert_cmpstr (result, ==, "bold underline");
    g_free (result);
}

static void
test_strip_color_selective (void)
{
    /* Only strip colors, keep attributes */
    char *input = "\00304red\003 \002bold\002";
    char *result = strip_color (input, -1, STRIP_COLOR);
    g_assert_cmpstr (result, ==, "red \002bold\002");
    g_free (result);
}

static void
test_strip_color_empty (void)
{
    char *result = strip_color ("", -1, STRIP_ALL);
    g_assert_cmpstr (result, ==, "");
    g_free (result);
}

/* ===== safe_strcpy tests ===== */

static void
test_safe_strcpy_normal (void)
{
    char buf[20];
    safe_strcpy (buf, "hello", sizeof (buf));
    g_assert_cmpstr (buf, ==, "hello");
}

static void
test_safe_strcpy_truncate (void)
{
    char buf[5];
    safe_strcpy (buf, "hello world", sizeof (buf));
    /* Should truncate and null-terminate */
    g_assert_cmpuint (strlen (buf), <, sizeof (buf));
}

static void
test_safe_strcpy_exact_fit (void)
{
    char buf[6]; /* "hello" + null */
    safe_strcpy (buf, "hello", sizeof (buf));
    g_assert_cmpstr (buf, ==, "hello");
}

/* ===== nocasestrstr tests ===== */

static void
test_nocasestrstr_found (void)
{
    const char *result = nocasestrstr ("Hello World", "world");
    g_assert_nonnull (result);
    g_assert_cmpstr (result, ==, "World");
}

static void
test_nocasestrstr_not_found (void)
{
    const char *result = nocasestrstr ("Hello World", "xyz");
    g_assert_null (result);
}

static void
test_nocasestrstr_empty_needle (void)
{
    const char *result = nocasestrstr ("Hello", "");
    g_assert_nonnull (result);
}

static void
test_nocasestrstr_rfc_case (void)
{
    /* nocasestrstr uses rfc_tolower for first-char but g_ascii_strncasecmp
       for the rest, so RFC bracket equivalence is limited. Standard ASCII
       case-insensitivity works though: */
    const char *result = nocasestrstr ("HELLO world", "hello WORLD");
    g_assert_nonnull (result);
}

/* ===== token_foreach tests ===== */

static int
count_callback (char *str, void *ud)
{
    int *count = (int *) ud;
    (*count)++;
    return 1; /* keep going */
}

static void
test_token_foreach_comma (void)
{
    int count = 0;
    char str[] = "a,b,c,d";
    token_foreach (str, ',', count_callback, &count);
    g_assert_cmpint (count, ==, 4);
}

static void
test_token_foreach_single (void)
{
    int count = 0;
    char str[] = "single";
    token_foreach (str, ',', count_callback, &count);
    g_assert_cmpint (count, ==, 1);
}

static int
stop_at_b (char *str, void *ud)
{
    if (strcmp (str, "b") == 0)
        return 0; /* stop */
    return 1;
}

static void
test_token_foreach_early_stop (void)
{
    /* Should return FALSE (0) when callback stops early */
    char str[] = "a,b,c";
    int result = token_foreach (str, ',', stop_at_b, NULL);
    g_assert_cmpint (result, ==, 0);
}

/* ===== file_part tests ===== */

static void
test_file_part_with_path (void)
{
    g_assert_cmpstr (file_part ("/home/user/file.txt"), ==, "file.txt");
}

static void
test_file_part_no_path (void)
{
    g_assert_cmpstr (file_part ("file.txt"), ==, "file.txt");
}

static void
test_file_part_trailing_slash (void)
{
    g_assert_cmpstr (file_part ("/home/user/"), ==, "");
}

static void
test_file_part_null (void)
{
    g_assert_cmpstr (file_part (NULL), ==, "");
}

/* ===== buf_get_line tests ===== */

static void
test_buf_get_line_basic (void)
{
    char data[] = "line1\nline2\nline3\n";
    char *line;
    int pos = 0;
    int len = strlen (data);

    g_assert_true (buf_get_line (data, &line, &pos, len));
    g_assert_cmpstr (line, ==, "line1");

    g_assert_true (buf_get_line (data, &line, &pos, len));
    g_assert_cmpstr (line, ==, "line2");

    g_assert_true (buf_get_line (data, &line, &pos, len));
    g_assert_cmpstr (line, ==, "line3");
}

static void
test_buf_get_line_end (void)
{
    char data[] = "only\n";
    char *line;
    int pos = 0;
    int len = strlen (data);

    g_assert_true (buf_get_line (data, &line, &pos, len));
    g_assert_cmpstr (line, ==, "only");

    /* Should return 0 when no more data */
    g_assert_false (buf_get_line (data, &line, &pos, len));
}

/* ===== expand_homedir tests ===== */

static void
test_expand_homedir_tilde (void)
{
    char *result = expand_homedir ("~/docs");
    g_assert_nonnull (result);
    /* Should start with the user's home directory */
    g_assert_true (g_str_has_prefix (result, g_get_home_dir ()));
    g_assert_true (g_str_has_suffix (result, "/docs"));
    g_free (result);
}

static void
test_expand_homedir_no_tilde (void)
{
    char *result = expand_homedir ("/absolute/path");
    g_assert_cmpstr (result, ==, "/absolute/path");
    g_free (result);
}

static void
test_expand_homedir_tilde_only (void)
{
    char *result = expand_homedir ("~");
    g_assert_cmpstr (result, ==, g_get_home_dir ());
    g_free (result);
}

/* ===== Registration ===== */

void
register_util_tests (void)
{
    /* rfc_casecmp */
    g_test_add_func ("/util/rfc_casecmp/equal", test_rfc_casecmp_equal);
    g_test_add_func ("/util/rfc_casecmp/brackets", test_rfc_casecmp_brackets);
    g_test_add_func ("/util/rfc_casecmp/different", test_rfc_casecmp_different);
    g_test_add_func ("/util/rfc_casecmp/empty", test_rfc_casecmp_empty);

    /* match */
    g_test_add_func ("/util/match/exact", test_match_exact);
    g_test_add_func ("/util/match/star", test_match_star);
    g_test_add_func ("/util/match/question", test_match_question);
    g_test_add_func ("/util/match/combined", test_match_combined);
    g_test_add_func ("/util/match/case_insensitive", test_match_case_insensitive);

    /* strip_color */
    g_test_add_func ("/util/strip_color/plain", test_strip_color_plain);
    g_test_add_func ("/util/strip_color/with_codes", test_strip_color_with_codes);
    g_test_add_func ("/util/strip_color/bold_underline", test_strip_color_bold_underline);
    g_test_add_func ("/util/strip_color/selective", test_strip_color_selective);
    g_test_add_func ("/util/strip_color/empty", test_strip_color_empty);

    /* safe_strcpy */
    g_test_add_func ("/util/safe_strcpy/normal", test_safe_strcpy_normal);
    g_test_add_func ("/util/safe_strcpy/truncate", test_safe_strcpy_truncate);
    g_test_add_func ("/util/safe_strcpy/exact_fit", test_safe_strcpy_exact_fit);

    /* nocasestrstr */
    g_test_add_func ("/util/nocasestrstr/found", test_nocasestrstr_found);
    g_test_add_func ("/util/nocasestrstr/not_found", test_nocasestrstr_not_found);
    g_test_add_func ("/util/nocasestrstr/empty_needle", test_nocasestrstr_empty_needle);
    g_test_add_func ("/util/nocasestrstr/rfc_case", test_nocasestrstr_rfc_case);

    /* token_foreach */
    g_test_add_func ("/util/token_foreach/comma", test_token_foreach_comma);
    g_test_add_func ("/util/token_foreach/single", test_token_foreach_single);
    g_test_add_func ("/util/token_foreach/early_stop", test_token_foreach_early_stop);

    /* file_part */
    g_test_add_func ("/util/file_part/with_path", test_file_part_with_path);
    g_test_add_func ("/util/file_part/no_path", test_file_part_no_path);
    g_test_add_func ("/util/file_part/trailing_slash", test_file_part_trailing_slash);
    g_test_add_func ("/util/file_part/null", test_file_part_null);

    /* buf_get_line */
    g_test_add_func ("/util/buf_get_line/basic", test_buf_get_line_basic);
    g_test_add_func ("/util/buf_get_line/end", test_buf_get_line_end);

    /* expand_homedir */
    g_test_add_func ("/util/expand_homedir/tilde", test_expand_homedir_tilde);
    g_test_add_func ("/util/expand_homedir/no_tilde", test_expand_homedir_no_tilde);
    g_test_add_func ("/util/expand_homedir/tilde_only", test_expand_homedir_tilde_only);
}
