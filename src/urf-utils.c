#include "urf-utils.h"

/**
 * get_rfkill_name:
 **/
char *
get_rfkill_name_by_index (guint index)
{
	char *filename;
	char *content;
	gsize length;
	gboolean ret;
	GError *error = NULL;

	filename = g_strdup_printf ("/sys/class/rfkill/rfkill%u/name", index);

	ret = g_file_get_contents(filename, &content, &length, &error);
	g_free (filename);

	if (!ret) {
		g_warning ("Get rfkill name: %s", error->message);
		return NULL;
	}

	g_strstrip (content);
	return content;
}
