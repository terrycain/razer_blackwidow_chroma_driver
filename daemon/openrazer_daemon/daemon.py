# SPDX-License-Identifier: GPL-2.0-or-later

"""
Daemon class

This class is the main core of the daemon, this serves a basic dbus module to control the main bit of the daemon
"""
__version__ = '3.8.0'

import configparser
import logging
import logging.handlers
import os
import sys
import signal
import time
import setproctitle
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib
from pyudev import Context, Monitor, MonitorObserver
import grp
import getpass
import json
import threading

import openrazer_daemon.hardware
from openrazer_daemon.dbus_services.service import DBusService
from openrazer_daemon.device import DeviceCollection
from openrazer_daemon.misc.screensaver_monitor import ScreensaverMonitor
from openrazer_daemon.misc.autosave_persistence import PersistenceAutoSave


class RazerDaemon(DBusService):
    """
    Daemon class

    This class sets up the main run loop which serves DBus messages. The logger is initialised
    in this module as well as finding and initialising devices.

    Serves the following functions via DBus
    * getDevices - Returns a list of serial numbers
    * enableTurnOffOnScreensaver - Starts/Continues the run loop on the screensaver thread
    * disableTurnOffOnScreensaver - Pauses the run loop on the screensaver thread
    """

    def __init__(self, verbose=False, log_dir=None, console_log=False, run_dir=None, config_file=None, persistence_file=None, test_dir=None):

        setproctitle.setproctitle('openrazer-daemon')  # pylint: disable=no-member

        # Expanding ~ as python doesn't do it by default, also creating dirs if needed
        try:
            if log_dir is not None:
                log_dir = os.path.expanduser(log_dir)
                os.makedirs(log_dir, exist_ok=True)
            if run_dir is not None:
                run_dir = os.path.expanduser(run_dir)
                os.makedirs(run_dir, exist_ok=True)
        except NotADirectoryError as e:
            print("Failed to create {}".format(e.filename), file=sys.stderr)
            sys.exit(1)

        if config_file is not None:
            config_file = os.path.expanduser(config_file)
            if not os.path.exists(config_file):
                print("Config file {} does not exist.".format(config_file), file=sys.stderr)
                sys.exit(1)

        if persistence_file is not None:
            persistence_file = os.path.expanduser(persistence_file)
            if not os.path.exists(persistence_file):
                print("Persistence file {} does not exist.".format(persistence_file), file=sys.stderr)
                sys.exit(1)

        self._test_dir = test_dir
        self._run_dir = run_dir

        self._config_file = config_file
        self._config = configparser.ConfigParser()
        self.read_config(config_file)

        # Logging
        log_level = logging.INFO
        if verbose or self._config.getboolean('General', 'verbose_logging'):
            log_level = logging.DEBUG
        self.logger = self._create_logger(log_dir, log_level, console_log)

        self._persistence_file = persistence_file
        self._persistence = configparser.ConfigParser()
        self._persistence.status = {"changed": False}
        self.read_persistence(persistence_file)

        # Check for plugdev group
        if not self._check_plugdev_group():
            self.logger.critical("User is not a member of the plugdev group")
            self.logger.critical("Please run the command 'sudo gpasswd -a $USER plugdev' and then reboot!")
            sys.exit(1)

        # Setup DBus to use gobject main loop
        dbus.mainloop.glib.threads_init()
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        super().__init__('/org/razer')

        self._init_signals()
        self._main_loop = GLib.MainLoop()

        # Listen for input events from udev
        self._init_udev_monitor()

        # Load Classes
        self._device_classes = openrazer_daemon.hardware.get_device_classes()

        self.logger.info("Initialising Daemon (v%s). Pid: %d", __version__, os.getpid())
        self._init_screensaver_monitor()

        self._razer_devices = DeviceCollection()
        self._load_devices(first_run=True)

        # Add DBus methods
        methods = {
            # interface, method, callback, in-args, out-args
            ('razer.devices', 'getDevices', self.get_serial_list, None, 'as'),
            ('razer.devices', 'supportedDevices', self.supported_devices, None, 's'),
            ('razer.devices', 'enableTurnOffOnScreensaver', self.enable_turn_off_on_screensaver, 'b', None),
            ('razer.devices', 'getOffOnScreensaver', self.get_off_on_screensaver, None, 'b'),
            ('razer.devices', 'syncEffects', self.sync_effects, 'b', None),
            ('razer.devices', 'getSyncEffects', self.get_sync_effects, None, 'b'),
            ('razer.daemon', 'version', self.version, None, 's'),
            ('razer.daemon', 'stop', self.stop, None, None),
        }

        for m in methods:
            self.logger.debug("Adding {}.{} method to DBus".format(m[0], m[1]))
            self.add_dbus_method(m[0], m[1], m[2], in_signature=m[3], out_signature=m[4])

        self._collecting_udev = False
        self._collecting_udev_devices = []

        self._init_autosave_persistence()

        # TODO remove
        self.sync_effects(self._config.getboolean('Startup', 'sync_effects_enabled'))
        # TODO ======

    @dbus.service.signal('razer.devices')
    def device_removed(self):
        self.logger.debug("Emitted Device Remove Signal")

    @dbus.service.signal('razer.devices')
    def device_added(self):
        self.logger.debug("Emitted Device Added Signal")

    def _create_logger(self, log_dir, log_level, want_console_log):
        """
        Initializes a logger and returns it.

        :param log_dir: If not None, specifies the directory to create the
        log file in
        :param log_level: The log level of messages to print
        :param want_console_log: True if we should print to the console

        :rtype:logging.Logger
        """
        logger = logging.getLogger('razer')
        logger.setLevel(log_level)
        formatter = logging.Formatter('%(asctime)s | %(name)-30s | %(levelname)-8s | %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
        # Don't propagate to default logger
        logger.propagate = 0

        if want_console_log:
            console_logger = logging.StreamHandler()
            console_logger.setLevel(log_level)
            console_logger.setFormatter(formatter)
            logger.addHandler(console_logger)

        if log_dir is not None:
            log_file = os.path.join(log_dir, 'razer.log')
            file_logger = logging.handlers.RotatingFileHandler(log_file, maxBytes=1048576, backupCount=1)  # 1 MiB
            file_logger.setLevel(log_level)
            file_logger.setFormatter(formatter)
            logger.addHandler(file_logger)

        return logger

    def _check_plugdev_group(self):
        """
        Check if the user is a member of the plugdev group. For the root
        user, this always returns True

        :rtype: bool
        """
        if getpass.getuser() == 'root':
            return True

        try:
            return grp.getgrnam('plugdev').gr_gid in os.getgroups()
        except KeyError:
            pass

        return False

    def _init_udev_monitor(self):
        self._udev_context = Context()
        udev_monitor = Monitor.from_netlink(self._udev_context)
        udev_monitor.filter_by(subsystem='hid')
        self._udev_observer = MonitorObserver(udev_monitor, callback=self._udev_input_event, name='device-monitor')

    def _init_screensaver_monitor(self):
        try:
            self._screensaver_monitor = ScreensaverMonitor(self)
            self._screensaver_monitor.monitoring = self._config.getboolean('Startup', 'devices_off_on_screensaver')
        except dbus.exceptions.DBusException as e:
            self.logger.error("Failed to init ScreensaverMonitor: {}".format(e))

    def _init_autosave_persistence(self):
        if not self._persistence:
            self.logger.debug("Persistence unspecified. Will not create auto save thread")
            return

        self._autosave_persistence = PersistenceAutoSave(self._persistence, self._persistence_file, self._persistence.status, self.logger, 10, self.write_persistence)
        self._autosave_persistence.thread = threading.Thread(target=self._autosave_persistence.watch)
        self._autosave_persistence.thread.daemon = True
        self._autosave_persistence.thread.start()

    def _init_signals(self):
        """
        Heinous hack to properly handle signals on the mainloop. Necessary
        if we want to use the mainloop run() functionality.
        """
        def signal_action(signum):
            """
            Action to take when a signal is trapped
            """
            self.quit(signum)

        def idle_handler():
            """
            GLib idle handler to propagate signals
            """
            GLib.idle_add(signal_action, priority=GLib.PRIORITY_HIGH)

        def handler(*args):
            """
            Unix signal handler
            """
            signal_action(args[0])

        def install_glib_handler(sig):
            """
            Choose a compatible method and install the handler
            """
            unix_signal_add = None

            if hasattr(GLib, "unix_signal_add"):
                unix_signal_add = GLib.unix_signal_add
            elif hasattr(GLib, "unix_signal_add_full"):
                unix_signal_add = GLib.unix_signal_add_full

            if unix_signal_add:
                unix_signal_add(GLib.PRIORITY_HIGH, sig, handler, sig)
            else:
                print("Can't install GLib signal handler!")

        for sig in signal.SIGINT, signal.SIGTERM, signal.SIGHUP:
            signal.signal(sig, idle_handler)
            GLib.idle_add(install_glib_handler, sig, priority=GLib.PRIORITY_HIGH)

    def read_config(self, config_file):
        """
        Read in the config file and set the defaults

        :param config_file: Config file
        :type config_file: str or None
        """
        # Generate sections as trying to access a value even if a default exists will die if the section does not
        for section in ('General', 'Startup'):
            self._config[section] = {}

        self._config['General'] = {
            'verbose_logging': False,
        }
        self._config['Startup'] = {
            'sync_effects_enabled': True,
            'devices_off_on_screensaver': True,
            'restore_persistence': True,
            'persistence_dual_boot_quirk': False,
        }

        if config_file is not None and os.path.exists(config_file):
            self._config.read(config_file)

        # Compatibility to older configs
        if 'mouse_battery_notifier' in self._config['Startup']:
            self._config['Startup']['battery_notifier'] = self._config['Startup']['mouse_battery_notifier']
        if 'mouse_battery_notifier_freq' in self._config['Startup']:
            self._config['Startup']['battery_notifier_freq'] = self._config['Startup']['mouse_battery_notifier_freq']

    def read_persistence(self, persistence_file):
        """
        Read the persistence file and set states into memory

        :param persistence_file: Persistence file
        :type persistence_file: str or None
        """
        if persistence_file is not None and os.path.exists(persistence_file):
            try:
                self._persistence.read(persistence_file)
            except (configparser.Error, UnicodeDecodeError):
                self.logger.warning('Failed to read persistence config, resetting!', exc_info=True)
                with open(persistence_file, "w") as f:
                    f.writelines("")

    def write_persistence(self, persistence_file):
        """
        Write in the persistence file

        :param persistence_file: Persistence file
        :type persistence_file: str or None
        """
        if not persistence_file:
            return

        self.logger.debug('Writing persistence config')

        for device in self._razer_devices:
            self._persistence[device.dbus.storage_name] = {}
            if 'set_dpi_xy' in device.dbus.METHODS or 'set_dpi_xy_byte' in device.dbus.METHODS:
                dpi_x = int(device.dbus.dpi[0])
                dpi_y = int(device.dbus.dpi[1])
                # When Y is not greater than 0 check for a DPI X only device, a device with 'available_dpi' and a Y value of 0
                if dpi_x > 0 and (dpi_y > 0 or ('available_dpi' in device.dbus.METHODS and dpi_y == 0)):
                    self._persistence[device.dbus.storage_name]['dpi_x'] = str(dpi_x)
                    self._persistence[device.dbus.storage_name]['dpi_y'] = str(dpi_y)

            if 'set_poll_rate' in device.dbus.METHODS:
                self._persistence[device.dbus.storage_name]['poll_rate'] = str(device.dbus.poll_rate)

            for i in device.dbus.ZONES:
                if device.dbus.zone[i]["present"]:
                    self._persistence[device.dbus.storage_name][i + '_active'] = str(device.dbus.zone[i]["active"])
                    self._persistence[device.dbus.storage_name][i + '_brightness'] = str(device.dbus.zone[i]["brightness"])
                    self._persistence[device.dbus.storage_name][i + '_effect'] = device.dbus.zone[i]["effect"]
                    self._persistence[device.dbus.storage_name][i + '_colors'] = ' '.join(str(i) for i in device.dbus.zone[i]["colors"])
                    self._persistence[device.dbus.storage_name][i + '_speed'] = str(device.dbus.zone[i]["speed"])
                    self._persistence[device.dbus.storage_name][i + '_wave_dir'] = str(device.dbus.zone[i]["wave_dir"])

        with open(persistence_file, 'w') as cf:
            self._persistence.write(cf)

    def get_off_on_screensaver(self):
        """
        Returns if turn off on screensaver

        :return: Result
        :rtype: bool
        """
        return self._screensaver_monitor.monitoring

    def enable_turn_off_on_screensaver(self, enable):
        """
        Enable the turning off of devices when the screensaver is active
        """
        self._screensaver_monitor.monitoring = enable

    def supported_devices(self):
        result = {cls.__name__: (cls.USB_VID, cls.USB_PID) for cls in self._device_classes}

        return json.dumps(result)

    def version(self):
        """
        Get the daemon version

        :return: Version string
        :rtype: str
        """
        return __version__

    def suspend_devices(self):
        """
        Suspend all devices
        """
        for device in self._razer_devices:
            device.dbus.suspend_device()

    def resume_devices(self):
        """
        Resume all devices
        """
        for device in self._razer_devices:
            device.dbus.resume_device()

    def get_serial_list(self):
        """
        Get list of devices serials
        """
        serial_list = self._razer_devices.serials()
        self.logger.debug('DBus called get_serial_list')
        return serial_list

    def sync_effects(self, enabled):
        """
        Sync the effects across the devices

        :param enabled: True to sync effects
        :type enabled: bool
        """
        # Todo perhaps move logic to device collection
        for device in self._razer_devices.devices:
            device.dbus.effect_sync = enabled

    def get_sync_effects(self):
        """
        Sync the effects across the devices

        :return: True if any devices sync effects
        :rtype: bool
        """
        result = False

        for device in self._razer_devices.devices:
            result |= device.dbus.effect_sync

        return result

    def _load_devices(self, first_run=False):
        """
        Go through supported devices and load them

        Loops through the available hardware classes, loops through
        each device in the system and adds it if needs be.
        """
        if first_run:
            # Just some pretty output
            max_name_len = max([len(cls.__name__) for cls in self._device_classes]) + 2
            for cls in self._device_classes:
                format_str = 'Loaded device specification: {0:-<' + str(max_name_len) + '} ({1:04x}:{2:04X})'

                self.logger.debug(format_str.format(cls.__name__ + ' ', cls.USB_VID, cls.USB_PID))

        if self._test_dir is not None:
            device_list = os.listdir(self._test_dir)
            test_mode = True
        else:
            device_list = list(self._udev_context.list_devices(subsystem='hid'))
            test_mode = False

        device_number = 0
        for device in device_list:

            for device_class in self._device_classes:
                # Interoperability between generic list of 0000:0000:0000.0000 and pyudev
                if test_mode:
                    sys_name = device
                    sys_path = os.path.join(self._test_dir, device)
                else:
                    sys_name = device.sys_name
                    sys_path = device.sys_path

                if sys_name in self._razer_devices:
                    continue

                if device_class.match(sys_name, sys_path):  # Check it matches sys/ ID format and has device_type file
                    self.logger.info('Found device.%d: %s', device_number, sys_name)

                    # TODO add testdir support
                    # Basically find the other usb interfaces
                    device_match = sys_name.split('.')[0]
                    additional_interfaces = []
                    if not test_mode:
                        double_device = False
                        for alt_device in self._razer_devices:
                            if device_match in alt_device.device_id and alt_device.device_id != sys_name and sys_path in alt_device.dbus.additional_interfaces:
                                self.logger.warning('BUG: Device %s has already been found with interface %s. Skipping', sys_name, alt_device.device_id)
                                double_device = True
                        if double_device:
                            continue

                        for alt_device in device_list:
                            if device_match in alt_device.sys_name and alt_device.sys_name != sys_name:
                                additional_interfaces.append(alt_device.sys_path)

                    # Checking permissions
                    test_file = os.path.join(sys_path, 'device_type')
                    file_group_id = os.stat(test_file).st_gid
                    file_group_name = grp.getgrgid(file_group_id)[0]

                    if os.getgid() != file_group_id and file_group_name != 'plugdev':
                        self.logger.critical("Could not access {0}/device_type, file is not owned by plugdev".format(sys_path))
                        break

                    razer_device = device_class(device_path=sys_path, device_number=device_number, config=self._config,
                                                persistence=self._persistence, testing=self._test_dir is not None,
                                                additional_interfaces=sorted(additional_interfaces),
                                                additional_methods=[])

                    # Wireless devices sometimes don't listen
                    count = 0
                    while count < 3:
                        # Loop to get serial, exit early if it gets one
                        device_serial = razer_device.get_serial()
                        if len(device_serial) > 0:
                            break
                        time.sleep(0.1)
                        count += 1
                    else:
                        logging.warning("Could not get serial for device {0}. Skipping".format(sys_name))
                        continue

                    self._razer_devices.add(sys_name, device_serial, razer_device)

                    device_number += 1

    def _add_device(self, device):
        """
        Add device event from udev

        :param device: Udev Device
        :type device: pyudev.device._device.Device
        """
        device_number = len(self._razer_devices)
        for device_class in self._device_classes:
            sys_name = device.sys_name
            sys_path = device.sys_path

            if sys_name in self._razer_devices:
                continue

            if device_class.match(sys_name, sys_path):  # Check it matches sys/ ID format and has device_type file
                self.logger.info('Found valid device.%d: %s', device_number, sys_name)
                razer_device = device_class(device_path=sys_path, device_number=device_number, config=self._config,
                                            persistence=self._persistence, testing=self._test_dir is not None,
                                            additional_interfaces=None, additional_methods=[])

                # Its a udev event so currently the device hasn't been chmodded yet
                time.sleep(0.2)

                # Wireless devices sometimes don't listen
                device_serial = razer_device.get_serial()

                if len(device_serial) > 0:
                    # Add Device
                    self._razer_devices.add(sys_name, device_serial, razer_device)
                    self.device_added()
                else:
                    logging.warning("Could not get serial for device {0}. Skipping".format(sys_name))
            else:
                # Basically find the other usb interfaces
                device_match = sys_name.split('.')[0]
                for d in self._razer_devices:
                    if device_match in d.device_id and d.device_id != sys_name:
                        if not sys_path in d.dbus.additional_interfaces:
                            d.dbus.additional_interfaces.append(sys_path)
                            return

    def _remove_device(self, device):
        """
        Remove device event from udev

        :param device: Udev Device
        :type device: pyudev.device._device.Device
        """
        device_id = device.sys_name

        try:
            device = self._razer_devices[device_id]

            device.dbus.close()
            device.dbus.remove_from_connection()
            self.write_persistence(self._persistence_file)
            self.logger.warning("Removing %s", device_id)

            # Delete device
            del self._razer_devices[device.device_id]
            self.device_removed()

        except IndexError:  # Why didn't i set it up as KeyError
            # It will return "extra" events for the additional usb interfaces bound to the driver
            pass

    def _udev_input_event(self, device):
        """
        Function called by the Udev monitor (#observerPattern)

        :param device: Udev device
        :type device: pyudev.device._device.Device
        """
        self.logger.debug('Device event [%s]: %s', device.action, device.device_path)
        if device.action == 'add':
            if self._collecting_udev:
                self._collecting_udev_devices.append(device)
                return
            else:
                self._collecting_udev_devices = [device]
                self._collecting_udev = True
                t = threading.Thread(target=self._collecting_udev_method, args=(device,))
                t.start()
        elif device.action == 'remove':
            self._remove_device(device)

    def _collecting_udev_method(self, device):
        time.sleep(2)  # delay to let udev add all devices that we want
        # Sort the devices
        self._collecting_udev_devices.sort(key=lambda x: x.sys_path, reverse=True)
        for d in self._collecting_udev_devices:
            self._add_device(d)
        self._collecting_udev = False

    def run(self):
        """
        Run the daemon
        """
        self.logger.info('Serving DBus')

        # Start listening for device changes
        self._udev_observer.start()

        # Start the mainloop
        try:
            self._main_loop.run()
        except KeyboardInterrupt:
            self.logger.debug('Shutting down')

    def stop(self):
        """
        Wrapper for quit
        """
        self.quit(None)

    def quit(self, signum):
        """
        Quit by stopping the main loop, observer, and screensaver thread
        """
        # pylint: disable=unused-argument
        if signum is None:
            self.logger.info('Stopping daemon.')
        else:
            self.logger.info('Stopping daemon on signal %d', signum)

        self._main_loop.quit()

        # Stop udev monitor
        self._udev_observer.send_stop()

        for device in self._razer_devices:
            device.dbus.close()

        # Write config
        self.write_persistence(self._persistence_file)
