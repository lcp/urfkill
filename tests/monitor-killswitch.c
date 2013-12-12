#include <urfkill.h>
#include <stdio.h>
#include <glib.h>

static GMainLoop *loop = NULL;

static const char *
type_to_name (UrfEnumType type)
{
	switch (type) {
	case URF_ENUM_TYPE_WLAN:
		return "WLAN";
	case URF_ENUM_TYPE_BLUETOOTH:
		return "BLUETOOTH";
	case URF_ENUM_TYPE_UWB:
		return "UWB";
	case URF_ENUM_TYPE_WIMAX:
		return "WIMAX";
	case URF_ENUM_TYPE_WWAN:
		return "WWAN";
	case URF_ENUM_TYPE_GPS:
		return "GPS";
	case URF_ENUM_TYPE_FM:
		return "FM";
	case URF_ENUM_TYPE_NFC:
		return "NFC";
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

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init();
#endif

	wlan = urf_killswitch_new (URF_ENUM_TYPE_WLAN);
	bluetooth = urf_killswitch_new (URF_ENUM_TYPE_BLUETOOTH);
	wwan = urf_killswitch_new (URF_ENUM_TYPE_WWAN);

	if (wlan) {
		show_state (wlan);
		g_signal_connect (wlan, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	} else {
		g_warning ("Failed to new wlan");
	}

	if (bluetooth) {
		show_state (bluetooth);
		g_signal_connect (bluetooth, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	} else {
		g_warning ("Failed to new bluetooth");
	}

	if (wwan) {
		show_state (wwan);
		g_signal_connect (wwan, "state-changed", G_CALLBACK (state_changed_cb), NULL);
	} else {
		g_warning ("Failed to new wwan");
	}

	loop = g_main_loop_new (NULL, FALSE);

	signal (SIGINT, main_sigint_handler);

	g_main_loop_run (loop);

	if (wlan)
		g_object_unref (wlan);
	if (bluetooth)
		g_object_unref (bluetooth);
	if (wwan)
		g_object_unref (wwan);

	return 0;
}
