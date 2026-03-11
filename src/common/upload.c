#include "hexchat.h"
#include "outbound.h"
#include "text.h"

#include <glib.h>

typedef struct {
    session *sess;
    char *target;
    char *filepath;
} UploadData;

static void
upload_data_free (UploadData *data)
{
    g_free (data->target);
    g_free (data->filepath);
    g_free (data);
}

static gboolean
curl_output_cb (GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    UploadData *data = user_data;
    session *sess = data->sess;
    
    char *str_return = NULL;
    gsize length;
    GError *err = NULL;

    if (condition & G_IO_IN)
    {
        GIOStatus status = g_io_channel_read_to_end (source, &str_return, &length, &err);
        if (status == G_IO_STATUS_NORMAL && str_return && length > 0)
        {
            char **lines = g_strsplit (str_return, "\n", 2);
            if (lines && lines[0]) 
            {
                char *clean_url = g_strstrip (g_strdup (lines[0]));
                if (g_str_has_prefix (clean_url, "http://") || g_str_has_prefix (clean_url, "https://")) 
                {
                    PrintTextf (sess, "* Upload completed: %s\n", clean_url);
                    if (data->target && *data->target)
                    {
                        char cmd[1024];
                        g_snprintf (cmd, sizeof(cmd), "MSG %s %s", data->target, clean_url);
                        handle_command (sess, cmd, FALSE);
                    }
                } else {
                    PrintTextf (sess, "* Upload returned unexpected data: %s\n", clean_url);
                }
                g_free (clean_url);
            }
            g_strfreev (lines);
        }
        else if (status == G_IO_STATUS_ERROR)
        {
            PrintTextf (sess, "* Upload failed (I/O error): %s\n", err->message);
            g_error_free (err);
        }
        g_free (str_return);
    }
    
    upload_data_free (data);
    g_io_channel_unref (source);
    return FALSE; /* Remove watch */
}

void
upload_file (session *sess, const char *target, const char *filepath)
{
    if (!sess || !filepath)
        return;

    char *upload_url = "https://0x0.st"; /* Default fallback */
    if (sess->server && sess->server->upload_url[0] != '\0')
        upload_url = sess->server->upload_url;

    PrintTextf (sess, "* Uploading %s to %s...\n", filepath, upload_url);

    char *file_arg = g_strdup_printf ("file=@%s", filepath);
    char *argv[] = {"curl", "-s", "-F", file_arg, upload_url, NULL};
    
    gint standard_output;
    GError *err = NULL;

    if (!g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, 
                                   NULL, NULL, NULL, NULL, &standard_output, NULL, &err))
    {
        PrintTextf (sess, "* Failed to spawn curl for upload: %s\n", err->message);
        g_error_free (err);
        g_free (file_arg);
        return;
    }

    UploadData *data = g_new0 (UploadData, 1);
    data->sess = sess;
    data->target = g_strdup (target);
    data->filepath = g_strdup (filepath);

    GIOChannel *channel = g_io_channel_unix_new (standard_output);
    g_io_add_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR, curl_output_cb, data);

    g_free (file_arg);
}
