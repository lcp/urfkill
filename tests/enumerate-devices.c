#include <time.h>
#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static void
print_urf_device (UrfDevice *device)
{
	guint index, type;
	gboolean soft, hard, platform;
	char *name;

	g_object_get (device,
		      "index", &index,
		      "type", &type,
		      "soft", &soft,
		      "hard", &hard,
		      "name", &name,
		      "platform", &platform,
		      NULL);

	printf ("index = %u\n", index);
	printf ("type  = %u\n", type);
	printf ("soft  = %d\n", soft);
	printf ("hard  = %d\n", hard);
	printf ("name  = %s\n", name);
	printf ("platform = %d\n", platform);
}

int
main ()
{
	UrfClient *client = NULL;
	UrfDevice *device;
	GList *devices, *item;

	g_type_init();

	client = urf_client_new ();
	urf_client_enumerate_devices_sync (client, NULL, NULL);

	g_print ("Daemon Version: %s\n\n", urf_client_get_daemon_version (client));

	devices = urf_client_get_devices (client);

	for (item = devices; item; item = item->next) {
		device = (UrfDevice *)item->data;
		print_urf_device (device);
		printf ("\n");
	}

	g_object_unref (client);

	return 0;
}
