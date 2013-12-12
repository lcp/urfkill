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
	UrfKillswitch *wlan = NULL;

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif

	wlan = urf_killswitch_new (URF_ENUM_TYPE_WLAN);

	g_object_set (wlan,
		      "state", URF_ENUM_STATE_SOFT_BLOCKED,
		      NULL);

	sleep (2);

	g_object_set (wlan,
		      "state", URF_ENUM_STATE_UNBLOCKED,
		      NULL);

	loop = g_main_loop_new (NULL, FALSE);

	signal (SIGINT, main_sigint_handler);

	g_main_loop_run (loop);

	g_object_unref (wlan);

	return 0;
}
