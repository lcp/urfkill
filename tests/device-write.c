#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static GMainLoop *loop = NULL;

static void
main_sigint_handler (gint sig)
{
	signal (SIGINT, SIG_DFL);
	g_main_loop_quit (loop);
}

int
main ()
{
	UrfClient *client = NULL;
	UrfDevice *device;
	GList *devices, *item;
	gboolean soft;

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif

	client = urf_client_new ();
	urf_client_enumerate_devices_sync (client, NULL, NULL);
	devices = urf_client_get_devices (client);

	if (!devices)
		goto out;

	item = devices;
	device = (UrfDevice *)item->data;
	g_object_get (device,
		      "soft", &soft,
		      NULL);

	g_object_set (device,
		      "soft", !soft,
		      NULL);

	sleep (2);

	g_object_set (device,
		      "soft", soft,
		      NULL);

	loop = g_main_loop_new (NULL, FALSE);

	signal (SIGINT, main_sigint_handler);

	g_main_loop_run (loop);

out:
	g_object_unref (client);

	return 0;
}
