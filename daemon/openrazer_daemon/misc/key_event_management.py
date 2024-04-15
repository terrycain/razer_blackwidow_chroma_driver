# SPDX-License-Identifier: GPL-2.0-or-later

"""
Receives events from /dev/input/by-id/somedevice

Events are 24 bytes long and are emitted from a character device in 24 byte chunks.
As the keyboards have "2" keyboard interfaces we need to listen on both of them in case some n00b
switches to game mode.

Each event is in the format of
* signed int of seconds
* signed int of microseconds
* unsigned short of event type
* unsigned short code
* signed int value
"""
import datetime
import fcntl
import json
import logging
import os
import random
import select
import struct
import threading
import time

# pylint: disable=import-error
from openrazer_daemon.keyboard import KEY_MAPPING, TARTARUS_KEY_MAPPING, EVENT_MAPPING, TARTARUS_EVENT_MAPPING, NAGA_HEX_V2_EVENT_MAPPING, NAGA_HEX_V2_KEY_MAPPING, ORBWEAVER_EVENT_MAPPING, ORBWEAVER_KEY_MAPPING
from .macro import MacroKey, MacroRunner, macro_dict_to_obj

EVENT_FORMAT = '@llHHI'
EVENT_SIZE = struct.calcsize(EVENT_FORMAT)

EPOLL_TIMEOUT = 0.01
SPIN_SLEEP = 0.01

EVIOCGRAB = 0x40044590

COLOUR_CHOICES = (
    (255, 0, 0),    # Red
    (0, 255, 0),    # Green
    (0, 0, 255),    # Blue
    (255, 255, 0),  # Yellow
    (0, 255, 255),  # Cyan
    (255, 0, 255),  # Magenta
)


def random_colour_picker(last_choice, iterable):
    """
    Chose a random choice but not the last one

    :param last_choice: Last choice
    :type last_choice: object

    :param iterable: Iterable object
    :type iterable: iterable

    :return: Choice
    :rtype: object
    """
    result = random.choice(iterable)
    while result == last_choice:
        result = random.choice(iterable)
    return result


class KeyWatcher(threading.Thread):
    """
    Thread to watch keyboard event files and return keypresses
    """
    @staticmethod
    def parse_event_record(data):
        """
        Parse Input event record

        :param data: Binary data
        :type data: bytes

        :return: Tuple of event time, key_action, key_code
        :rtype: tuple
        """
        # Event Seconds, Event Microseconds, Event Type, Event Code, Event Value
        ev_sec, ev_usec, ev_type, ev_code, ev_value = struct.unpack(EVENT_FORMAT, data)

        if ev_type != 0x01:  # input-event-codes.h EV_KEY 0x01
            return None, None, None

        if ev_value == 0:
            key_action = 'release'
        elif ev_value == 1:
            key_action = 'press'
        elif ev_value == 2:
            key_action = 'autorepeat'
        else:
            key_action = 'unknown'

        seconds = ev_sec + (ev_usec * 0.000001)
        date = datetime.datetime.fromtimestamp(seconds)

        result = (date, key_action, ev_code)

        if ev_type == ev_code == ev_value == 0:
            return None, None, None

        return result

    def __init__(self, device_id, event_files, parent, use_epoll=True):
        super().__init__()

        self._logger = logging.getLogger('razer.device{0}.keywatcher'.format(device_id))
        self._event_files = event_files
        self._shutdown = False
        self._use_epoll = use_epoll
        self._parent = parent

        self.open_event_files = [open(event_file, 'rb') for event_file in self._event_files]
        # Set open files to non blocking mode
        for event_file in self.open_event_files:
            flags = fcntl.fcntl(event_file.fileno(), fcntl.F_GETFL)
            fcntl.fcntl(event_file.fileno(), fcntl.F_SETFL, flags | os.O_NONBLOCK)

    def run(self):
        """
        Main event loop
        """
        # Create dict of Event File Descriptor: Event File Object
        event_file_map = {event_file.fileno(): event_file for event_file in self.open_event_files}

        # Create epoll object
        poll_object = select.epoll()

        # Register files with select
        for event_fd in event_file_map.keys():
            poll_object.register(event_fd, select.EPOLLIN | select.EPOLLPRI)

        # Loop
        while not self._shutdown:
            # epoll is nice but it wasn't getting events properly :(

            try:  # Cheap hack until i merged new code
                if self._use_epoll:
                    self._poll_epoll(poll_object, event_file_map)
                else:
                    self._poll_read()
            except (IOError, OSError):  # Basically if there's an error, most likely device has been removed then it'll get deleted properly
                pass

            time.sleep(SPIN_SLEEP)

        # Unbind files and close them
        for event_fd, event_file in event_file_map.items():
            poll_object.unregister(event_fd)
            event_file.close()

        poll_object.close()

    def _poll_epoll(self, poll_object, event_file_map):
        events = poll_object.poll(EPOLL_TIMEOUT)

        if len(events) != 0:
            # pylint: disable=unused-variable
            for event_fd, mask in events:
                while True:
                    key_data = event_file_map[event_fd].read(EVENT_SIZE)
                    if not key_data:
                        break

                    date, key_action, key_code = self.parse_event_record(key_data)

                    # Skip if date, key_action and key_code is none as that's a spacer record
                    if date is None:
                        continue

                    # Now if key is pressed then we record
                    self._parent.key_action(date, key_code, key_action)

    def _poll_read(self):
        for event_file in self.open_event_files:

            key_data = event_file.read(EVENT_SIZE)

            if key_data is None:
                continue

            date, key_action, key_code = self.parse_event_record(key_data)

            # Skip if date, key_action and key_code is none as that's a spacer record
            if date is None:
                continue

            # Now if key is pressed then we record
            self._parent.key_action(date, key_code, key_action)

    @property
    def shutdown(self):
        """
        Thread shutdown condition

        :return: Shutdown condition
        :rtype: bool
        """
        return self._shutdown

    @shutdown.setter
    def shutdown(self, value):
        """
        Set thread shutdown condition

        :param value: Boolean, normally only True would be used
        :type value: str
        """
        self._shutdown = value


class KeyboardKeyManager(object):
    """
    Key management class.

    This class deals with anything to do with keypresses. Currently it does:
    * Receiving keypresses from the KeyWatcher
    * Logic to deal with GameMode shortcut not working when macro's not enabled
    * Logic to deal with recording on the fly macros and replaying them

    It will be used to store keypresses in a list (for at most 2 seconds) if enabled for the ripple effect, when I
    get round to making the effect.
    """
    KEY_MAP = KEY_MAPPING
    EVENT_MAP = EVENT_MAPPING

    # pylint: disable=too-many-instance-attributes
    def __init__(self, device_id, event_files, parent, use_epoll=False, testing=False, should_grab_event_files=False):

        self._device_id = device_id
        self._logger = logging.getLogger('razer.device{0}.keymanager'.format(device_id))
        self._parent = parent
        self._parent.register_observer(self)
        self._testing = testing

        self._event_files = event_files
        self._access_lock = threading.Lock()
        self._keywatcher = KeyWatcher(device_id, event_files, self, use_epoll=use_epoll)
        self._open_event_files = self._keywatcher.open_event_files

        if len(event_files) > 0:
            self._logger.debug("Starting KeyWatcher")
            self._keywatcher.start()
        else:
            self._logger.warning("No event files for KeyWatcher")

        self._recording_macro = False
        self._macros = {}

        self._current_macro_bind_key = None
        self._current_macro_combo = []

        self._threads = set()
        self._clean_counter = 0

        self._temp_key_store_active = False
        self._temp_key_store = []
        self._temp_expire_time = datetime.timedelta(seconds=2)

        self._last_colour_choice = None

        self._should_grab_event_files = should_grab_event_files
        self._event_files_locked = False

        if self._should_grab_event_files:
            self.grab_event_files(True)

    @property
    def temp_key_store(self):
        """
        Get the temporary key store

        :return: List of keys
        :rtype: list
        """
        # Locking so it doesn't mutate whilst copying
        self._access_lock.acquire()
        now = datetime.datetime.now()

        # Remove expired keys from store
        try:
            # Get date and if its less than now its expired
            while self._temp_key_store[0][0] < now:
                self._temp_key_store.pop(0)
        except IndexError:
            pass

        # Creating a copy as then it doesn't mutate whilst iterating
        result = self._temp_key_store[:]
        self._access_lock.release()
        return result

    @property
    def temp_key_store_state(self):
        """
        Get the state of the temporary key store

        :return: Active state
        :rtype: bool
        """
        return self._temp_key_store_active

    @temp_key_store_state.setter
    def temp_key_store_state(self, value):
        """
        Set the state of the temporary key store

        :param value: Active state
        :type value: bool
        """
        self._temp_key_store_active = value

    def grab_event_files(self, grab):
        """
        Grab the event files exclusively

        :param grab: True to grab, False to release
        :type grab: bool
        """
        if not self._testing:
            for event_file in self._open_event_files:
                fcntl.ioctl(event_file.fileno(), EVIOCGRAB, int(grab))
        self._event_files_locked = grab

    def key_action(self, event_time, key_id, key_press='press'):
        """
        Process a key press event

        Ok an attempt to explain the logic
        * The function sets a value _fn_down depending on the state of FN.
        * Adds keypress and release events to a macro list if recording a macro.
        * Pressing FN+F9 starts recording a macro, then selecting any key marks that as a macro key,
          then it will record keys, then pressing FN+F9 will save macro.
        * Pressing any macro key will run macro.
        * Pressing FN+F10 will toggle game mode.
        :param event_time: Time event occurred
        :type event_time: datetime.datetime

        :param key_id: Key Event ID
        :type key_id: int

        :param key_press: Can either be press, release, autorepeat
        :type key_press: bool
        """
        # Disable pylints complaining for this part, #PerformanceOverNeatness
        # pylint: disable=too-many-branches,too-many-statements

        # Get event files if they arnt locked #nasty hack
        if not self._event_files_locked and self._should_grab_event_files:
            self.grab_event_files(True)

        if key_press == 'autorepeat':  # TODO not done right yet
            # If its brightness then convert autorepeat to key presses
            # Brightness keys are defined in keyboard.py and razerkbd_driver.c
            if key_id in (0x2ab, 0x2aa):
                key_press = 'press'
            else:
                # Quit out early
                return

        now = datetime.datetime.now()

        # Remove expired keys from store
        try:
            # Get date and if its less than now its expired
            while self._temp_key_store[0][0] < now:
                self._temp_key_store.pop(0)
        except IndexError:
            pass

        # Clean up any threads
        if self._clean_counter > 20 and len(self._threads) > 0:
            self._clean_counter = 0
            self.clean_macro_threads()

        try:
            # Convert event ID to key name
            key_name = self.EVENT_MAP[key_id]

            # self._logger.info("Got key: {0}, state: {1}".format(key_name, 'DOWN' if key_press else 'UP'))

            # Key release
            if key_press == 'release':
                if self._recording_macro:
                    # Skip as don't care about releasing macro bind key
                    if key_name not in (self._current_macro_bind_key, 'MACROMODE'):
                        # Record key release events
                        self._current_macro_combo.append((event_time, key_name, 'UP'))

            else:
                # Key press

                if self._temp_key_store_active:
                    colour = random_colour_picker(self._last_colour_choice, COLOUR_CHOICES)
                    self._last_colour_choice = colour
                    self._temp_key_store.append((now + self._temp_expire_time, self.KEY_MAP[key_name], colour))

                # Macro FN+F9 logic
                if key_name == 'MACROMODE':
                    self._logger.info("Got macro combo")

                    if not self._recording_macro:
                        # Starting to record macro
                        self._recording_macro = True
                        self._current_macro_bind_key = None
                        self._current_macro_combo = []

                        self._parent.setMacroEffect(0x01)
                        self._parent.setMacroMode(True)

                    else:
                        self._logger.debug("Finished recording macro")
                        # Finish recording macro
                        if self._current_macro_bind_key is not None:
                            if len(self._current_macro_combo) > 0:
                                self.add_kb_macro()
                            else:
                                # Clear macro
                                self.dbus_delete_macro(self._current_macro_bind_key)
                        self._recording_macro = False
                        self._parent.setMacroEffect(0x00)
                        self._parent.setMacroMode(False)
                # Sets up game mode as when enabling macro keys it stops the key working
                elif key_name == 'GAMEMODE':
                    self._logger.info("Got game mode combo")

                    game_mode = self._parent.getGameMode()
                    self._parent.setGameMode(not game_mode)

                # Brightness logic
                elif key_name == 'BRIGHTNESSDOWN':
                    # Get brightness value
                    current_brightness = self._parent.method_args.get('brightness', None)
                    if current_brightness is None:
                        current_brightness = self._parent.getBrightness()

                    if current_brightness > 0:
                        current_brightness -= 10
                        if current_brightness < 0:
                            current_brightness = 0

                        self._parent.setBrightness(current_brightness)
                        # self._parent.method_args['brightness'] = current_brightness
                elif key_name == 'BRIGHTNESSUP':
                    # Get brightness value
                    current_brightness = self._parent.method_args.get('brightness', None)
                    if current_brightness is None:
                        current_brightness = self._parent.getBrightness()

                    if current_brightness < 100:
                        current_brightness += 10
                        if current_brightness > 100:
                            current_brightness = 100

                        self._parent.setBrightness(current_brightness)
                        # self._parent.method_args['brightness'] = current_brightness

                elif self._recording_macro:

                    if self._current_macro_bind_key is None:
                        # Restrict macro bind keys to M1-M5
                        if key_name not in ('M1', 'M2', 'M3', 'M4', 'M5', 'M6'):
                            self._logger.warning("Macros are only for M1-M6 for now.")
                            self._recording_macro = False
                            self._parent.setMacroMode(False)
                        else:
                            self._current_macro_bind_key = key_name
                            self._parent.setMacroEffect(0x00)
                    # Don't want no recursion, cancel macro, don't let one call macro in a macro
                    elif key_name == self._current_macro_bind_key:
                        self._logger.warning("Skipping macro assignment as would cause recursion")
                        self._recording_macro = False
                        self._parent.setMacroMode(False)
                    # Anything else just record it
                    else:
                        self._current_macro_combo.append((event_time, key_name, 'DOWN'))
                # Not recording anything so if a macro key is pressed then run
                else:
                    # If key has a macro, play it
                    if key_name in self._macros:
                        self.play_macro(key_name)

        except KeyError as err:
            self._logger.exception("Got key error. Couldn't convert event to key name", exc_info=err)

    def add_kb_macro(self):
        """
        Tidy up the recorded macro and add it to the store

        Goes through the macro and generated relative delays between key events
        """
        new_macro = []

        start_time = self._current_macro_combo[0][0]
        for event_time, key, state in self._current_macro_combo:
            delay = (event_time - start_time).microseconds
            start_time = event_time
            new_macro.append(MacroKey(key, delay, state))

        self._macros[self._current_macro_bind_key] = new_macro

    def clean_macro_threads(self):
        """
        Threadless-threadpool

        Goes though the threads (macro play jobs) and removed the threads if they have finished.
        #SetMagic
        """
        self._logger.debug("Cleaning up macro threads")
        to_remove = set()

        for macro_thread in self._threads:
            macro_thread.join(timeout=0.05)
            if not macro_thread.is_alive():
                to_remove.add(macro_thread)

        self._threads -= to_remove

    def play_macro(self, macro_key):
        """
        Play macro for a given key

        Launches a thread and adds it to the pool
        :param macro_key: Macro Key
        :type macro_key: str
        """
        self._logger.info("Running Macro %s:%s", macro_key, str(self._macros[macro_key]))
        macro_thread = MacroRunner(self._device_id, macro_key, self._macros[macro_key])
        macro_thread.start()
        self._threads.add(macro_thread)

    # Methods to be used with DBus
    def dbus_delete_macro(self, key_name):
        """
        Delete a macro from a key

        :param key_name: Key Name
        :type key_name: str
        """
        try:
            del self._macros[key_name]
        except KeyError:
            pass

    def dbus_get_macros(self):
        """
        Get macros in JSON format

        Returns a JSON blob of all active macros in the format of
        {BIND_KEY: [MACRO_DICT...]}

        MACRO_DICT is a dict representation of an action that can be performed. The dict will have a
        type key which determines what type of action it will perform.
        For example there are key press macros, URL opening macros, Script running macros etc...
        :return: JSON of macros
        :rtype: str
        """
        result_dict = {}
        for macro_key, macro_combo in self._macros.items():
            str_combo = [value.to_dict() for value in macro_combo]
            result_dict[macro_key] = str_combo

        return json.dumps(result_dict)

    def dbus_add_macro(self, macro_key, macro_json):
        """
        Add macro from JSON

        The macro_json will be a list of macro objects which is then converted into JSON
        :param macro_key: Macro bind key
        :type macro_key: str

        :param macro_json: Macro JSON
        :type macro_json: str
        """
        macro_list = [macro_dict_to_obj(macro_object_dict) for macro_object_dict in json.loads(macro_json)]
        self._macros[macro_key] = macro_list

    def close(self):
        """
        Cleanup function
        """
        if self._keywatcher.is_alive():
            self._parent.remove_observer(self)

            self._logger.debug("Stopping key manager")
            self._keywatcher.shutdown = True
            self._keywatcher.join(timeout=2)
            if self._keywatcher.is_alive():
                self._logger.error("Could not stop KeyWatcher thread")

    def __del__(self):
        self.close()

    def notify(self, msg):
        """
        Receive notificatons from the device (we only care about effects)

        :param msg: Notification
        :type msg: tuple
        """
        if not isinstance(msg, tuple):
            self._logger.warning("Got msg that was not a tuple")
        elif msg[0] == 'effect':
            # We have a message directed at us
            # MSG format
            #  0         1       2             3
            # ('effect', Device, 'effectName', 'effectparams'...)
            # Device is the device the msg originated from (could be parent device)
            if msg[2] != 'setRipple':
                # If we are not doing ripple effect then disable the storing of keys
                # self.temp_key_store_state = False
                pass


class NagaHexV2KeyManager(KeyboardKeyManager):
    KEY_MAP = NAGA_HEX_V2_KEY_MAPPING
    EVENT_MAP = NAGA_HEX_V2_EVENT_MAPPING


class GamepadKeyManager(KeyboardKeyManager):
    GAMEPAD_EVENT_MAPPING = TARTARUS_EVENT_MAPPING
    GAMEPAD_KEY_MAPPING = TARTARUS_KEY_MAPPING

    def __init__(self, device_id, event_files, parent, use_epoll=True, testing=False):
        super().__init__(device_id, event_files, parent, use_epoll, testing=testing)

        self._mode_modifier = False
        self._mode_modifier_combo = []
        self._mode_modifier_key_down = False

    def key_action(self, event_time, key_id, key_press=True):
        """
        Process a key press event

        Ok an attempt to explain the logic
        * The function sets a value _fn_down depending on the state of FN.
        * Adds keypress and release events to a macro list if recording a macro.
        * Pressing FN+F9 starts recording a macro, then selecting any key marks that as a macro key,
          then it will record keys, then pressing FN+F9 will save macro.
        * Pressing any macro key will run macro.
        * Pressing FN+F10 will toggle game mode.
        :param event_time: Time event occurred
        :type event_time: datetime.datetime

        :param key_id: Key Event ID
        :type key_id: int

        :param key_press: If true then its a press, else its a release
        :type key_press: bool
        """
        # Disable pylints complaining for this part, #PerformanceOverNeatness
        # pylint: disable=too-many-branches,too-many-statements
        self._access_lock.acquire()

        if not self._event_files_locked:
            self.grab_event_files(True)

        now = datetime.datetime.now()

        # Remove expired keys from store
        try:
            # Get date and if its less than now its expired
            while self._temp_key_store[0][0] < now:
                self._temp_key_store.pop(0)
        except IndexError:
            pass

        # Clean up any threads
        if self._clean_counter > 20 and len(self._threads) > 0:
            self._clean_counter = 0
            self.clean_macro_threads()

        try:
            # Convert event ID to key name

            key_name = self.GAMEPAD_EVENT_MAPPING[key_id]
            # Key press

            if self._temp_key_store_active:
                colour = random_colour_picker(self._last_colour_choice, COLOUR_CHOICES)
                self._last_colour_choice = colour
                self._temp_key_store.append((now + self._temp_expire_time, self.GAMEPAD_KEY_MAPPING[key_name], colour))

            # if self._testing:
            # if key_press:
                # self._logger.debug("Got Key: {0} Down".format(key_name))
            # else:
                # self._logger.debug("Got Key: {0} Up".format(key_name))

            # Logic for mode switch modifier
            if self._mode_modifier:
                if key_name == 'MODE_SWITCH' and key_press:
                    # Start the macro string
                    self._mode_modifier_key_down = True
                    self._mode_modifier_combo.clear()
                    self._mode_modifier_combo.append('MODE')

                elif key_name == 'MODE_SWITCH' and not key_press:
                    # Release mode_switch
                    self._mode_modifier_key_down = False

                elif key_press and self._mode_modifier_key_down:
                    # Any keys pressed whilst mode_switch is down
                    self._mode_modifier_combo.append(key_name)

                    # Override keyname so it now equals a macro
                    key_name = '+'.join(self._mode_modifier_combo)

            self._logger.debug("Macro String: {0}".format(key_name))

            if key_name in self._macros and key_press:
                self.play_macro(key_name)

        except KeyError as err:
            self._logger.exception("Got key error. Couldn't convert event to key name", exc_info=err)

        self._access_lock.release()

    @property
    def mode_modifier(self):
        """
        Get if the MODE_SWTICH key is to act as a modifier

        :return: True if a modifier, false if not
        :rtype: bool
        """
        return self._mode_modifier

    @mode_modifier.setter
    def mode_modifier(self, value):
        """
        Set MODE_SWITCH modifier state

        :param value: Modifier state
        :type value: bool
        """
        self._mode_modifier = True if value else False


class OrbweaverKeyManager(GamepadKeyManager):
    GAMEPAD_EVENT_MAPPING = ORBWEAVER_EVENT_MAPPING
    GAMEPAD_KEY_MAPPING = ORBWEAVER_KEY_MAPPING
