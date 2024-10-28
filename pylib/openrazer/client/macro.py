# SPDX-License-Identifier: GPL-2.0-or-later

import json as _json

import dbus as _dbus

import openrazer_daemon.misc.macro as _daemon_macro
from openrazer_daemon import keyboard


class RazerMacro(object):
    def __init__(self, serial: str, devname: str, daemon_dbus=None, capabilities=None):
        if daemon_dbus is None:
            session_bus = _dbus.SessionBus()
            daemon_dbus = session_bus.get_object("org.razer", "/org/razer/device/{0}".format(serial))

        if capabilities is None:
            self._capabilities = {}
        else:
            self._capabilities = capabilities

        self._macro_dbus = _dbus.Interface(daemon_dbus, "razer.device.macro")

        self._macro_enabled = False

        self.name = devname

    def get_macros(self) -> dict:
        json_payload = self._macro_dbus.getMacros()
        macro_structure = _json.loads(json_payload)

        macro_key_mapping = {}
        for bind_key, macro_list in macro_structure.items():
            macro_objects = []
            for macro_dict in macro_list:
                macro_obj = _daemon_macro.macro_dict_to_obj(macro_dict)
                macro_objects.append(macro_obj)

            macro_key_mapping[bind_key] = macro_objects

        return macro_key_mapping

    def add_macro(self, bind_key: str, macro_object_sequence: list):
        """
        Add macro to specified bind key

        :param bind_key: Bind Key (has to be in openrazer.keyboard.KEY_MAPPING)
        :type bind_key: str

        :param macro_object_sequence: List of macro objects
        :type macro_object_sequence: list or tuple or __daemon_macro.MacroObject
        """
        if isinstance(macro_object_sequence, _daemon_macro.MacroObject):
            macro_object_sequence = [macro_object_sequence]
        if not isinstance(macro_object_sequence, (tuple, list)):
            raise ValueError("macro_object_sequence is not iterable")

        macro_list = []
        for macro_obj in macro_object_sequence:
            if not isinstance(macro_obj, _daemon_macro.MacroObject):
                raise ValueError("{0} is not a macro object".format(str(macro_obj)))

            macro_list.append(macro_obj.to_dict())

        json_payload = _json.dumps(macro_list)
        self._macro_dbus.addMacro(bind_key, json_payload)

    def del_macro(self, bind_key: str):
        key_map = keyboard.KEY_MAPPING
        map_str = "keyboard.KEY_MAPPING"
        if self.name in ["Razer Orbweaver", "Razer Orbweaver Chroma", "Razer Tartarus V2", "Razer Tartarus Chroma V2", "Razer Tartarus Pro"]:
            key_map = keyboard.ORBWEAVER_KEY_MAPPING
            map_str = "keyboard.ORBWEAVER_KEY_MAPPING"
        elif self.name in ["Razer Tartarus", "Razer Tartarus Chroma", "Razer Nostromo"]:
            key_map = keyboard.TARTARUS_KEY_MAPPING
            map_str = "keyboard.TARTARUS_KEY_MAPPING"
        elif self.name in ["Razer Naga Hex V2", "Razer Naga Chroma"]:
            key_map = keyboard.NAGA_HEX_V2_KEY_MAPPING
            map_str = "keyboard.NAGA_HEX_V2_KEY_MAPPING"

        if bind_key not in key_map:
            raise ValueError("Key {0} is not in openrazer.{1}".format(bind_key, map_str))
        else:
            self._macro_dbus.deleteMacro(bind_key)

    @property
    def mode_modifier(self):
        if 'macro_mode_modifier' in self._capabilities:
            return self._macro_dbus.getModeModifier()
        return False

    @mode_modifier.setter
    def mode_modifier(self, value):
        if 'macro_mode_modifier' in self._capabilities and isinstance(value, bool):
            self._macro_dbus.getModeModifier(value)

    @staticmethod
    def create_url_macro_item(url: str) -> _daemon_macro.MacroURL:
        """
        Create a macro object that opens a URL in a browser

        :param url: URL
        :type url: str

        :return: Macro object
        :rtype: _daemon_macro.MacroURL
        """
        return _daemon_macro.MacroURL(url)

    @staticmethod
    def create_script_macro_item(script_path: str, script_args: str = None) -> _daemon_macro.MacroScript:
        """
        Create a macro object that runs a script

        The arguments to the script should be a string containing all the arguments, if any values contain spaces they should be quoted accordingly
        :param script_path: Script filepath, includes script name
        :type script_path: str

        :param script_args: Script arguments
        :type script_args: str or None

        :return: Macro object
        :rtype: _daemon_macro.MacroScript
        """
        return _daemon_macro.MacroScript(script_path, script_args)

    @staticmethod
    def create_keypress_up_macro_item(key_name: str, pre_pause: int = 0) -> _daemon_macro.MacroKey:
        """
        Create a macro action that consists of a key release event

        :param key_name: Key Name, compatible with XTE
        :type key_name: str

        :param pre_pause: Optional delay before key is actioned (if turned on in daemon)
        :type pre_pause: int

        :return: Macro Key
        :rtype: _daemon_macro.MacroKey
        """
        return _daemon_macro.MacroKey(key_name, pre_pause, 'UP')

    @staticmethod
    def create_keypress_down_macro_item(key_name: str, pre_pause: int = 0) -> _daemon_macro.MacroKey:
        """
        Create a macro action that consists of a key press event

        :param key_name: Key Name, compatible with XTE
        :type key_name: str

        :param pre_pause: Optional delay before key is actioned (if turned on in daemon)
        :type pre_pause: int

        :return: Macro Key
        :rtype: _daemon_macro.MacroKey
        """
        return _daemon_macro.MacroKey(key_name, pre_pause, 'DOWN')

    @classmethod
    def create_keypress_macro_item(cls, key_name: str, pre_pause: int = 0) -> list:
        """
        Create a macro action that consists of a key press and release event

        The pre_pause delay will be applied to both key events
        :param key_name: Key Name, compatible with XTE
        :type key_name: str

        :param pre_pause: Optional delay before key is actioned (if turned on in daemon)
        :type pre_pause: int

        :return: Macro Key
        :rtype: list of _daemon_macro.MacroKey
        """
        return [cls.create_keypress_down_macro_item(key_name, pre_pause), cls.create_keypress_up_macro_item(key_name, pre_pause)]
