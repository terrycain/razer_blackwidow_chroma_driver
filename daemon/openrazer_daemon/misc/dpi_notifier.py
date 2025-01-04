# SPDX-License-Identifier: GPL-2.0-or-later

import logging
import threading
import time
import math
import subprocess


class DpiNotifier(threading.Thread):
    """
    Thread to notify about dpi changes
    """

    def __init__(self, parent, device_id, device_name):
        super().__init__()
        self._logger = logging.getLogger(
            'razer.device{0}.dpinotifier'.format(device_id))

        self.event = threading.Event()
        self.frequency = 500
        self.last_dpi_level_x = 0
        self.last_dpi_level_y = 0

        self._shutdown = False
        self._device_name = device_name

        self._get_dpi = getattr(parent, "getDPI", lambda: [0, 0])

        self._last_notify_time = math.floor(time.time() * 1000)
        self._last_notification_id = "0"

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
            result = subprocess.run(["notify-send", "-a", "OpenRazer", "-i", icon, "-r", self._last_notification_id,
                                     "-p", "-t", "4000", summary, message], check=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, text=True)
            self._last_notification_id = result.stdout.rstrip()
        except subprocess.CalledProcessError as e:
            self._logger.warning(f"Failed to show notification: {
                                 e.output.strip()}")

    def notify(self):
        dpi = self._get_dpi()
        dpi_level_x = dpi[0] if dpi[0] is not None else 0
        dpi_level_y = dpi[1] if dpi[1] is not None else 0

        dpi_x_has_changed = dpi_level_x != self.last_dpi_level_x
        dpi_y_has_changed = dpi_level_y != self.last_dpi_level_y

        if dpi_x_has_changed:
            self.last_dpi_level_x = dpi_level_x

        if dpi_y_has_changed:
            self.last_dpi_level_y = dpi_level_y

        if dpi_level_x <= 0 or (not dpi_x_has_changed and not dpi_y_has_changed):
            return

        dpi_level = str(dpi_level_x)
        if dpi_level_x != dpi_level_y:
            dpi_level = "[x:{0}, y:{1}]"

        title = self._device_name
        message = "DPI is {0}".format(dpi_level)
        icon = "input-mouse"

        self._logger.debug("{0} {1}".format(title, message))
        self.show_notification(summary=title, message=message, icon=icon)

    def run(self):
        """
        Main thread function
        """

        while not self._shutdown:
            now = math.floor(time.time() * 1000)
            have_waited_long_enough = (
                now - self._last_notify_time) < self.frequency

            if self.event.is_set() and self.frequency > 0 and have_waited_long_enough:
                self._last_notify_time = now
                self.notify()

            time.sleep(0.1)

        self._logger.debug("Shutting down DPI notifier")


class DpiNotifierManager(object):
    """
    Class which manages the overall process of DPI notifications
    """

    def __init__(self, parent, device_number, device_name):
        if getattr(parent, "getDPI", None) is None:
            return

        self._logger = logging.getLogger(
            'razer.device{0}.notifiermanager'.format(device_number))

        self._notifier = DpiNotifier(parent, device_number, device_name)
        self._notifier.start()

        self._is_closed = False

    def close(self):
        """
        Close the manager, stop ripple thread
        """
        if not self._is_closed:
            self._logger.debug("Closing DPI Notifier Manager")
            self._is_closed = True

            self._notifier.shutdown = True
            self._notifier.join(timeout=2)
            if self._notifier.is_alive():
                self._logger.error("Could not stop DPI Notifier thread")

    def __del__(self):
        self.close()

    @property
    def active(self):
        return self._notifier.event.is_set()

    @active.setter
    def active(self, value):
        if value:
            self._notifier.event.set()
        else:
            self._notifier.event.clear()

    @property
    def frequency(self):
        return self._notifier.frequency

    @frequency.setter
    def frequency(self, frequency):
        self._notifier.frequency = frequency
