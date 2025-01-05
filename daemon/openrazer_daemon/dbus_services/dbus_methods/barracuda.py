# SPDX-License-Identifier: GPL-2.0-or-later

"""
Module for barracuda methods
"""
import math
from openrazer_daemon.dbus_services import endpoint


@endpoint('razer.device.power', 'getBattery', out_sig='d')
def get_battery(self):
    """
    Get headset's battery level
    """
    self.logger.debug("DBus call get_battery")

    driver_path = self.get_driver_path('charge_level')

    with open(driver_path, 'r') as driver_file:
        battery_255 = float(driver_file.read().strip())
        if battery_255 < 0:
            return -1.0

        battery_100 = (battery_255 / 255) * 100
        return battery_100


@endpoint('razer.device.power', 'isCharging', out_sig='b')
def is_charging(self):
    """
    Get charging status
    """
    self.logger.debug("DBus call is_charging")

    driver_path = self.get_driver_path('charge_status')

    with open(driver_path, 'r') as driver_file:
        return bool(int(driver_file.read().strip()))


@endpoint('razer.device.power', 'setIdleTime', in_sig='q')
def set_idle_time(self, idle_time):
    """
    Set the idle time of the headset in seconds

    :param idle_time: Idle time in seconds (unsigned short)
    :type idle_time: int
    """
    self.logger.debug("DBus call set_idle_time")

    driver_path = self.get_driver_path('device_idle_time')

    with open(driver_path, 'w') as driver_file:
        driver_file.write(str(idle_time))


@endpoint('razer.device.power', 'getIdleTime', out_sig='q')
def get_idle_time(self):
    """
    Get the idle time of the headset in seconds

    :return: Idle time in seconds (unsigned short)
    :rtype: int
    """
    self.logger.debug("DBus call get_idle_time")

    driver_path = self.get_driver_path('device_idle_time')

    with open(driver_path, 'r') as driver_file:
        result = driver_file.read()
        result = int(result.strip())

    return result


@endpoint('razer.device.power', 'setLowBatteryThreshold', in_sig='y')
def set_low_battery_threshold(self, threshold):
    """
    Set the low battery threshold as a percentage

    :param threshold: Battery threshold as a percentage
    :type threshold: int
    """
    self.logger.debug("DBus call set_low_battery_threshold")

    driver_path = self.get_driver_path('charge_low_threshold')

    threshold = math.floor((threshold / 100) * 255)

    with open(driver_path, 'w') as driver_file:
        driver_file.write(str(threshold))


@endpoint('razer.device.power', 'getLowBatteryThreshold', out_sig='y')
def get_low_battery_threshold(self):
    """
    Get the low battery threshold as a percentage

    :return: Battery threshold as a percentage
    :rtype: int
    """
    self.logger.debug("DBus call get_low_battery_threshold")

    driver_path = self.get_driver_path('charge_low_threshold')

    with open(driver_path, 'r') as driver_file:
        result = driver_file.read()
        result = int(result.strip())

    return round((result / 255) * 100)
