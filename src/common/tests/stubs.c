/*
 * Stubs for symbols needed by url.c and util.c that are defined
 * in other parts of the application (fe-gtk4, hexchat.c, cfgfiles.c).
 * These minimal implementations allow the functions under test
 * (url_check_word, strip_color, etc.) to link without pulling in
 * the entire application.
 */

#include <stdio.h>
#include <glib.h>
#include "hexchat.h"

/* Global prefs (needed by url.c url_add for grabber_limit) */
struct hexchatprefs prefs = {0};

/* Stub for fe_url_add (called by url_add, not by url_check_word) */
void fe_url_add (const char *url) { (void)url; }

/* Stub for hexchat_fopen_file (called by url_save_tree) */
FILE *hexchat_fopen_file (const char *file, const char *mode, int xof_flags)
{
    (void)file; (void)mode; (void)xof_flags;
    return NULL;
}

/* Stub for hexchat_open_file (may be referenced) */
int hexchat_open_file (const char *file, int flags, int mode, int xof_flags)
{
    (void)file; (void)flags; (void)mode; (void)xof_flags;
    return -1;
}

/* Stubs for hexchatc.h globals */
GSList *serv_list = NULL;
GSList *sess_list = NULL;
int xchat_is_quitting = 0;

/* Stub for current_sess (used by url.c match_nick/match_channel) */
session *current_sess = NULL;

/* Stub for userlist_find (used by url.c match_nick) */
struct User *userlist_find (struct session *sess, const char *name)
{
    (void)sess; (void)name;
    return NULL;
}
