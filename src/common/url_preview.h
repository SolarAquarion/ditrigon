#ifndef HEXCHAT_URL_PREVIEW_H
#define HEXCHAT_URL_PREVIEW_H

#include <glib.h>
#include "hexchat.h"

typedef struct {
    char *url;
    char *title;
    char *image_url;
    char *description;
    gboolean is_image;
} URLPreviewData;

typedef void (*URLPreviewCallback) (session *sess, URLPreviewData *preview, gpointer user_data);

void url_preview_sniff (session *sess, const char *url, URLPreviewCallback callback, gpointer user_data);

void url_preview_data_free (URLPreviewData *data);

#endif
