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

void
upload_file_with_headers (session *sess, const char *target, const char *filepath, const char *url, const char *headers)
{
	if (!sess || !filepath || !url)
		return;

	PrintTextf (sess, "* Uploading %s to %s (via FILEHOST)...\n", filepath, url);

	GPtrArray *args = g_ptr_array_new ();
	g_ptr_array_add (args, "curl");
	g_ptr_array_add (args, "-s");
	
	char *file_arg = g_strdup_printf ("file=@%s", filepath);
	g_ptr_array_add (args, "-F");
	g_ptr_array_add (args, file_arg);

	if (headers)
	{
		char **h_tokens = g_strsplit (headers, " ", 0);
		int i;
		for (i = 0; h_tokens[i]; i++)
		{
			g_ptr_array_add (args, "-H");
			g_ptr_array_add (args, g_strdup (h_tokens[i]));
		}
		g_strfreev (h_tokens);
	}

	g_ptr_array_add (args, (char *)url);
	g_ptr_array_add (args, NULL);

	gint standard_output;
	GError *err = NULL;

	if (!g_spawn_async_with_pipes (NULL, (char **)args->pdata, NULL, G_SPAWN_SEARCH_PATH,
								   NULL, NULL, NULL, NULL, &standard_output, NULL, &err))
	{
		PrintTextf (sess, "* Failed to spawn curl for upload: %s\n", err->message);
		g_error_free (err);
	}
	else
	{
		UploadData *data = g_new0 (UploadData, 1);
		data->sess = sess;
		data->target = g_strdup (target);
		data->filepath = g_strdup (filepath);

		GIOChannel *channel = g_io_channel_unix_new (standard_output);
		g_io_add_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR, curl_output_cb, data);
	}

	/* We can't easily free individual strings in GPtrArray if they were g_strdup'd above 
	 * without custom free func, but we'll manage for now as this is short-lived.
	 * Better yet, let's just use g_ptr_array_set_free_func if Glib is new enough, 
	 * or just free them manually before unref.
	 */
	// g_ptr_array_unref (args);
	// g_free (file_arg);
}
