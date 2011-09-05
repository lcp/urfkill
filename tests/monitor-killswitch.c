#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static GMainLoop *loop = NULL;

static const char *
type_to_name (UrfSwitchType type)
{
	switch (type) {
	case URFSWITCH_TYPE_WLAN:
		return "WLAN";
	case URFSWITCH_TYPE_BLUETOOTH:
		return "BLUETOOTH";
	case URFSWITCH_TYPE_UWB:
		return "UWB";
	case URFSWITCH_TYPE_WIMAX:
		return "WIMAX";
	case URFSWITCH_TYPE_WWAN:
		return "WWAN";
	case URFSWITCH_TYPE_GPS:
		return "GPS";
	case URFSWITCH_TYPE_FM:
		return "FM";
	default:
		return NULL;
	}
	return NULL;
}

static void
state_changed_cb (UrfKillswitch *killswitch, const int state)
{
	const char *type_name;
	type_name = type_to_name (urf_killswitch_get_switch_type (killswitch));

	g_print ("CHANGE Type %s state %d\n", type_name, state);
}

static void
show_state (UrfKillswitch *killswitch)
{
	const char *type_name;
	int state;

	type_name = type_to_name (urf_killswitch_get_switch_type (killswitch));
	g_object_get (killswitch,
		      "state", &state,
		      NULL);

	g_print ("Type %s state %d\n", type_name, state);
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
	UrfKillswitch *wlan = NULL;
	UrfKillswitch *bluetooth = NULL;
	UrfKillswitch *wwan = NULL;

	g_type_init();

	wlan = urf_killswitch_new (URFSWITCH_TYPE_WLAN);
	bluetooth = urf_killswitch_new (URFSWITCH_TYPE_BLUETOOTH);
	wwan = urf_killswitch_new (URFSWITCH_TYPE_WWAN);

	if (wlan) {
		show_state (wlan);
		g_signal_connect (wlan, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	}

	if (bluetooth) {
		show_state (bluetooth);
		g_signal_connect (bluetooth, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	}

	if (wwan) {
		show_state (wwan);
		g_signal_connect (wwan, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	}

	loop = g_main_loop_new (NULL, FALSE);

	signal (SIGINT, main_sigint_handler);

	g_main_loop_run (loop);

	g_object_unref (wlan);
	g_object_unref (bluetooth);
	g_object_unref (wwan);

	return 0;
}
