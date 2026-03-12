#include "config.h"
#include "url_preview.h"
#include <string.h>

#ifdef USE_LIBSOUP
#include <libsoup/soup.h>

typedef struct {
    struct session *sess;
    URLPreviewCallback callback;
    gpointer user_data;
    char *url;
} SniffContext;

static SoupSession *session_soup = NULL;

static void
url_preview_data_init (URLPreviewData *data, const char *url)
{
    data->url = g_strdup (url);
    data->title = NULL;
    data->image_url = NULL;
    data->description = NULL;
    data->is_image = FALSE;
}

void
url_preview_data_free (URLPreviewData *data)
{
    if (!data) return;
    g_free (data->url);
    g_free (data->title);
    g_free (data->image_url);
    g_free (data->description);
    g_free (data);
}

static char *
extract_tag (const char *html, const char *tag_name)
{
    char *pattern = g_strdup_printf ("property=\"%s\" content=\"", tag_name);
    const char *start = strstr (html, pattern);
    char *ret = NULL;

    if (!start)
    {
        g_free (pattern);
        pattern = g_strdup_printf ("name=\"%s\" content=\"", tag_name);
        start = strstr (html, pattern);
    }

    if (start)
    {
        start += strlen (pattern);
        const char *end = strchr (start, '"');
        if (end)
        {
            ret = g_strndup (start, end - start);
        }
    }

    g_free (pattern);
    return ret;
}

static void
on_content_loaded (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    SoupSession *session = SOUP_SESSION (source_object);
    SoupMessage *msg = soup_session_get_async_result_message (session, res);
    GBytes *bytes;
    GError *error = NULL;
    
    SniffContext *ctx = user_data;

    bytes = soup_session_send_and_read_finish (session, res, &error);

    if (error)
    {
        g_error_free (error);
        g_free (ctx->url);
        g_free (ctx);
        return;
    }

    const char *content_type = soup_message_headers_get_content_type (soup_message_get_response_headers (msg), NULL);
    URLPreviewData *data = g_new0 (URLPreviewData, 1);
    url_preview_data_init (data, ctx->url);

    if (content_type && g_str_has_prefix (content_type, "image/"))
    {
        data->is_image = TRUE;
        data->image_url = g_strdup (ctx->url);
    }
    else if (content_type && g_str_has_prefix (content_type, "text/html"))
    {
        gsize size;
        const char *html = g_bytes_get_data (bytes, &size);
        if (html && size > 0)
        {
            data->title = extract_tag (html, "og:title");
            data->image_url = extract_tag (html, "og:image");
            data->description = extract_tag (html, "og:description");

            if (!data->title)
            {
                const char *t_start = strstr (html, "<title>");
                if (t_start)
                {
                    t_start += 7;
                    const char *t_end = strstr (t_start, "</title>");
                    if (t_end)
                        data->title = g_strndup (t_start, t_end - t_start);
                }
            }
        }
    }

    if (data->title || data->image_url || data->is_image)
    {
        ctx->callback (ctx->sess, data, ctx->user_data);
    }
    else
    {
        url_preview_data_free (data);
    }

    g_bytes_unref (bytes);
    g_free (ctx->url);
    g_free (ctx);
}

void
url_preview_sniff (struct session *sess, const char *url, URLPreviewCallback callback, gpointer user_data)
{
    if (!session_soup)
    {
        session_soup = soup_session_new_with_options ("user-agent", "Ditrigon IRC", NULL);
    }

    SoupMessage *msg = soup_message_new ("GET", url);
    if (!msg) return;

    SniffContext *ctx = g_new0 (SniffContext, 1);
    ctx->sess = sess;
    ctx->callback = callback;
    ctx->user_data = user_data;
    ctx->url = g_strdup (url);

    soup_session_send_and_read_async (session_soup, msg, G_PRIORITY_DEFAULT, NULL, on_content_loaded, ctx);
    g_object_unref (msg);
}
#else
void url_preview_sniff (struct session *sess, const char *url, URLPreviewCallback callback, gpointer user_data) {}
void url_preview_data_free (URLPreviewData *data) {}
#endif
