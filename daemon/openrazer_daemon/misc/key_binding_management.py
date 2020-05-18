"""
Recives key events from the key event manager and does stuff.
"""

import json
import logging
import os
import time
import sys
import subprocess
from openrazer_daemon.keyboard import KeyboardColour, KEY_UP, KEY_DOWN, KEY_HOLD

# TODO: figure out a better way to handle this
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from evdev import UInput, ecodes

DEFAULT_PROFILE = {
    "name": "Default",
    "default_map": "Default",
    "Default": {
        "is_using_matrix": False,
        "binding": {
        }
    }
}


class KeybindingManager():
    """
    Key binding manager
    """
    # pylint: disable=too-many-instance-attributes

    def __init__(self, device_id, parent, testing=False):
        self._parent = parent
        self._device_id = device_id
        self._serial_number = self._parent.getSerial()
        self._config_file = self.get_config_file_name()

        self._logger = logging.getLogger('razer.device{0}.bindingmanager'.format(device_id))
        # self._parent.register_observer(self)
        self._testing = testing
        self._fake_device = UInput(name="{0} (mapped)".format(self._parent.getDeviceName()))

        self._profiles = {"0": DEFAULT_PROFILE}
        self._current_profile_id = None

        self._current_keys = []
        self._old_mapping_name = None
        self._current_mapping_name = None
        self._shift_modifier = None

        self._rows, self._cols = self._parent.MATRIX_DIMS
        self._keyboard_grid = KeyboardColour(self._rows, self._cols)

        self._macro_mode = False
        self._macro_key = None

        if os.path.exists(self._config_file):
            self.read_config_file(self._config_file)
        self.current_profile = "0"

    # pylint: disable=no-member
    def __key(self, key_code, key_action):
        key_code = int(key_code)
        if key_action == KEY_UP:
            for _ in range(0, 5):
                try:
                    self._current_keys.remove(key_code)
                    break
                except ValueError:
                    time.sleep(.005)

        elif key_action == KEY_DOWN:
            self._current_keys.append(key_code)

        self._fake_device.write(ecodes.EV_KEY, key_code, key_action)
        self._fake_device.syn()

    # pylint: disable=too-many-branches
    def key_action(self, key_code, key_action):
        """
        Check for a binding and act on it.

        :param key_code: The key code of the pressed key.
        :type key_code: int
        """
        # self._logger.debug("Key action: {0}, {1}".format(key_code, key_action))

        key_code = str(key_code)
        current_binding = self.current_mapping["binding"]

        if key_code in current_binding:  # Key bound
            for action in current_binding[key_code]:
                if key_action != KEY_UP:  # Key pressed (or autorepeat)
                    if action["type"] == "execute":
                        subprocess.run(["/bin/sh", "-c", action["value"]], check=False)

                    elif action["type"] == "key":
                        self.__key(action["value"], key_action)

                    elif action["type"] == "map":
                        self.current_mapping = action["value"]
                        self._shift_modifier = None  # No happy accidents

                    elif action["type"] == "profile":
                        i = 0
                        for _, profile in self._profiles.items():
                            if profile["name"] == action["value"]:
                                self.current_profile = str(i)
                                self._shift_modifier = None  # No happy accidents
                                break
                            i += 1

                    elif action["type"] == "release":
                        self.__key(action["value"], KEY_UP)

                    elif action["type"] == "shift":
                        self.current_mapping = action["value"]
                        self._shift_modifier = key_code  # Set map shift key

                    elif action["type"] == "sleep":
                        time.sleep(int(action["value"]))

                else:
                    if key_code not in (183, 184, 185, 186, 187):  # Macro key released, skip it
                        if action["type"] == "key":  # Key released
                            self.__key(action["value"], KEY_UP)

                        elif action["type"] == "sleep":  # Wait for key to be added before removing it
                            time.sleep(int(action["value"]))

        else:  # Ordinary key action
            if key_code == self._shift_modifier:  # Key is the shift modifier
                if key_action == KEY_UP:
                    self.current_mapping = self._old_mapping_name
                    self._shift_modifier = None
                    for key in self._current_keys:  # If you forget to release a key before the releasing shift modifier, release it now.
                        self.__key(key, KEY_UP)
            else:
                self.__key(key_code, key_action)

    @property
    def current_mapping(self):
        """
        Return the current mapping

        :return: The current mapping
        :rtype: dict
        """
        return self._current_mapping

    @current_mapping.setter
    def current_mapping(self, value):
        """
        Set the current mapping and changes the led matrix

        :param value: The new mapping
        :type value: str
        """
        self._logger.debug("Change mapping to %s", str(value))

        self._current_mapping = self._current_profile[value]
        self._old_mapping_name = self._current_mapping_name
        self._current_mapping_name = value

        if self._current_mapping["is_using_matrix"]:
            current_matrix = self._current_mapping["matrix"]
            for row in current_matrix:
                for key in current_matrix[row]:
                    self._keyboard_grid.set_key_colour(int(row), int(key), tuple(current_matrix[row][key]))

            payload = self._keyboard_grid.get_total_binary()
            self._parent.setKeyRow(payload)
            self._parent.setCustom()

        try:
            capabilities = self._parent.METHODS
            if 'keypad_set_profile_led_red' in capabilities:
                self._parent.setRedLED(self.current_mapping["red_led"])
            if 'keypad_set_profile_led_green' in capabilities:
                self._parent.setGreenLED(self.current_mapping["green_led"])
            if 'keypad_set_profile_led_blue' in capabilities:
                self._parent.setBlueLED(self.current_mapping["blue_led"])
        except KeyError:
            pass

    @property
    def current_profile(self):
        """
        Return the current profile

        :return: the current profile name
        :rtype: str
        """
        return self._current_profile["name"]

    @current_profile.setter
    def current_profile(self, value):
        """
        Set the current profile

        :param value: The profile number
        :type: int
        """
        self._current_profile = self._profiles[value]
        self._current_profile_id = value
        self.current_mapping = self._current_profile["default_map"]

    @property
    def macro_mode(self):
        """
        Return the state of macro mode

        :return: the macro mode state
        :rtype: bool
        """
        return self._macro_mode

    @macro_mode.setter
    def macro_mode(self, value):
        """
        Set the state of macro mode

        :param value: The state of macro mode
        :type: bool
        """
        if value:
            self._macro_mode = True
            self._parent.setMacroEffect(0x01)
            self._parent.setMacroMode(True)

        elif not value:
            self._macro_mode = False
            self.macro_key = None
            self._parent.setMacroMode(False)

    @property
    def macro_key(self):
        """
        Return the macro key being recorded to

        :return: the macro key
        :rtype: int
        """
        return self._macro_key

    @macro_key.setter
    def macro_key(self, value):
        """
        Set the macro key being recorded to

        :param value: The macro key
        :type: int
        """
        if value is not None:
            self._macro_key = value
            self._parent.setMacroEffect(0x00)
            self._parent.clearActions(self._current_profile_id, self._current_mapping_name, str(value))

        else:
            self._macro_key = None

    def read_config_file(self, config_file):
        """
        Read the configuration file and sets the variables accordingly

        :param config_file: path to the configuration file
        :type config_file: str

        :param profile: The profile name to use
        :type profile: str
        """
        self._logger.info("Reading config file from %s", config_file)

        with open(config_file, 'r') as file:
            self._profiles = json.load(file)

    def write_config_file(self, config_file):
        """
        Write the _profiles dict to the config file

        :param config_file: The path to the config file
        :type config_file: str
        """
        self._logger.debug("writing config file to %s", config_file)

        with open(config_file, 'w') as file:
            json.dump(self._profiles, file, indent=4)

    def get_config_file_name(self):
        """
        Get the name of the config file

        :return: path to config file
        :rtype: str
        """
        # pylint: disable=protected-access
        config_path = os.path.dirname(self._parent._config_file)
        return config_path + "/keybinding_" + self._serial_number + ".json"

    def close(self):
        try:
            self._fake_device.close()
        except RuntimeError as err:
            self._logger.exception("Error closing fake device", exc_info=err)

    def __del__(self):
        self.close()

    # DBus Stuff
    def dbus_get_profiles(self):
        return_list = []
        for _, profile in self._profiles:
            return_list.append(profile["name"])

        return json.dumps(return_list)

    def dbus_get_maps(self, profile):
        return_list = []
        for key in self._profiles[profile]:
            if key not in ("name", "default_map"):
                return_list.append(key)

        return json.dumps(return_list)

    # pylint: disable=too-many-arguments
    def dbus_add_action(self, profile, mapping, key_code, action_type, value, action_id=None):
        key = self._profiles[profile][mapping]["binding"].setdefault(key_code, [])

        if action_id is not None:
            key[action_id] = {"type": action_type, "value": value}
        else:
            key.append({"type": action_type, "value": value})

        self.write_config_file(self._config_file)

    def dbus_set_matrix(self, profile, mapping, frame):
        self._profiles[profile][mapping].update({"matrix": frame})
        self._profiles[profile][mapping]["is_using_matrix"] = True

        self.write_config_file(self._config_file)
