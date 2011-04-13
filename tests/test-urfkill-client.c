#include <time.h>
#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static GMainLoop *loop = NULL;

static void
print_urf_killswitch (UrfKillswitch *killswitch)
{
	printf ("index = %u\n", urf_killswitch_get_rfkill_index (killswitch));
	printf ("type  = %u\n", urf_killswitch_get_rfkill_type (killswitch));
	printf ("state = %d\n", urf_killswitch_get_rfkill_state (killswitch));
	printf ("soft  = %d\n", urf_killswitch_get_rfkill_soft (killswitch));
	printf ("hard  = %d\n", urf_killswitch_get_rfkill_hard (killswitch));
	printf ("name  = %s\n", urf_killswitch_get_rfkill_name (killswitch));
}

static void
rfkill_added_cb (UrfClient *client, UrfKillswitch *killswitch, gpointer data)
{
	printf ("== Added ==\n");
	print_urf_killswitch (killswitch);
	printf ("\n");
}

static void
rfkill_removed_cb (UrfClient *client, UrfKillswitch *killswitch, gpointer data)
{
	printf ("== removed ==\n");
	print_urf_killswitch (killswitch);
	printf ("\n");

	g_object_unref (killswitch);
}

static void
rfkill_changed_cb (UrfClient *client, UrfKillswitch *killswitch, gpointer data)
{
	printf ("== Changed ==\n");
	print_urf_killswitch (killswitch);
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
	gboolean status;
	GPtrArray *killswitches;
	guint i;
	UrfKillswitch *item;

	g_type_init();

	client = urf_client_new ();
	status = urf_client_set_wlan_block (client);
	printf ("Status of block: %d\n", status);

	sleep (2);

	status = urf_client_set_wlan_unblock (client);
	printf ("Status of unblock: %d\n", status);

	status = urf_client_key_control_enabled (client, NULL);
	printf ("Key control is %s\n", status?"on":"off");

	sleep (2);

	status = urf_client_enable_key_control (client, FALSE, NULL);
	if (!status)
		printf ("failed to disable key control\n");
	else {
		status = urf_client_key_control_enabled (client, NULL);
		printf ("Key control is %s\n", status?"on":"off");
	}

	sleep (2);

	status = urf_client_enable_key_control (client, TRUE, NULL);
	if (!status)
		printf ("failed to enable key control\n");
	else {
		status = urf_client_key_control_enabled (client, NULL);
		printf ("Key control is %s\n", status?"on":"off");
	}

	killswitches = urf_client_get_killswitches (client);

	for (i = 0; i<killswitches->len; i++) {
		item = (UrfKillswitch *)g_ptr_array_index (killswitches, i);
		print_urf_killswitch (item);
		printf ("\n");
	}

	item = urf_client_get_killswitch (client, 1, NULL, NULL);

	printf ("== GetKillswitch ==\n");
	print_urf_killswitch (item);
	printf ("\n");

	g_signal_connect (client, "rfkill-added", G_CALLBACK (rfkill_added_cb), NULL);
	g_signal_connect (client, "rfkill-removed", G_CALLBACK (rfkill_removed_cb), NULL);
	g_signal_connect (client, "rfkill-changed", G_CALLBACK (rfkill_changed_cb), NULL);

	loop = g_main_loop_new (NULL, FALSE);

	g_main_loop_run (loop);

	g_object_unref (client);

	return 0;
}
