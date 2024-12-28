# SPDX-License-Identifier: GPL-2.0-or-later

"""
This will do until I can be bothered to create indicator applet to do battery level
"""
import logging
import threading
import datetime
import time
import subprocess


class BatteryNotifier(threading.Thread):
    """
    Thread to notify about battery
    """

    def __init__(self, parent, device_id, device_name):
        super().__init__()
        self._logger = logging.getLogger('razer.device{0}.batterynotifier'.format(device_id))

        self.event = threading.Event()
        self.frequency = 0
        self.percent = 0

        self._shutdown = False
        self._device_name = device_name

        # Could save reference to parent but only need battery level function
        self._get_battery_func = parent.getBattery

        self._last_notify_time = datetime.datetime(1970, 1, 1)

    @property
    def shutdown(self):
        """
        Get the shutdown flag
        """
        return self._shutdown

    @shutdown.setter
    def shutdown(self, value):
        """
        Set the shutdown flag

        :param value: Shutdown
        :type value: bool
        """
        self._shutdown = value

    def show_notification(self, summary: str, message: str, icon: str) -> None:
        try:
            subprocess.run(["notify-send", "-a", "OpenRazer", "-i", icon, "-t", "4000", summary, message],
                           check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        except subprocess.CalledProcessError as e:
            self._logger.warning(f"Failed to show notification: {e.output.strip()}")

    def notify_battery(self):
        now = datetime.datetime.now()

        if (now - self._last_notify_time).seconds > self.frequency:
            battery_level = self._get_battery_func()
            battery_percent = round(battery_level)

            # Sometimes due to various issues we don't get the percentage correctly.
            # Just ignore them and don't show a bogus notification.
            # See also: https://github.com/openrazer/openrazer/issues/2122
            if battery_level in (0.0, -1.0):
                self._logger.debug("Got bogus battery value: {0}, ignoring.".format(battery_level))
                # Since we don't update _last_notify_time here we're going to retry very soon again.
                # Sleep a bit so we don't spam the device with requests.
                time.sleep(10)
                return

            # Update the last notified time so that we alert in the configured frequency.
            self._last_notify_time = now

            title = self._device_name
            message = "Battery is {0}%".format(battery_percent)
            icon = "battery-full"

            if battery_level <= 10.0:
                message = "Battery is low ({0}%). Please charge your device".format(battery_percent)
                icon = "battery-empty"

            elif battery_level <= 30.0:
                icon = "battery-low"

            elif battery_level <= 70.0:
                icon = "battery-good"

            elif battery_level == 100.0:
                message = "Battery is fully charged ({0}%)".format(battery_percent)

            self._logger.debug("{0} Battery at {1}%".format(self._device_name, battery_percent))

            if battery_level <= self.percent:
                self.show_notification(summary=title, message=message, icon=icon)

    def run(self):
        """
        Main thread function
        """

        while not self._shutdown:
            if self.event.is_set() and self.frequency > 0:
                self.notify_battery()

            time.sleep(0.1)

        self._logger.debug("Shutting down battery notifier")


class BatteryManager(object):
    """
    Class which manages the overall process of notifing battery levels
    """

    def __init__(self, parent, device_number, device_name):
        self._logger = logging.getLogger('razer.device{0}.batterymanager'.format(device_number))
        self._parent = parent

        self._battery_thread = BatteryNotifier(parent, device_number, device_name)
        self._battery_thread.start()

        self._is_closed = False

    def close(self):
        """
        Close the manager, stop ripple thread
        """
        if not self._is_closed:
            self._logger.debug("Closing Battery Manager")
            self._is_closed = True

            self._battery_thread.shutdown = True
            self._battery_thread.join(timeout=2)
            if self._battery_thread.is_alive():
                self._logger.error("Could not stop BatteryNotify thread")

    def __del__(self):
        self.close()

    @property
    def active(self):
        return self._battery_thread.event.is_set()

    @active.setter
    def active(self, value):
        if value:
            self._battery_thread.event.set()
        else:
            self._battery_thread.event.clear()

    @property
    def frequency(self):
        return self._battery_thread.frequency

    @frequency.setter
    def frequency(self, frequency):
        self._battery_thread.frequency = frequency

    @property
    def percent(self):
        return self._battery_thread.percent

    @percent.setter
    def percent(self, percent):
        self._battery_thread.percent = percent
