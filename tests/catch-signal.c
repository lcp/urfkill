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
rfkill_added_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== Added ==\n");
	print_urf_device (device);
	printf ("\n");
}

static void
rfkill_removed_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== removed ==\n");
	print_urf_device (device);
	printf ("\n");
}

static void
rfkill_changed_cb (UrfClient *client, UrfDevice *device, gpointer data)
{
	printf ("== Changed ==\n");
	print_urf_device (device);
	printf ("\n");
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

	g_signal_connect (client, "rfkill-added", G_CALLBACK (rfkill_added_cb), NULL);
	g_signal_connect (client, "rfkill-removed", G_CALLBACK (rfkill_removed_cb), NULL);
	g_signal_connect (client, "rfkill-changed", G_CALLBACK (rfkill_changed_cb), NULL);

	loop = g_main_loop_new (NULL, FALSE);

	g_main_loop_run (loop);

	g_object_unref (client);

	return 0;
}
