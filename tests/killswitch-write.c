#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

int
main ()
{
	UrfKillswitch *wlan = NULL;

	g_type_init();

	wlan = urf_killswitch_new (URFSWITCH_TYPE_WLAN);

	g_object_set (wlan,
		      "state", URFSWITCH_STATE_SOFT_BLOCKED,
		      NULL);

	sleep (2);

	g_object_set (wlan,
		      "state", URFSWITCH_STATE_UNBLOCKED,
		      NULL);

	g_object_unref (wlan);

	return 0;
}
