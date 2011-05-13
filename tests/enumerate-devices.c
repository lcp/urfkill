#include <time.h>
#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

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

int
main ()
{
	UrfClient *client = NULL;
	UrfDevice *item;
	char *daemon_version;
	gboolean status;
	GPtrArray *devices;
	guint i;

	g_type_init();

	client = urf_client_new ();

	g_print ("Daemon Version: %s\n\n", urf_client_get_daemon_version (client));

	devices = urf_client_get_devices (client);

	for (i = 0; i<devices->len; i++) {
		item = (UrfDevice *)g_ptr_array_index (devices, i);
		print_urf_device (item);
		printf ("\n");
	}

	g_object_unref (client);

	return 0;
}
