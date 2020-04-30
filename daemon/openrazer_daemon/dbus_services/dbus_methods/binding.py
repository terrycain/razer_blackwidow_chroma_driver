"""
Keyboard Binding methods
"""
import json
from openrazer_daemon.dbus_services import endpoint

@endpoint('razer.device.binding', 'getProfiles', out_sig='s')
def get_profiles(self):
    """
    Get profiles

    :return: JSON of profiles
    :rtype: str
    """
    self.logger.debug("DBus call get_profiles")

    return self.binding_manager.dbus_get_profiles()

@endpoint('razer.device.binding', 'getActiveProfile', out_sig='s')
def get_active_profile(self):
    """
    Get the active profile

    :return: Name of active profile
    :rtype: str
    """

    self.logger.debug("DBus call get_active_profile")

    return self.binding_manager.current_profile

@endpoint('razer.device.binding', 'setActiveProfile', in_sig='y')
def set_active_profile(self, profile):
    """
    Set the active profile

    :param profile: The profile number
    :type: int
    """

    self.logger.debug("DBus call set_active_profile")

    self.binding_manager.current_profile = profile

@endpoint('razer.device.binding', 'getMaps', in_sig='y', out_sig='s')
def get_maps(self, profile):
    """
    Get maps

    :param profile: The profile number
    :type: int

    :return: JSON of maps
    :rtype: str
    """

    self.logger.debug("DBus call get_maps")

    return self.binding_manager.dbus_get_maps(profile)


@endpoint('razer.device.binding', 'getMap', in_sig='ys', out_sig='s')
def get_map(self, profile, map):
    """
    Get map

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :return: JSON of maps
    :rtype: str
    """

    self.logger.debug("DBus call get_map")

    return self.binding_manager.dbus_get_map(profile, map)

@endpoint('razer.device.binding', 'getActiveMap', out_sig='s')
def get_active_map(self):
    """
    Get the active map

    :return: The active map
    :rtype: str
    """
    
    self.logger.debug("DBus call get_active_map")

    return self.binding_manager.current_mapping

@endpoint('razer.device.binding', 'setActiveMap', in_sig='s')
def set_active_map(self, map):
    """
    Set the active map

    :param map: The map name
    :type: str
    """

    self.logger.debug("DBus call set_active_map")

    self.binding_manager.current_mapping = map


@endpoint('razer.device.binding', 'getActions', in_sig='sss', out_sig='s')
def get_actions(self, profile, map, key_code):
    """
    Get profiles

    :param profile: The profile number
    :type: int

    :param map: The map number
    :type: str

    :param key_code: The key code
    :type: str

    :return: JSON of actions
    :rtype: str
    """
    self.logger.debug("DBus call get_actions")
  
    return self.binding_manager.dbus_get_actions(profile, map, key_code)

@endpoint('razer.device.binding', 'addMap', in_sig='ys')
def add_map(self, profile, map):
    """
    Add a map

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str
    """

    self.logger.debug("DBus call add_map")

    self.binding_manager.dbus_get_map(profile, map)

@endpoint('razer.device.binding', 'addAction', in_sig='yssss')
def add_action(self, profile, map, key_code, action_type, value):
    """
    Add an action to the given key

    :param profile: The profile number
    :type: int

    :param map: The map number
    :type: str

    :param key_code: The key code
    :type: str

    :param action_type: The action type
    :type: str

    :param value: The value
    :type: str
    """
    self.logger.debug("DBus call add_action")
  
    self.binding_manager.dbus_add_action(profile, map, key_code, action_type, value)

@endpoint('razer.device.binding', 'updateAction', in_sig='ysssss')
def update_action(self, profile, map, key_code, action_type, value, action_id):
    """
    Add an action to the given key

    :param profile: The profile number
    :type: int

    :param map: The map number
    :type: str

    :param key_code: The key code
    :type: str

    :param action_type: The action type
    :type: str

    :param value: The value
    :type: str

    :param action_id: The action to update
    :type: str
    """
    self.logger.debug("DBus call update_action")
  
    self.binding_manager.dbus_add_action(profile, map, key_code, action_type, value, action_id)

@endpoint('razer.device.binding', 'removeAction', in_sig='ysss')
def remove_action(self, profile, map, key_code, action_id):
    """
    Remove the specified action

    :param profile: The profile number
    :type: int

    :param map: The map number
    :type: str

    :param key_code: The key code
    :type: str

    :param action_id: The action id
    :type: str
    """
    self.logger.debug("DBus call remove_action")
  
    self.binding_manager.dbus_remove_action(profile, map, key_code, action_id)

@endpoint('razer.device.binding', 'clearActions', in_sig='yss')
def clear_actions(self, profile, map, key_code):
    """
    Clear all actions for a given key

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :param key_code: The key code
    :type: str
    """

    self.logger.debug("DBus call clear_actions")

    self.binding_manager.dbus_clear_actions(profile, map, key_code)

@endpoint('razer.device.binding.lighting', 'getProfileLEDs', in_sig='ys', out_sig='bbb')
def get_profile_leds(self, profile, map):
    """
    Get the state of the profile LEDs

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :return: The state of the Red profile LED
    :type: bool

    :return: The state of the Green profile LED
    :type: bool

    :return: The state of the Blue profile LED
    :type: bool
    """
    self.logger.debug("DBus call get_profile_leds")
    
    return self.binding_manager.dbus_get_profile_leds(profile, map) 


@endpoint('razer.device.binding.lighting', 'setProfileLEDs', in_sig='ysbbb')
def set_profile_leds(self, profile, map, red, green, blue):
    """
    Set the profile LED state

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :param red: The red LED state
    :type: bool

    :param green: The green LED state
    :type: bool

    :param blue: The blue LED state
    :type: bool
    """

    self.logger.debug("DBus call set_profile_leds")

    self.binding_manager.dbus_set_profile_leds(profile, map, red, green, blue)

@endpoint('razer.device.binding.lighting', 'getMatrix', in_sig='ys', out_sig='s')
def get_matrix(self, profile, map):
    """
    Get the led matrix of the specified map

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :return: JSON of matrix
    :rtype: str
    """

    self.logger.debug("DBus call get_matrix")

    return self.binding_manager.get_matrix(profile, map)

@endpoint('razer.device.binding.lighting', 'setMatrix', in_sig='yss')
def set_matrix(self, profile, map, matrix):
    """
    Set the led matrix

    :param profile: The profile number
    :type: int

    :param map: The map name
    :type: str

    :param matrix: The led matrix
    :type: str
    """

    self.logger.debug("DBus call set_matrix")

    self.binding_manager.dbus_set_matrix(profile, map, json.loads(matrix))

