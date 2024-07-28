razer.conf(5)

# NAME

razer.conf - configuration file for the OpenRazer Daemon

# DESCRIPTION

The _~/.config/openrazer/razer.conf_ (*$XDG_CONFIG_HOME* is respected, if set) file configures some startup and runtime options of the *openrazer-daemon*(8). *openrazer-daemon*(8) is able to use configuration files specified in non-standard locations by providing the _--config_ argument.

The configuration file is formatted like a .INI file and is parsed by Python's ConfigParser.

# EXAMPLE

An example file should be installed under _/usr/share/openrazer/razer.conf.example_.

# SECTIONS

## GENERAL

The first section in the configuration file is a *[General]* section. This section contains miscellaneous options.

*verbose_logging* _bool_
	This flag specifies if the daemon is to output detailed logging information. This value acts the same as if *-v* or *--verbose* was passed at the command line.

## STARTUP

The *[Startup]* section in the configuration file contains values to be used during startup, for example it can decide if syncing effects will be active when started.

*sync_effects_enabled* _bool_
	This flag specifies if the effects syncing logic is active when the daemon is started, not having to wait for the user to activate them.

*devices_off_on_screensaver* _bool_
	This flag specifies if the functionality to turn off razer devices when the screensaver is activated is active when the daemon starts.

*battery_notifier* _bool_
	This flag specifies that notifications should be shown regularly with the remaining battery level.

*battery_notifier_freq* _int_
	This flag specifies how often the notification with the remaining battery level should be shown.

*battery_notifier_percent* _int_
	This flag specifies what battery percentage the device should reach before sending notifications.

*restore_persistence* _bool_
	This flag specifies whether effects saved in persistence.conf should be applied when the daemon starts.

*persistence_dual_boot_quirk* _bool_
	This flag specifies whether the persisted effect is applied twice to address a quirk with newer devices that were last changed by Razer Synapse on Windows. Only applicable with restore_persistence = True.

# SEE ALSO

*openrazer-daemon*(8)
