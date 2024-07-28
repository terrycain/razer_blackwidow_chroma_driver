# SPDX-License-Identifier: GPL-2.0-or-later

"""
Keyboards class
"""
import re

from openrazer_daemon.hardware.device_base import RazerDeviceBrightnessSuspend as _RazerDeviceBrightnessSuspend
from openrazer_daemon.misc.key_event_management import KeyboardKeyManager as _KeyboardKeyManager, GamepadKeyManager as _GamepadKeyManager, OrbweaverKeyManager as _OrbweaverKeyManager
from openrazer_daemon.misc.ripple_effect import RippleManager as _RippleManager


class _MacroKeyboard(_RazerDeviceBrightnessSuspend):
    """
    Keyboard class

    Has macro functionality and brightness based suspend
    """

    def __init__(self, *args, **kwargs):
        if 'additional_methods' in kwargs:
            kwargs['additional_methods'].extend(['get_keyboard_layout'])
        else:
            kwargs['additional_methods'] = ['get_keyboard_layout']
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        self.key_manager = _KeyboardKeyManager(self._device_number, self.event_files, self, use_epoll=True, testing=self._testing)

        self.logger.info('Putting device into driver mode. Daemon will handle special functionality')
        self.set_device_mode(0x03, 0x00)  # Driver mode

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        try:
            self.set_device_mode(0x00, 0x00)  # Device mode
        except FileNotFoundError:  # Could be called when daemon is stopping or device is removed.
            pass

        self.key_manager.close()

    def _resume_device(self):
        """
        Restore device mode

        Set device back to driver mode after e.g. suspend which resets the
        device to default device mode.

        NOTE: This is really the wrong place to put this, since this callback
        is for screensaver unlock, and not for 'wake up from suspend' or
        similar. Nevertheless for now this seems to be the best place for this
        and should resolve some issues with macro keys not working after
        suspend.
        """
        super()._resume_device()

        self.logger.info('Putting device back into driver mode.')
        self.set_device_mode(0x03, 0x00)  # Driver mode


class _RippleKeyboard(_MacroKeyboard):
    """
    Keyboard class

    Inherits _MacroKeyboard and has a ripple manager
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        if not self.HAS_MATRIX:
            # You can use _MacroKeyboard instead if the keyboard doesn't support matrix
            raise RuntimeError("Cannot use RippleKeyboard without matrix capabilities")

        self.ripple_manager = _RippleManager(self, self._device_number)

        # we need to set the effect to ripple (if needed) after the ripple manager has started
        # otherwise it doesn't work
        if self.zone["backlight"]["effect"] == "ripple" or self.zone["backlight"]["effect"] == "rippleRandomColour":
            effect_func_name = 'set' + self.capitalize_first_char(self.zone["backlight"]["effect"])
            effect_func = getattr(self, effect_func_name, None)

            if effect_func is not None:
                if effect_func_name == 'setRipple':
                    effect_func(self.zone["backlight"]["colors"][0], self.zone["backlight"]["colors"][1], self.zone["backlight"]["colors"][2], self.ripple_manager._ripple_thread._refresh_rate)
                elif effect_func_name == 'setRippleRandomColour':
                    effect_func(self.ripple_manager._ripple_thread._refresh_rate)

    def _close(self):
        super()._close()

        self.ripple_manager.close()


class RazerNostromo(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Nostromo
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Nostromo-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0111
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keypad', 'keypad_get_profile_led_red', 'keypad_set_profile_led_red', 'keypad_get_profile_led_green', 'keypad_set_profile_led_green', 'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue',
               'get_macros', 'delete_macro', 'add_macro',

               # ?
               'keypad_get_mode_modifier', 'keypad_set_mode_modifier']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/564/564_tartarus_classic.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        # self.key_manager = _GamepadKeyManager(self._device_number, self.event_files, self, testing=self._testing)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerTartarus(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Tartarus
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Tartarus(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0201
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keypad',
               'set_static_effect', 'bw_set_pulsate', 'keypad_get_profile_led_red', 'keypad_set_profile_led_red', 'keypad_get_profile_led_green',
               'keypad_set_profile_led_green', 'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue', 'get_macros', 'delete_macro', 'add_macro', 'keypad_get_mode_modifier', 'keypad_set_mode_modifier']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/228/228_tartarus.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        # self.key_manager = _GamepadKeyManager(self._device_number, self.event_files, self, testing=self._testing)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerTartarusChroma(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Tartarus Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Tartarus_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0208
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keypad', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_none_effect', 'set_static_effect', 'set_spectrum_effect', 'keypad_get_profile_led_red', 'keypad_set_profile_led_red', 'keypad_get_profile_led_green',
               'keypad_set_profile_led_green', 'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue', 'get_macros', 'delete_macro', 'add_macro', 'keypad_get_mode_modifier', 'keypad_set_mode_modifier']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/598/598_tartarus_chroma.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        # self.key_manager = _GamepadKeyManager(self._device_number, self.event_files, self, testing=self._testing)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerTartarusV2(_RippleKeyboard):
    """
    Class for Razer Tartarus V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Tartarus_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x022B
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [4, 6]

    METHODS = ['get_device_type_keypad',
               'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_key_row',
               'set_reactive_effect',
               'set_none_effect',
               'set_custom_effect',
               'set_wave_effect',
               'set_static_effect',
               'set_spectrum_effect',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'keypad_get_profile_led_red', 'keypad_set_profile_led_red',
               'keypad_get_profile_led_green', 'keypad_set_profile_led_green',
               'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue',
               'get_macro_mode', 'set_macro_mode', 'get_macro_effect', 'set_macro_effect',
               'get_macros', 'delete_macro', 'add_macro',
               'set_ripple_effect', 'set_ripple_effect_random_colour',
               'get_game_mode', 'set_game_mode',
               'keypad_get_mode_modifier', 'keypad_set_mode_modifier']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1255/1255_tartarus_v2.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerOrbweaver(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Orbweaver
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Orbweaver(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0113
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keypad',
               'keypad_get_profile_led_red', 'keypad_set_profile_led_red', 'keypad_get_profile_led_green', 'keypad_set_profile_led_green', 'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue',
               'get_macros', 'delete_macro', 'add_macro', 'keypad_get_mode_modifier', 'keypad_set_mode_modifier',
               'set_none_effect', 'bw_set_pulsate', 'bw_set_static']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/56/56_orbweaver.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        # self.key_manager = _OrbweaverKeyManager(self._device_number, self.event_files, self, testing=self._testing)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerOrbweaverChroma(_RippleKeyboard):
    """
    Class for the Razer Orbweaver Chroma
    """

    EVENT_FILE_REGEX = re.compile(r'.*Razer_Orbweaver_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0207
    DEDICATED_MACRO_KEYS = True
    HAS_MATRIX = True
    MATRIX_DIMS = [5, 22]
    METHODS = ['get_device_type_keypad',
               'set_key_row',
               'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_none_effect', 'set_static_effect', 'set_spectrum_effect', 'set_wave_effect', 'set_custom_effect',
               'keypad_get_profile_led_red', 'keypad_set_profile_led_red',
               'keypad_get_profile_led_green', 'keypad_set_profile_led_green',
               'keypad_get_profile_led_blue', 'keypad_set_profile_led_blue',
               'get_macros', 'delete_macro', 'add_macro',
               'keypad_get_mode_modifier', 'keypad_set_mode_modifier',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/607/607_orbweaver_chroma.png"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Methods are loaded into DBus by this point

        # self.key_manager = _OrbweaverKeyManager(self._device_number, self.event_files, self, testing=self._testing)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerBlackWidowUltimate2012(_MacroKeyboard):
    """
    Class for the Razer BlackWidow Ultimate 2012
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_BlackWidow_Ultimate_2012(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x010D
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/563/563_blackwidow_ultimate_classic.png"


class RazerBlackWidowStealth(_MacroKeyboard):
    """
    Class for the Razer BlackWidow (Classic)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_BlackWidow(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x011B
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/17559/razer-blackwidow-gallery-01.png"


class RazerBlackWidowStealthEdition(_MacroKeyboard):
    """
    Class for the Razer BlackWidow Stealth Edition
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_BlackWidow(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x010E
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/17559/razer-blackwidow-gallery-01.png"


class RazerBlackWidowUltimate2013(_MacroKeyboard):
    """
    Class for the Razer BlackWidow Ultimate 2013
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_BlackWidow_Ultimate(_2013)?(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x011A
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/245/438_blackwidow_ultimate_2014.png"


class RazerBlackWidowTournamentEdition2014(_MacroKeyboard):
    """
    Class for the Razer BlackWidow Tournament Edition 2014
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_Tournament_Edition(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x011C
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/17564/razer-blackwidow-te-stealth-hero-01.png"


class RazerBlackWidowV3ProWired(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V3 Pro Wired
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_V3_Pro(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x025A
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour',
               # Battery
               'set_charge_effect', 'set_charge_colour', 'get_battery', 'is_charging', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/3809-1-EN-v1.png"


class RazerBlackWidowV3ProWireless(RazerBlackWidowV3ProWired):
    """
    Class for the Razer BlackWidow v3 Pro 2.4 Ghz Wireless
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_V3_Pro(_\d+)?(-if01)?-event-kbd')

    USB_PID = 0x025C
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               # TODO:
               # Ripple works with this keyboard when plugged as wired but failed
               # to get it to work in wireless mode.
               # 'set_ripple_effect', 'set_ripple_effect_random_colour',
               'set_charge_effect', 'set_charge_colour', 'get_battery', 'is_charging', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/3809-1-EN-v1.png"


class RazerBlackWidowChroma(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0203
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',

               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/279/279_blackwidow_chroma.png"


class RazerBlackWidowChromaV2(_RippleKeyboard):
    """
    Class for the BlackWidow Chroma V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Chroma_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0221
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1179/1179_blackwidow_chroma_v2_alt.png"


class RazerBlackWidowChromaTournamentEdition(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Tournament Edition Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Tournament_Edition_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0209
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macros', 'delete_macro', 'add_macro',
               'get_macro_mode', 'set_macro_mode', 'get_macro_effect', 'set_macro_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/571/571_blackwidow_tournament_edition_chroma.png"


class RazerBlackWidowXChroma(_RippleKeyboard):
    """
    Class for the Razer BlackWidow X Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_X_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0216
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',

               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/716/716_blackwidow_x_chroma.png"


class RazerHuntsmanV2Tenkeyless(_RippleKeyboard):
    """
    Class for the Razer Huntsman V2 Tenkeyless
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_V2_Tenkeyless(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026B
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 18]
    POLL_RATES = [125, 250, 500, 1000, 2000, 4000, 8000]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates', 'get_keyswitch_optimization', 'set_keyswitch_optimization',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/5638/5638-1-en-v1.png"


class RazerHuntsmanV2(_RippleKeyboard):
    """
    Class for the Razer Huntsman V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026C
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    POLL_RATES = [125, 250, 500, 1000, 2000, 4000, 8000]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates', 'get_keyswitch_optimization', 'set_keyswitch_optimization',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/5642/5642-1-en-v1.png"


# TODO Should become _RippleKeyboard once kernel support for driver mode is implemented
class RazerHuntsmanV2Analog(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Huntsman V2 Analog
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_V2_Analog(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0266
    HAS_MATRIX = True
    MATRIX_DIMS = [8, 22]
    # TODO Remove get_keyboard_layout once not _RazerDeviceBrightnessSuspend anymore
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'get_macro_mode', 'set_macro_mode', 'get_macro_effect', 'set_macro_effect', 'get_game_mode', 'set_game_mode',
               'set_custom_effect', 'set_key_row', 'get_keyboard_layout']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/4023-1-EN-v1.png"


# TODO Should become _RippleKeyboard once kernel support for driver mode is implemented
class RazerHuntsmanMiniAnalog(_RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Huntsman Mini Analog
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_Mini_Analog(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0282
    HAS_MATRIX = True
    MATRIX_DIMS = [5, 15]
    # TODO Remove get_keyboard_layout once not _RazerDeviceBrightnessSuspend anymore
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'get_macro_mode', 'set_macro_mode', 'get_macro_effect', 'set_macro_effect', 'get_game_mode', 'set_game_mode',
               'set_custom_effect', 'set_key_row', 'get_keyboard_layout']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1689/1689-huntsmanmini.png"


class RazerBlackWidowXTournamentEditionChroma(_RippleKeyboard):
    """
    Class for the Razer BlackWidow X Tournament Edition Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_X_Tournament_Edition_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x021A
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',

               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/717/717_blackwidow_x_tournament_edition_chroma.png"


class RazerBladeStealth(_RippleKeyboard):
    """
    Class for the Razer Blade Stealth
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0205
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/667/667_blade_stealth_2016_6500u.png"


class RazerBladeStealthLate2016(_RippleKeyboard):
    """
    Class for the Razer Blade Stealth (Late 2016)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0220
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/667/667_blade_stealth_2016_6500u.png"


class RazerBladeProLate2016(_MacroKeyboard):
    """
    Class for the Razer Blade Pro (Late 2016)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Pro(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0210
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/736/736_blade_pro_2016.png"


class RazerBladeLate2016(_RippleKeyboard):
    """
    Class for the Razer Blade (Late 2016)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0224
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/736/736_blade_pro_2016.png"


class RazerBladeQHD(_RippleKeyboard):
    """
    Class for the Razer Blade (QHD)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x020F
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect',
               'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',

               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/736/736_blade_pro_2016.png"


class RazerBlackWidowUltimate2016(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Ultimate 2016
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Ultimate_2016(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0214
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro', 'set_starlight_random_effect',

               'set_ripple_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/616/616_blackwidow_ultimate_2016.png"


class RazerBlackWidowXUltimate(_RippleKeyboard):
    """
    Class for the Razer BlackWidow X Ultimate
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_X_Ultimate(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0217
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro', 'set_starlight_random_effect',

               'set_ripple_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/718/718_blackwidow_x_ultimate.png"


class RazerOrnataChroma(_RippleKeyboard):
    """
    Class for the Razer Ornata Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*Ornata_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x021E
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/727/727_ornata_chroma.png"


class RazerOrnataV2(_RippleKeyboard):
    """
    Class for the Razer Ornata V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Ornata_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x025D
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1672/ornata-v2.png"


class RazerOrnataV3(_RippleKeyboard):
    """
    Class for the Razer Ornata V3
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Ornata_V3(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x02A1
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 10]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6075-1-en-v1.png"


class RazerOrnataV3X(_MacroKeyboard):
    """
    Class for the Razer Ornata V3 X
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Ornata_V3_X(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0294
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6071-1-en-v1.png"


class RazerOrnataV3X_Alternate(RazerOrnataV3X):
    """
    Class for the Razer Ornata V3 X (Alternate)
    """
    USB_PID = 0x02A2


class RazerOrnataV3Tenkeyless(_RippleKeyboard):
    """
    Class for the Razer Ornata V3 Tenkeyless
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Ornata_V3_Tenkeyless(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x02A3
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 8]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro', 'set_ripple_effect',
               'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13038/13038-1-en-v1.png"


class RazerHuntsmanElite(_RippleKeyboard):
    """
    Class for the Razer Huntsman Elite
    """
    EVENT_FILE_REGEX = re.compile(r'.*Huntsman_Elite(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0226
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [9, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1361/1361_huntsman_elite.png"


class RazerHuntsmanTournamentEdition(_RippleKeyboard):
    """
    Class for the Razer Huntsman Tournament Edition
    """
    EVENT_FILE_REGEX = re.compile(r'.*Huntsman_Tournament_Edition_00000000001A(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0243
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 18]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1537/1537_huntsman_te.png"


class RazerBlackWidowElite(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Elite
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Elite(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0228
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1398/1398_blackwidowelite.png"


class RazerHuntsman(_RippleKeyboard):
    """
    Class for the Razer Huntsman
    """
    EVENT_FILE_REGEX = re.compile(r'.*Huntsman(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0227
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1360/1360_huntsman-3.png"


class RazerBlackWidowV3(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V3
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_V3(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x024E
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/3827-1-EN-v1.png"


class RazerBlackWidowV3TK(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V3 TK
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_V3_Tenkeyless(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0A24
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [6, 18]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1709/1709-blackwidow-v3-tkl.png"


class RazerBlackWidowV3MiniHyperspeed(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V3 Mini Hyperspeed
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_V3_Mini(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0258
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [5, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_battery', 'is_charging']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1777/500x500-blackwidowv3mini.png"


class RazerBlackWidowV3MiniHyperspeedWireless(RazerBlackWidowV3MiniHyperspeed):
    """
    Class for the Razer BlackWidow V3 Mini Hyperspeed (Wireless)
    """
    USB_PID = 0x0271


class RazerCynosaChroma(_RippleKeyboard):
    """
    Class for the Razer Cynosa Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*Cynosa_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x022A
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1256/1256_cynosa_chroma.png"


class RazerCynosaChromaPro(_RippleKeyboard):
    """
    Class for the Razer Cynosa Chroma Pro
    """
    EVENT_FILE_REGEX = re.compile(r'.*Cynosa_Chroma_Pro(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x022C
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1257/1257_cynosa_chroma_pro_alt.png"


class RazerCynosaV2(_RippleKeyboard):
    """
    Class for the Razer Cynosa V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Cynosa_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x025E
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1694/1694-2.png"


class RazerCynosaLite(_MacroKeyboard):
    """
    Class for the Razer Cynosa Lite
    """
    EVENT_FILE_REGEX = re.compile(r'.*Cynosa_Lite(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x023F
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/og-image/cynosa-lite-OGimage.jpg"


class RazerBlackWidowLite(_MacroKeyboard):
    """
    Class for the Razer BlackWidow Lite
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Lite(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0235
    METHODS = ['get_device_type_keyboard', 'set_static_effect',
               'set_none_effect', 'set_breath_single_effect',
               'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1456/1456_blackwidowlite_-_2.png"


class RazerBlackWidow2019(_RippleKeyboard):
    """
    Class for the Razer BlackWidow 2019
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0241
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1501/1501-blackwidow2019.png"


class RazerBlackWidowEssential(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Essential
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_BlackWidow_Essential(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0237
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_none_effect',
               'set_spectrum_effect', 'set_wave_effect', 'set_reactive_effect',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_breath_single_effect', 'set_key_row', 'set_custom_effect', 'get_game_mode',
               'set_game_mode', 'get_macro_mode', 'set_macro_mode', 'get_macro_effect',
               'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1501/1501-blackwidow2019.png"


class RazerOrnata(_RippleKeyboard):
    """
    Class for the Razer Ornata
    """
    EVENT_FILE_REGEX = re.compile(r'.*Ornata(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x021F
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_single_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode', 'set_breath_single_effect',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/726/726_ornata.png"


class RazerAnansi(_MacroKeyboard):
    """
    Class for the Razer Anansi
    """
    EVENT_FILE_REGEX = re.compile(r'.*Anansi(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x010F
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_keyboard',
               'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode', 'get_macro_effect',
               'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro', 'set_static_effect',
               'set_spectrum_effect', 'set_none_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/54/54_anansi.png"


class RazerDeathStalkerExpert(_MacroKeyboard):
    """
    Class for the Razer DeathStalker Expert
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathStalker(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0202
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/49/49_razer_deathstalker.png"


class RazerDeathStalkerEssential(_MacroKeyboard):
    """
    Class for the Razer DeathStalker/DeathStalker Essential
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathStalker(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0118
    METHODS = ['get_device_type_keyboard', 'get_game_mode', 'set_game_mode', 'set_macro_mode', 'get_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'bw_set_pulsate', 'bw_set_static', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/49/49_razer_deathstalker.png"


class RazerDeathStalkerChroma(_RippleKeyboard):
    """
    Class for the Razer DeathStalker Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*DeathStalker_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0204
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [1, 12]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/665/665_deathstalker_chroma.png"


class RazerDeathStalkerV2(_RippleKeyboard):
    """
    Class for the Razer DeathStalker V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*DeathStalker_V2(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0295
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6118/6118-1-en-v1.png"


class RazerDeathStalkerV2ProWired(_RippleKeyboard):
    """
    Class for the Razer DeathStalker V2 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*DSV2_Pro_000000000000-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0292
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour',
               # Battery
               'get_battery', 'is_charging']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6118/6118-1-en-v1.png"


class RazerDeathStalkerV2ProWireless(RazerDeathStalkerV2ProWired):
    """
    Class for the Razer DeathStalker V2 Pro (Wireless)
    """
    USB_PID = 0x0290


class RazerDeathStalkerV2ProTKLWired(_RippleKeyboard):
    """
    Class for the Razer DeathStalker V2 Pro TKL (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*DSV2_Pro_000000000000-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0298
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 17]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour',
               # Battery
               'get_battery', 'is_charging']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6117/6117-1-en-v1.png"


class RazerDeathStalkerV2ProTKLWireless(RazerDeathStalkerV2ProTKLWired):
    """
    Class for the Razer DeathStalker V2 Pro TKL (Wireless)
    """
    USB_PID = 0x0296


class RazerBlackWidowChromaOverwatch(_RippleKeyboard):
    """
    Class for the Razer BlackWidow Chroma (Overwatch)
    """
    EVENT_FILE_REGEX = re.compile(r'.*BlackWidow_Chroma(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0211
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',

               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/23326/overwatch-razer-gallery-5.png"


class RazerBladeStealthMid2017(_RippleKeyboard):
    """
    Class for the Razer Blade Stealth (Mid 2017)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x022D
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1213/1213_blade_stealth_2017_7500u.png"


class RazerBladePro2017(_RippleKeyboard):
    """
    Class for the Razer Blade Pro (2017)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Pro(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0225
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 25]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1200/1200_blade_pro_2017.png"


class RazerBladePro2017FullHD(_RippleKeyboard):
    """
    Class for the Razer Blade Pro FullHD (2017)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_ProFullHD(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x022F
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 25]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1200/1200_blade_pro_2017.png"


class RazerBladeStealthLate2017(_RippleKeyboard):
    """
    Class for the Razer Blade Stealth (Late 2017)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0232
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1213/1213_blade_stealth_2017_7500u.png"


class RazerBlade2018(_RippleKeyboard):
    """
    Class for the Razer Blade 15 (2018)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0233
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour', 'get_logo_active', 'set_logo_active']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1418/1418_blade_2018__base.png"


class RazerBlade2018Mercury(_RippleKeyboard):
    """
    Class for the Razer Blade 15 (2018) Mercury
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0240
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row', 'set_starlight_random_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1552/1552-blade-stealth-mercury-white.png"


class RazerBlade2018Base(_MacroKeyboard):
    """
    Class for the Razer Blade 15 (2018) Base Model
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x023B
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1418/1418_blade_2018__base.png"


class RazerBladeStealth2019(_MacroKeyboard):
    """
    Class for the Razer Blade Stealth (2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0239
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1475/1475_bladestealth13(2019).png"


class RazerBladeStealthLate2019(_MacroKeyboard):
    """
    Class for the Razer Blade Stealth (Late 2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x024A
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-stealth-13/shop/stealth-l2p-1.jpg"


class RazerBladeStealthEarly2020(_MacroKeyboard):
    """
    Class for the Razer Blade Stealth (Early 2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0252
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-stealth-13/shop/sl25p-fhd-4.jpg"


class RazerBladeStealthLate2020(_MacroKeyboard):
    """
    Class for the Razer Blade Stealth (Late 2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade_Stealth(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0259
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-stealth-13/shop/sl25p-fhd-4.jpg"


class RazerBook2020(_MacroKeyboard):
    """
    Class for the Razer Book (2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026A
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1743/razerbook132020.png"


class RazerBlade2019Adv(_RippleKeyboard):
    """
    Class for the Razer Blade 15 (2019) Advanced
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x023A
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1482/blade15.png"


class RazerBladeMid2019Mercury(_RippleKeyboard):
    """
    Class for the Razer Blade 15 (Mid 2019) Mercury
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0245
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-15/shop/blade15-mercury-1.jpg"


class RazerBlade2019Base(_MacroKeyboard):
    """
    Class for the Razer Blade 15 (Mid 2019) Base Model
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0246
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1518/1518_blade15_mid2019-base.png"


class RazerBladeEarly2020Base(_MacroKeyboard):
    """
    Class for the Razer Blade Base (Early 2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0255
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-15/blade-15-base-model-spec-image-v2.png"


class RazerBladeProLate2019(_RippleKeyboard):
    """
    Class for the Razer Blade Pro (Late 2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x024C
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/razer-blade-pro-17/razer-blade-pro-17-2019-OGimage-1200x630.jpg"


class RazerBladeAdvancedLate2019(_RippleKeyboard):
    """
    Class for the Razer Blade Advanced (Late 2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x024B
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/razer-blade-pro-17/razer-blade-pro-17-2019-OGimage-1200x630.jpg"


class RazerBladeProEarly2020(_RippleKeyboard):
    """
    Class for the Razer Blade Pro (Early 2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0256
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_custom_effect', 'set_key_row',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1654/blade-pro-17-2020-2.png"


class RazerBlade2019StudioEdition(_RippleKeyboard):
    """
    Class for the Razer Blade 15 Studio Edition (2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x024D
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/blade-15/shop/studio-ch41-1.jpg"


class RazerBladePro2019(_RippleKeyboard):
    """
    Class for the Razer Blade Pro 17 (2019)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0234
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active',
               'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/razer-blade-pro-17/razer-blade-pro-17-2019-OGimage-1200x630.jpg"


class RazerBlade15Advanced2020(_RippleKeyboard):
    """
    Class for the Razer Blade 15 Advanced (2020)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0253
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1651/razer-blade-15-advanced-2020.png"


class RazerBlade15Advanced2021(_RippleKeyboard):
    """
    Class for the Razer Blade 15 Advanced (Mid 2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0276
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1778/1778-razerblade15advanced2021rz09-0409x-2.png"


class RazerBlade17Pro2021(_RippleKeyboard):
    """
    Class for the Razer Blade 17 Pro (Mid 2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0279
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/5524/5524-1-en-v2.png"


class RazerBlade142021(_RippleKeyboard):
    """
    Class for the Razer Blade 14 (2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0270
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/og-image/razer-blade-14-og-image-1200x630.jpg"


class RazerHuntsmanMini(_RippleKeyboard):
    """
    Class for the Razer Huntsman Mini
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_Mini_00000000001A(-if01)?-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x0257
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [5, 15]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1689/1689-huntsmanmini.png"


class RazerHuntsmanMiniJP(_RippleKeyboard):
    """
    Class for the Razer Huntsman Mini JP
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Huntsman_Mini_00000000001A(-if01)?-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x0269
    HAS_MATRIX = True
    WAVE_DIRS = (0, 1)
    MATRIX_DIMS = [5, 15]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://rzrwarranty.s3.amazonaws.com/4403e5e6b96d59dc95a278e257688e64a68f7b24945617653d68e3273877c01e.png"


class RazerBlade15AdvancedEarly2021(_RippleKeyboard):
    """
    Class for the Razer Blade 15 Advanced (Early 2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026D
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1761/blade-15-advanced-2021-rz09-0367x.png"


class RazerBlade17ProEarly2021(_RippleKeyboard):
    """
    Class for the Razer Blade 17 Pro (Early 2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026E
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/4025-16-EN-v1.png"


class RazerBladeEarly2021Base(_MacroKeyboard):
    """
    Class for the Razer Blade Base (Early 2021)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x026F
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1756/blade-15-base-2021-rz09-0369x.png"


class RazerBladeEarly2022Base(_MacroKeyboard):
    """
    Class for the Razer Blade Base (Early 2022)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x027A
    METHODS = ['get_device_type_keyboard', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1756/blade-15-base-2021-rz09-0369x.png"


class RazerBlade172022(_RippleKeyboard):
    """
    Class for the Razer Blade 17 (2022)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x028B
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/5896/5896-1-en-v2.png"


class RazerBlade142022(_RippleKeyboard):
    """
    Class for the Razer Blade 14 (2022)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x028C
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/5896/5896-1-en-v2.png"


class RazerBlade15AdvancedEarly2022(_RippleKeyboard):
    """
    Class for the Razer Blade 15 Advanced (Early 2022)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x028A
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/pnx.assets/a1ee4c5a780a401444be898fe93ade69/thumbnail-blade15-new-model.png"


class RazerBlackWidowV4(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V4
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_V4(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0287
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [8, 23]
    POLL_RATES = [125, 250, 500, 1000, 2000, 4000, 8000]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_wheel_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13219/13219-1-en-v1.png"


class RazerBlackWidowV4X(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V4 X
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_V4_X(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0293
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [6, 22]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_wheel_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13223/13223-1-en-v1.png"


class RazerBlackWidowV4Pro(_RippleKeyboard):
    """
    Class for the Razer BlackWidow V4 Pro
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_BlackWidow_V4_Pro(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x028D
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [8, 23]
    POLL_RATES = [125, 250, 500, 1000, 2000, 4000, 8000]
    METHODS = ['get_device_type_keyboard', 'set_wave_effect', 'set_wheel_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'get_game_mode', 'set_game_mode', 'get_macro_mode', 'set_macro_mode',
               'get_macro_effect', 'set_macro_effect', 'get_macros', 'delete_macro', 'add_macro',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/9703/9703-1-en-v1.png"


class RazerBlade142023(_RippleKeyboard):
    """
    Class for the Razer Blade 14 (2023)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x029D
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13031/13031-1-en-v2.png"


class RazerBlade142024(_RippleKeyboard):
    """
    Class for the Razer Blade 14 (2024)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x02B6
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13031/13031-1-en-v2.png"


class RazerBlade152023(_RippleKeyboard):
    """
    Class for the Razer Blade 15 (2023)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x029E
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/9696/9696-1-en-v2.png"


class RazerBlade162023(_RippleKeyboard):
    """
    Class for the Razer Blade 16 (2023)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x029F
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/9668/9668-1-en-v2.png"


class RazerBlade182023(_RippleKeyboard):
    """
    Class for the Razer Blade 18 (2023)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Blade(-if01)?-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x02A0
    HAS_MATRIX = True
    MATRIX_DIMS = [6, 16]
    METHODS = ['get_device_type_keyboard', 'get_logo_active', 'set_logo_active', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect',
               'set_breath_dual_effect', 'set_custom_effect', 'set_key_row',
               'set_starlight_random_effect', 'set_starlight_single_effect', 'set_starlight_dual_effect',
               'set_ripple_effect', 'set_ripple_effect_random_colour']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/9676/9676-1-en-v1.png"
