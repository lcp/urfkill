#include <time.h>
#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static GMainLoop *loop = NULL;

static void
print_urf_device (UrfDevice *device)
{
	guint index, type;
	gboolean soft, hard;
	char *name;

	g_object_get (device,
		      "index", &index,
		      "type", &type,
		      "soft", &soft,
		      "hard", &hard,
		      "name", &name,
		      NULL);

	printf ("index = %u\n", index);
	printf ("type  = %u\n", type);
	printf ("soft  = %d\n", soft);
	printf ("hard  = %d\n", hard);
	printf ("name  = %s\n", name);
}

static void
device_added_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== Added ==\n");
	print_urf_device (device);
	printf ("\n");
}

static void
device_removed_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== removed ==\n");
	print_urf_device (device);
	printf ("\n");
}

static void
device_changed_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== Changed ==\n");
	print_urf_device (device);
	printf ("\n");
}

static void
key_pressed_cb (UrfClient *client, const int keycode, gpointer data)
{
	printf ("\n*** Key %d pressed ***\n", keycode);
}

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

	g_type_init();

	client = urf_client_new ();

	g_signal_connect (client, "device-added", G_CALLBACK (device_added_cb), NULL);
	g_signal_connect (client, "device-removed", G_CALLBACK (device_removed_cb), NULL);
	g_signal_connect (client, "device-changed", G_CALLBACK (device_changed_cb), NULL);
	g_signal_connect (client, "rf-key-pressed", G_CALLBACK (key_pressed_cb), NULL);

	loop = g_main_loop_new (NULL, FALSE);

	g_main_loop_run (loop);

	g_object_unref (client);

	return 0;
}
