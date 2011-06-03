const GLib = imports.gi.GLib;
const Lang = imports.lang;
const Urfkill = imports.gi.Urfkill;
const Mainloop = imports.mainloop;

const UrfDeviceType = {
    ALL       : 0,
    WLAN      : 1,
    BLUETOOTH : 2,
    UWB       : 3,
    WIMAX     : 4,
    WWAN      : 5,
    GPS       : 6,
    FM        : 7
};

function deviceTypeToString (type) {
  switch (type) {
    case UrfDeviceType.ALL:
      return "ALL";
    case UrfDeviceType.WLAN:
      return "WLAN";
    case UrfDeviceType.BLUETOOTH:
      return "BLUETOOTH";
    case UrfDeviceType.UWB:
      return "UWB";
    case UrfDeviceType.WIMAX:
      return "WIMAX";
    case UrfDeviceType.WWAN:
      return "WWAN";
    case UrfDeviceType.GPS:
      return "GPS";
    case UrfDeviceType.FM:
      return "FM";
    default:
      return "Unknown";
  }
}

function printDevice (device) {
  object_path = device.get_object_path ();
  print ("   Object Path:", object_path);
  print ("   Index:", device.index);
  print ("   Type:", deviceTypeToString (device.type));
  print ("   Name:", device.name);
  print ("   Soft:", device.soft);
  print ("   Hard:", device.hard);
}

function DeviceQuery () {
  this._init ();
}

DeviceQuery.prototype = {
  _init: function () {
    this._urfClient = new Urfkill.Client ();
    this._devices = this._urfClient.get_devices ();
    this._cookie = 0;
    this._urfClient.connect ("device-added", Lang.bind (this, this._device_added));
    this._urfClient.connect ("device-removed", Lang.bind (this, this._device_removed));
    this._urfClient.connect ("device-changed", Lang.bind (this, this._device_changed));
    this._urfClient.connect ("rf-key-pressed", Lang.bind (this, this._key_pressed));
  },

  _device_added : function (client, device, data) {
    print ("\nDevice added");
    printDevice (device);
  },

  _device_removed : function (client, device, data) {
    print ("\nDevice removed");
    printDevice (device);
  },

  _device_changed : function (client, device, data) {
    print ("\nDevice changed");
    printDevice (device);
  },

  _key_pressed : function (client, keycode, data) {
    print ("\n*** key", keycode, "is pressed ***");
  },

  inhibit : function () {
    this._cookie = this._urfClient.inhibit ("Just a test", null);
  },

  uninhibit : function () {
    this._urfClient.uninhibit (this._cookie);
  },

  daemon_version : function () {
    return this._urfClient.get_daemon_version ();
  },

  enumerate_devices : function () {
    for (i = 0; i < this._devices.length; i++) {
      device = this._devices[i];
      printDevice (device);
      print ("\n");
    }
  }
}

var query = new DeviceQuery ();
print ("Daemon Version:", query.daemon_version (), "\n");
query.enumerate_devices ();
query.inhibit ();

Mainloop.run ('');
