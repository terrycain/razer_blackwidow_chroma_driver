# SPDX-License-Identifier: GPL-2.0-or-later

"""
Keypad Effects
"""
from openrazer_daemon.dbus_services import endpoint


@endpoint('razer.device.lighting.profile_led', 'getRedLED', out_sig='b')
def keypad_get_profile_led_red(self):
    """
    Get red profile LED state

    :return: Red profile LED state
    :rtype: bool
    """
    self.logger.debug("DBus call keypad_profile_led_red")

    driver_path = self.get_driver_path('profile_led_red')

    with open(driver_path, 'r') as driver_file:
        return driver_file.read().strip() == '1'


@endpoint('razer.device.lighting.profile_led', 'setRedLED', in_sig='b')
def keypad_set_profile_led_red(self, enable):
    """
    Set red profile LED state

    :param enable: Status of red profile LED
    :type enable: bool
    """
    self.logger.debug("DBus call keypad_set_profile_led_red")

    driver_path = self.get_driver_path('profile_led_red')

    with open(driver_path, 'w') as driver_file:
        if enable:
            driver_file.write('1')
        else:
            driver_file.write('0')


@endpoint('razer.device.lighting.profile_led', 'getGreenLED', out_sig='b')
def keypad_get_profile_led_green(self):
    """
    Get green profile LED state

    :return: Green profile LED state
    :rtype: bool
    """
    self.logger.debug("DBus call keypad_get_profile_led_green")

    driver_path = self.get_driver_path('profile_led_green')

    with open(driver_path, 'r') as driver_file:
        return driver_file.read().strip() == '1'


@endpoint('razer.device.lighting.profile_led', 'setGreenLED', in_sig='b')
def keypad_set_profile_led_green(self, enable):
    """
    Set green profile LED state

    :param enable: Status of green profile LED
    :type enable: bool
    """
    self.logger.debug("DBus call keypad_set_profile_led_green")

    driver_path = self.get_driver_path('profile_led_green')

    with open(driver_path, 'w') as driver_file:
        if enable:
            driver_file.write('1')
        else:
            driver_file.write('0')


@endpoint('razer.device.lighting.profile_led', 'getBlueLED', out_sig='b')
def keypad_get_profile_led_blue(self):
    """
    Get blue profile LED state

    :return: Blue profile LED state
    :rtype: bool
    """
    self.logger.debug("DBus call keypad_get_profile_led_blue")

    driver_path = self.get_driver_path('profile_led_blue')

    with open(driver_path, 'r') as driver_file:
        return driver_file.read().strip() == '1'


@endpoint('razer.device.lighting.profile_led', 'setBlueLED', in_sig='b')
def keypad_set_profile_led_blue(self, enable):
    """
    Set blue profile LED state

    :param enable: Status of blue profile LED
    :type enable: bool
    """
    self.logger.debug("DBus call keypad_set_profile_led_blue")

    driver_path = self.get_driver_path('profile_led_blue')

    with open(driver_path, 'w') as driver_file:
        if enable:
            driver_file.write('1')
        else:
            driver_file.write('0')
