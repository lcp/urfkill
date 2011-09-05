#include <time.h>
#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

#define INHIBIT_SECONDS 60

int
main ()
{
	UrfClient *client = NULL;
	guint cookie;

	g_type_init();

	client = urf_client_new ();

	g_print ("Daemon Version: %s\n\n", urf_client_get_daemon_version (client));

	cookie = urf_client_inhibit (client, "Just a test", NULL);
	g_print ("inhibit for %d seconds\n", INHIBIT_SECONDS);

	sleep (INHIBIT_SECONDS);

	urf_client_uninhibit (client, cookie);
	g_print ("uninhibit\n");

	g_object_unref (client);

	return 0;
}
