# SPDX-License-Identifier: GPL-2.0-or-later

"""
Module to handle custom colours
"""

import struct
import subprocess


KEY_MAPPING = {
    # Row 0
    'M6': (0, 0), 'ESC': (0, 1), 'F1': (0, 3), 'F2': (0, 4), 'F3': (0, 5), 'F4': (0, 6), 'F5': (0, 7), 'F6': (0, 8), 'F7': (0, 9), 'F8': (0, 10), 'F9': (0, 11), 'F10': (0, 12), 'F11': (0, 13), 'F12': (0, 14), 'PRTSCR': (0, 15), 'SCRLK': (0, 16), 'PAUSE': (0, 17), 'LOGO': (0, 20), 'JP1': (0, 21),
    # Row 1
    'M1': (1, 0), 'BACKTICK': (1, 1), '1': (1, 2), '2': (1, 3), '3': (1, 4), '4': (1, 5), '5': (1, 6), '6': (1, 7), '7': (1, 8), '8': (1, 9), '9': (1, 10), '0': (1, 11), 'DASH': (1, 12), 'EQUALS': (1, 13), 'BACKSPACE': (1, 14), 'INS': (1, 15), 'HOME': (1, 16), 'PAGEUP': (1, 17), 'NUMLK': (1, 18), 'NPFORWARDSLASH': (1, 19), 'NPASTERISK': (1, 20), 'NPDASH': (1, 21),
    # Row 2
    'M2': (2, 0), 'TAB': (2, 1), 'Q': (2, 2), 'W': (2, 3), 'E': (2, 4), 'R': (2, 5), 'T': (2, 6), 'Y': (2, 7), 'U': (2, 8), 'I': (2, 9), 'O': (2, 10), 'P': (2, 11), 'LEFTSQUAREBRACKET': (2, 12), 'RIGHTSQUAREBRACKET': (2, 13), 'DELETE': (2, 15), 'END': (2, 16), 'PAGEDOWN': (2, 17), 'NP7': (2, 18), 'NP8': (2, 19), 'NP9': (2, 20), 'NPPLUS': (2, 21),
    # Row 3
    'M3': (3, 0), 'CAPSLK': (3, 1), 'A': (3, 2), 'S': (3, 3), 'D': (3, 4), 'F': (3, 5), 'G': (3, 6), 'H': (3, 7), 'J': (3, 8), 'K': (3, 9), 'L': (3, 10), 'SEMICOLON': (3, 11), 'APOSTROPHE': (3, 12), 'POUNDSIGN': (3, 13), 'RETURN': (3, 14), 'NP4': (3, 18), 'NP5': (3, 19), 'NP6': (3, 20),
    # Row 4
    'M4': (4, 0), 'LEFTSHIFT': (4, 1), 'BACKSLASH': (4, 2), 'Z': (4, 3), 'X': (4, 4), 'C': (4, 5), 'V': (4, 6), 'B': (4, 7), 'N': (4, 8), 'M': (4, 9), 'COMMA': (4, 10), 'PERIOD': (4, 11), 'FORWARDSLASH': (4, 12), 'JP2': (4, 13), 'RIGHTSHIFT': (4, 14), 'UPARROW': (4, 16), 'NP1': (4, 18), 'NP2': (4, 19), 'NP3': (4, 20), 'ENTER': (4, 21),
    # Row 5
    'M5': (5, 0), 'LEFTCTRL': (5, 1), 'SUPER': (5, 2), 'LEFTALT': (5, 3), 'SPACE': (5, 7), 'RIGHTALT': (5, 11), 'FN': (5, 12), 'CTXMENU': (5, 13), 'RIGHTCTRL': (5, 14), 'LEFTARROW': (5, 15), 'DOWNARROW': (5, 16), 'RIGHTARROW': (5, 17), 'NP0': (5, 19), 'NPPERIOD': (5, 20),

    # Additional mappings
    'MACROMODE': (0, 11), 'GAMEMODE': (0, 12), 'MUTE': (0, 3), 'VOL_DOWN': (0, 4), 'VOL_UP': (0, 5), 'MEDIA_BACK': (0, 7), 'MEDIA_PLAY': (0, 8), 'MEDIA_FORWARD': (0, 9), 'BRIGHTNESSDOWN': (0, 13), 'BRIGHTNESSUP': (0, 14)
}


#      0   1   2   3   4    5   6   7
#    ------------------------------------
# 0 |  1   2   3   4   5        UP
# 1 |  6   7   8   9   10  LEFT    RIGHT
# 2 |  11  12  13  14  15  TMB DOWN MS
#
#
#

TARTARUS_KEY_MAPPING = {
    '1': (0, 0),
    '2': (0, 1),
    '3': (0, 2),
    '4': (0, 3),
    '5': (0, 4),
    'UP': (0, 6),

    '6': (1, 0),
    '7': (1, 1),
    '8': (1, 2),
    '9': (1, 3),
    '10': (1, 4),
    'LEFT': (1, 5),
    'RIGHT': (1, 7),

    '11': (2, 0),
    '12': (2, 1),
    '13': (2, 2),
    '14': (2, 3),
    '15': (2, 4),
    'THUMB': (2, 5),
    'DOWN': (2, 6),
    'MODE_SWITCH': (2, 7)

}

#      0   1   2   3   4    5   6   7
#    ------------------------------------
# 0 |  1   2   3   4   5        UP
# 1 |  6   7   8   9   10  LEFT    RIGHT
# 2 |  11  12  13  14  15  TMB DOWN MS
# 3 |  16  17  18  19  20
#

ORBWEAVER_KEY_MAPPING = {
    '1': (0, 0),
    '2': (0, 1),
    '3': (0, 2),
    '4': (0, 3),
    '5': (0, 4),
    'UP': (0, 6),

    '6': (1, 0),
    '7': (1, 1),
    '8': (1, 2),
    '9': (1, 3),
    '10': (1, 4),
    'LEFT': (1, 5),
    'RIGHT': (1, 7),

    '11': (2, 0),
    '12': (2, 1),
    '13': (2, 2),
    '14': (2, 3),
    '15': (2, 4),
    'THUMB': (2, 5),
    'DOWN': (2, 6),
    'MODE_SWITCH': (2, 7),

    '16': (3, 0),
    '17': (3, 1),
    '18': (3, 2),
    '19': (3, 3),
    '20': (3, 4)
}

# Naga hex buttons are normal 1-7 keys
#      0 1 2 3 4
#   --------------
# 0 |    1   2
# 1 |  4       3
# 2 |    5   7
# 3 |      6
NAGA_HEX_V2_KEY_MAPPING = {
    '1': (0, 1), '2': (0, 3),
    '4': (1, 0), '3': (1, 4),
    '5': (2, 1), '7': (2, 3),
    '6': (3, 2)
}

#
EVENT_MAPPING = {
    1: 'ESC', 2: '1', 3: '2', 4: '3', 5: '4', 6: '5', 7: '6', 8: '7', 9: '8',
    10: '9', 11: '0', 12: 'DASH', 13: 'EQUALS', 14: 'BACKSPACE', 15: 'TAB', 16: 'Q', 17: 'W', 18: 'E', 19: 'R',
    20: 'T', 21: 'Y', 22: 'U', 23: 'I', 24: 'O', 25: 'P', 26: 'LEFTSQUAREBRACKET', 27: 'RIGHTSQUAREBRACKET', 28: 'RETURN', 29: 'LEFTCTRL',
    30: 'A', 31: 'S', 32: 'D', 33: 'F', 34: 'G', 35: 'H', 36: 'J', 37: 'K', 38: 'L', 39: 'SEMICOLON',
    40: 'APOSTROPHE', 41: 'BACKTICK', 42: 'LEFTSHIFT', 43: 'POUNDSIGN', 44: 'Z', 45: 'X', 46: 'C', 47: 'V', 48: 'B', 49: 'N',
    50: 'M', 51: 'COMMA', 52: 'PERIOD', 53: 'FORWARDSLASH', 54: 'RIGHTSHIFT', 55: 'NPASTERISK', 56: 'LEFTALT', 57: 'SPACE',
    58: 'CAPSLK', 59: 'F1',
    60: 'F2', 61: 'F3', 62: 'F4', 63: 'F5', 64: 'F6', 65: 'F7', 66: 'F8', 67: 'F9', 68: 'F10', 69: 'NUMLK',
    70: 'SCRLK', 71: 'NP7', 72: 'NP8', 73: 'NP9', 74: 'NPDASH', 75: 'NP4', 76: 'NP5', 77: 'NP6', 78: 'NPPLUS', 79: 'NP1',
    80: 'NP2', 81: 'NP3', 82: 'NP0', 83: 'NPPERIOD', 86: 'BACKSLASH', 87: 'F11', 88: 'F12',
    96: 'ENTER', 97: 'RIGHTCTRL', 98: 'NPFORWARDSLASH', 99: 'PRTSCR',
    100: 'RIGHTALT', 102: 'HOME', 103: 'UPARROW', 104: 'PAGEUP', 105: 'LEFTARROW', 106: 'RIGHTARROW', 107: 'END', 108: 'DOWNARROW', 109: 'PAGEDOWN', 110: 'INS',
    111: 'DELETE', 113: 'MUTE', 114: 'VOL_DOWN', 115: 'VOL_UP', 119: 'PAUSE',
    125: 'SUPER', 127: 'CTXMENU',
    163: 'MEDIA_FORWARD', 164: 'MEDIA_PLAY', 165: 'MEDIA_BACK',
    183: 'M1', 184: 'M2', 185: 'M3', 186: 'M4', 187: 'M5', 188: 'M6',
    189: 'M7', 190: 'M8', 191: 'M9', 192: 'M10', 193: 'M11', 194: 'M12',
    0x2ad: 'MACROMODE', 0x2ac: 'GAMEMODE', 0x2ab: 'BRIGHTNESSDOWN', 0x2aa: 'BRIGHTNESSUP'
}

TARTARUS_EVENT_MAPPING = {
    15: '1',
    16: '2',
    17: '3',
    18: '4',
    19: '5',
    58: '6',
    30: '7',
    31: '8',
    32: '9',
    33: '10',
    42: '11',
    44: '12',
    45: '13',
    46: '14',
    47: '15',
    56: 'MODE_SWITCH',
    57: 'THUMB',
    103: 'UP',
    105: 'LEFT',
    106: 'RIGHT',
    108: 'DOWN',
}

ORBWEAVER_EVENT_MAPPING = {
    41: '1',
    2: '2',
    3: '3',
    4: '4',
    5: '5',
    15: '6',
    16: '7',
    17: '8',
    18: '9',
    19: '10',
    58: '11',
    30: '12',
    31: '13',
    32: '14',
    33: '15',
    42: '16',
    44: '17',
    45: '18',
    46: '19',
    47: '20',
    56: 'MODE_SWITCH',
    57: 'THUMB',
    103: 'UP',
    105: 'LEFT',
    106: 'RIGHT',
    108: 'DOWN',
}

NAGA_HEX_V2_EVENT_MAPPING = {
    2: '1', 3: '2', 4: '3', 5: '4', 6: '5', 7: '6', 8: '7'
}

XTE_MAPPING = {
    'ESC': 'Escape',
    'DASH': 'minus',
    'EQUALS': 'equal',
    'BACKSPACE': 'BackSpace',
    'TAB': 'Tab',
    'LEFTSQUAREBRACKET': 'bracketleft',
    'RIGHTSQUAREBRACKET': 'bracketright',
    'RETURN': 'Return',
    'LEFTCTRL': 'Control_L',
    'SEMICOLON': 'semicolon',
    'APOSTROPHE': 'apostrophe',
    'BACKTICK': 'grave',
    'LEFTSHIFT': 'Shift_L',
    'POUNDSIGN': 'numbersign',
    'COMMA': 'comma',
    'PERIOD': 'period',
    'FORWARDSLASH': 'slash',
    'RIGHTSHIFT': 'Shift_R',
    'NPASTERISK': 'KP_Multiply',
    'LEFTALT': 'Alt_L',
    'SPACE': 'space',
    'CAPSLK': 'Caps_Lock',
    'NUMLK': 'Num_Lock',
    'SCRLK': 'Scroll_Lock',
    'PAUSE': 'Pause',
    'NP7': '7',
    'NP8': '8',
    'NP9': '9',
    'NPDASH': 'KP_Subtract',
    'NP4': '4',
    'NP5': '5',
    'NP6': '6',
    'NPPLUS': 'KP_Add',
    'NP1': '1',
    'NP2': '2',
    'NP3': '3',
    'NP0': '0',
    'PRTSCR': 'Print',
    'NPPERIOD': 'KP_Decimal',
    'BACKSLASH': 'backslash',
    'ENTER': 'Enter',
    'RIGHTCTRL': 'Control_r',
    'NPFORWARDSLASH': 'KP_Divide',
    'RIGHTALT': 'Alt_R',
    'HOME': 'Home',
    'UPARROW': 'Up',
    'PAGEUP': 'Page_Up',
    'LEFTARROW': 'Left',
    'RIGHTARROW': 'Right',
    'END': 'End',
    'DOWNARROW': 'Down',
    'PAGEDOWN': 'Page_Down',
    'INS': 'Insert',
    'DELETE': 'Delete',
    'SUPER': 'Super_L',
    'CTXMENU': 'Menu',
    'M1': 'XF86Tools',
    'M2': 'XF86Launch5',
    'M3': 'XF86Launch6',
    'M4': 'XF86Launch7',
    'M5': 'XF86Launch8',
    'M6': 'XF86Launch9',
    'FN': None,
    'GAMEMODE': None,
    'MACROMODE': None,
}


class KeyDoesNotExistError(Exception):
    """
    Simple custom error
    """
    pass


class NoBackupError(Exception):
    pass


class RGB(object):

    @staticmethod
    def clamp(value):
        """
        Clamp a value to 0-255

        :param value: Value to be clamped
        :type value: integer or float

        :return: Integer in the range of 0-255
        :rtype: int
        """
        result = int(value)

        if value > 255:
            result = 255
        elif value < 0:
            result = 0

        return result

    def __init__(self, red=0, green=0, blue=0):
        self._red = red
        self._green = green
        self._blue = blue

    @property
    def red(self):
        """
        Getter for red element

        :return: Red element
        :rtype: int
        """
        return self._red

    @red.setter
    def red(self, value):
        """
        Setter for red value

        :param value: Red value
        :type value: int or float
        """
        self._red = RGB.clamp(value)

    @property
    def green(self):
        """
        Getter for green element

        :return: Green element
        :rtype: int
        """
        return self._green

    @green.setter
    def green(self, value):
        """
        Setter for green value

        :param value: Green value
        :type value: int or float
        """
        self._green = RGB.clamp(value)

    @property
    def blue(self):
        """
        Getter for blue element

        :return: Blue element
        :rtype: int
        """
        return self._blue

    @blue.setter
    def blue(self, value):
        """
        Setter for blue value

        :param value: Blue value
        :type value: int or float
        """
        self._blue = RGB.clamp(value)

    def set(self, colour_tuple):
        """
        Sets all the colours at once

        :param colour_tuple: Tuple of R,G,B elements
        :type colour_tuple: tuple
        """
        # Shortcut to clamp all parameters and assign to the 3 variables
        self._red, self._green, self._blue = list(map(RGB.clamp, colour_tuple))

    def get(self):
        """
        Gets all the colours as a tuple

        :return: RGB tuple
        :rtype: tuple
        """
        return self._red, self._green, self._blue

    def __bytes__(self):
        """
        Convert to bytes

        :return: Byte string
        :rtype: bytearray
        """
        return bytes((self._red, self._green, self._blue))

    def __repr__(self):
        """
        String representation

        :return: String
        :rtype: str
        """
        return "RGB Object (#{0:02X}{1:02X}{2:02X})".format(self._red, self._green, self._blue)


class KeyboardColour(object):
    """
    Keyboard class which represents the colour state of the keyboard.
    """

    def __init__(self, rows, columns):
        self.rows = rows
        self.columns = columns

        # Two-dimensional array to hold color data
        self.colors = []

        # Backup object (currently not used)
        self.backup = None

        # Initialize array with empty values
        self.reset_rows()

    def backup_configuration(self):
        """
        Backs up the current configuration
        """
        self.backup = KeyboardColour(self.rows, self.columns)
        self.backup.get_from_total_binary(self.get_total_binary())

    def restore_configuration(self):
        """
        Restores the previous configuration
        """
        if self.backup is None:
            raise NoBackupError()

        self.colors = self.backup.colors
        self.backup = None

    def get_rows_raw(self):
        """
        Gets the raw representation of the rows

        :return: Rows
        :rtype: list
        """
        return self.colors

    def reset_rows(self):
        """
        Reset the rows of the keyboard
        """
        self.colors.clear()

        for row in range(0, self.rows):
            # Create 22 rgb values
            self.colors.append([RGB() for _ in range(0, self.columns)])

    def set_key_colour(self, row, col, colour):
        """
        Set the colour of a key

        :param row: Row ID
        :type row: int

        :param col: Column ID
        :type col: int

        :param colour: Colour to set
        :type colour: tuple

        :raises KeyDoesNotExistError: If given key does not exist
        """
        self.colors[row][col].set(colour)

    def get_key_colour(self, key):
        """
        Get the colour of a key

        :param key: Key to set the colour of
        :type key: str

        :raises KeyDoesNotExistError: If given key does not exist
        """
        if key not in KEY_MAPPING:
            raise KeyDoesNotExistError("The key \"{0}\" does not exist".format(key))

        row_id, col_id = KEY_MAPPING[key]
        return self.colors[row_id][col_id].get()

    def reset_key(self, row, col):
        """
        Reset the colour of a key

        :param row: Row ID
        :type row: int

        :param col: Column ID
        :type col: int

        :raises KeyDoesNotExistError: If given key does not exist
        """
        self.colors[row][col].set((0, 0, 0))

    def get_row_binary(self, row_id):
        """
        Gets the binary payload for a given row

        :param row_id: Row ID
        :type row_id: int

        :return: Byte string of 67 bytes, Row ID byte then 22 RGB bytes
        :rtype: bytearray
        """
        assert isinstance(row_id, int), "Row ID is not an int"

        payload = bytes([row_id, 0x00, len(self.colors[row_id]) - 1])

        for rgb in self.colors[row_id]:
            payload += bytes(rgb)

        return payload

    def get_total_binary(self):
        """
        Gets the binary payload for the whole keyboard

        :return: Byte string of 6*67 bytes, (Row ID byte then 22 RGB bytes) * 6
        :rtype: bytearray
        """
        payload = b''

        for row in range(0, self.rows):
            payload += self.get_row_binary(row)

        return payload

    def get_from_total_binary(self, binary_blob):
        """
        Load in a binary blob which is the output from get_total_binary

        :param binary_blob: Binary blob
        :type binary_blob: bytearray
        """
        self.reset_rows()

        for row_id in range(0, self.rows):
            binary_blob = binary_blob[1:]  # Skip first byte

            for col_id, binary_rgb in enumerate([binary_blob[i:i + 3] for i in range(0, 66, 3)]):
                rgb = struct.unpack('=BBB', binary_rgb)
                self.colors[row_id][col_id].set(rgb)

            binary_blob = binary_blob[66:]  # Skip the current row


def get_keyboard_layout():
    """
    Function to get the keyboard layout

    I can see this becoming an ugly mess as it'll need to support multiple keyboard layouts which have different names
    on different systems

    :return: Keyboard layout
    :rtype: str
    """
    cmd = ['setxkbmap', '-query']
    output = subprocess.check_output(cmd)

    result = 'gb'

    if output:
        output = output.decode('utf-8').splitlines()
        layout = 'gb'
        variant = ''
        for line in output:
            if line.startswith('layout'):
                layout = line.split(':', 1)[1].strip()
                if layout.find(',') > -1:
                    layout = layout.split(',')[0]

            elif line.startswith('variant'):
                variant = line.split(':', 1)[1].strip().split(',')[0]

                if 'latin9' in variant:  # Removes some rubbish from ubuntu
                    variant = 'latin9'

        if variant == '':
            result = layout
        else:
            result = layout + '-' + variant

    # If the user has an international layout variant ignore that part
    result = result.replace('-altgr-intl', '')

    return result
