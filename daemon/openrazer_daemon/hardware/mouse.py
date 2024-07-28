# SPDX-License-Identifier: GPL-2.0-or-later

"""
Mouse class
"""
import re
from openrazer_daemon.hardware.device_base import RazerDeviceBrightnessSuspend as __RazerDeviceBrightnessSuspend, RazerDevice as __RazerDevice


class RazerViperMini(__RazerDevice):
    """
    Class for the Razer Viper Mini
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Viper_Mini-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x008A
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness',
               # Underglow/Logo use LOGO_LED
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1634/vipermini.png"

    DPI_MAX = 8500


class RazerLanceheadWirelessWired(__RazerDevice):
    """
    Class for the Razer Lancehead Wireless (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Lancehead_Wireless-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0070
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 16]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               'get_left_brightness', 'set_left_brightness', 'get_right_brightness', 'set_right_brightness',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Left side
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Right side
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1205/1205_lancehead.png"

    DPI_MAX = 16000


class RazerLanceheadWirelessReceiver(RazerLanceheadWirelessWired):
    """
    Class for the Razer Lancehead Wireless (Receiver)
    """
    USB_PID = 0x006F
    METHODS = RazerLanceheadWirelessWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerLanceheadWired(__RazerDevice):
    """
    Class for the Razer Lancehead (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Lancehead-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0059
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 16]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               'get_left_brightness', 'set_left_brightness', 'get_right_brightness', 'set_right_brightness',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Left side
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Right side
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1205/1205_lancehead.png"

    DPI_MAX = 16000


class RazerLanceheadWireless(RazerLanceheadWired):
    """
    Class for the Razer Lancehead (Wireless)
    """
    USB_PID = 0x005A
    METHODS = RazerLanceheadWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerDeathAdderEssentialWhiteEdition(__RazerDevice):
    """
    Class for the Razer DeathAdder Essential (White Edition)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_DeathAdder_Essential_White_Edition-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0071
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness',
               'get_scroll_brightness', 'set_scroll_brightness',
               # Logo
               'set_logo_static', 'set_logo_none', 'set_logo_breath_single',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_none', 'set_scroll_breath_single']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/da10m/carousel/razer-death-adder-gallery-25.png"

    DPI_MAX = 6400


class RazerAbyssusEliteDVaEdition(__RazerDevice):
    """
    Class for the Razer Abyssus Elite (D.Va Edition)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_D.Va_Razer_Abyssus_Elite-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x006A
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness',
               # Underglow/Logo use LOGO_LED
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1288/d.va_abyssus_elite.png"

    DPI_MAX = 7200


class RazerAbyssusEssential(__RazerDevice):
    """
    Class for the Razer Abyssus Essential
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Abyssus_Essential-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x006B
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness',
               # Backlight/Logo...same
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1290/1290_abyssusessential.png"

    DPI_MAX = 7200


class RazerLanceheadTE(__RazerDevice):
    """
    Class for the Razer Lancehead Tournament Edition
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Lancehead_TE-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0060
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 16]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               'get_left_brightness', 'set_left_brightness', 'get_right_brightness', 'set_right_brightness',
               # Logo
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Left side
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Right side
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1203/1206_lanceheadte.png"

    DPI_MAX = 16000


class RazerMambaChromaWireless(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Mamba Chroma (Wireless)
    """
    USB_VID = 0x1532
    USB_PID = 0x0045
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 15]
    METHODS = ['get_device_type_mouse', 'get_battery', 'is_charging', 'set_backlight_wave',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random',
               'set_backlight_breath_single', 'set_backlight_breath_dual', 'set_custom_effect', 'set_key_row',
               'set_charge_effect', 'set_charge_colour',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/609/609_mamba_500x500.png"

    DPI_MAX = 16000


class RazerMambaChromaWired(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Mamba Chroma (Wired)
    """
    USB_VID = 0x1532
    USB_PID = 0x0044
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 15]
    METHODS = ['get_device_type_mouse', 'set_backlight_wave',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random',
               'set_backlight_breath_single', 'set_backlight_breath_dual', 'set_custom_effect', 'set_key_row', 'max_dpi',
               'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold', 'get_battery', 'is_charging']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/609/609_mamba_500x500.png"

    DPI_MAX = 16000


class RazerMambaTE(__RazerDevice):
    """
    Class for the Razer Mamba Tournament Edition
    """
    USB_VID = 0x1532
    USB_PID = 0x0046
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 16]
    METHODS = ['get_device_type_mouse', 'get_backlight_brightness', 'set_backlight_brightness', 'set_backlight_wave',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random',
               'set_backlight_breath_single', 'set_backlight_breath_dual', 'set_custom_effect', 'set_key_row', 'max_dpi',
               'get_dpi_xy', 'set_dpi_xy']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/606/606_mambate_500x500.png"

    DPI_MAX = 16000


class RazerAbyssus(__RazerDevice):
    """
    Class for the Razer Abyssus
    """
    USB_VID = 0x1532
    USB_PID = 0x0042
    METHODS = ['get_device_type_mouse', 'set_logo_none', 'set_logo_on', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/274/abyssus2014_500x500.png"


class RazerImperator(__RazerDevice):
    """
    Class for the Razer Imperator 2012
    """
    USB_VID = 0x1532
    USB_PID = 0x002F
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/215/215_imperator.png"

    DPI_MAX = 6400


class RazerOuroboros(__RazerDevice):
    """
    Class for the Razer Ouroboros
    """
    USB_VID = 0x1532
    USB_PID = 0x0032
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate', 'set_scroll_none', 'set_scroll_on', 'get_scroll_brightness', 'set_scroll_brightness',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/26/26_ouroboros.png"

    DPI_MAX = 8200


class RazerOrochi2013(__RazerDevice):
    """
    Class for the Razer Orochi 2013
    """
    USB_VID = 0x1532
    USB_PID = 0x0039
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate', 'set_scroll_none', 'set_scroll_on']

    DPI_MAX = 6400

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/612/612_orochi_2015.png"


class RazerOrochiWired(__RazerDevice):
    """
    Class for the Razer Orochi (Wired)
    """
    USB_VID = 0x1532
    USB_PID = 0x0048
    METHODS = ['get_device_type_mouse',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_on',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random',
               'set_backlight_breath_single', 'set_backlight_breath_dual',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/612/612_orochi_2015.png"

    DPI_MAX = 8200


class RazerDeathAdderChroma(__RazerDevice):
    """
    Class for the Razer DeathAdder Chroma
    """
    USB_VID = 0x1532
    USB_PID = 0x0043
    METHODS = ['get_device_type_mouse',
               'get_logo_brightness', 'set_logo_brightness', 'set_logo_none', 'set_logo_static', 'set_logo_breath_single', 'set_logo_blinking', 'set_logo_spectrum',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_breath_single', 'set_scroll_blinking', 'set_scroll_spectrum',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/278/278_deathadder_chroma.png"

    DPI_MAX = 10000


class RazerDeathAdder2000(__RazerDevice):
    """
    Class for the Razer DeathAdder 2000
    """
    USB_VID = 0x1532
    USB_PID = 0x004F
    METHODS = ['get_device_type_mouse',
               'get_logo_brightness', 'set_logo_brightness', 'set_logo_none', 'set_logo_on', 'set_logo_breath_mono',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_on', 'set_scroll_breath_mono',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/da10m/carousel/razer-death-adder-gallery-09.png"

    DPI_MAX = 2000


class RazerDeathAdder2013(__RazerDevice):
    """
    Class for the Razer DeathAdder 2013
    """
    USB_VID = 0x1532
    USB_PID = 0x0037
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_scroll_none', 'set_scroll_static', 'set_scroll_breath_single', 'set_scroll_blinking',
               'set_logo_none', 'set_logo_static', 'set_logo_breath_single', 'set_logo_blinking']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/561/561_deathadder_classic.png"

    DPI_MAX = 6400


class RazerNagaHexV2(__RazerDevice):
    """
    Class for the Razer Naga Hex V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_Hex_V2-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0050
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [1, 3]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Thumbgrid is technically backlight ID
               'get_backlight_brightness', 'set_backlight_brightness',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random', 'set_backlight_breath_single', 'set_backlight_breath_dual',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # #Macros
               'get_macros', 'delete_macro', 'add_macro',
               # Can set Logo, Scroll and thumbgrid with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/715/715_nagahexv2_500x500.png"

    DPI_MAX = 16000

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # self.key_manager = _NagaHexV2KeyManager(self._device_number, self.event_files, self, use_epoll=True, testing=self._testing, should_grab_event_files=True)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerNaga2012(__RazerDevice):
    """
    Class for the Razer Naga 2012
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Naga-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x002E
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on', 'set_backlight_none', 'set_backlight_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/39/razer-naga-gallery-4.png"

    DPI_MAX = 5600


class RazerNagaChroma(__RazerDevice):
    """
    Class for the Razer Naga Chroma
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_Chroma-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0053
    HAS_MATRIX = True
    DEDICATED_MACRO_KEYS = True
    MATRIX_DIMS = [1, 3]
    METHODS = ['get_device_type_mouse', 'get_dpi_xy', 'set_dpi_xy', 'max_dpi',
               'get_poll_rate', 'set_poll_rate',
               'get_backlight_brightness', 'set_backlight_brightness',
               'get_logo_brightness', 'set_logo_brightness',
               'get_scroll_brightness', 'set_scroll_brightness',
               # Thumbgrid is technically backlight ID
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random', 'set_backlight_breath_single', 'set_backlight_breath_dual',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # #Macros
               'get_macros', 'delete_macro', 'add_macro',
               # Can set Logo, Scroll and thumbgrid with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/636/636_naga_chroma.png"

    DPI_MAX = 16000

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # self.key_manager = _NagaHexV2KeyManager(self._device_number, self.event_files, self, use_epoll=True, testing=self._testing, should_grab_event_files=True)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerNagaTrinity(__RazerDevice):
    """
    Class for the Razer Naga Trinity
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_Trinity-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0067
    HAS_MATRIX = False  # TODO Device supports matrix, driver missing
    DEDICATED_MACRO_KEYS = True
    # MATRIX_DIMS = [1, 3]
    METHODS = ['get_device_type_mouse', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_brightness', 'set_brightness', 'set_static_effect', 'max_dpi']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1251/1251_razer_naga_trinity.png"

    DPI_MAX = 16000

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        # self.key_manager = _NagaHexV2KeyManager(self._device_number, self.event_files, self, use_epoll=True, testing=self._testing, should_grab_event_files=True)

    def _close(self):
        """
        Close the key manager
        """
        super()._close()

        # self.key_manager.close()


class RazerNagaHex(__RazerDevice):
    """
    Class for the Razer Naga Hex
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_Hex-if01-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0041
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/23/23_naga_hex.png"

    DPI_MAX = 5600


class RazerNagaHexRed(__RazerDevice):
    """
    Class for the Razer Naga Hex (Red)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_Hex-if01-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0036
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/12/razer-naga-hex-gallery-12.png"

    DPI_MAX = 5600


class RazerTaipan(__RazerDevice):
    """
    Class for the Razer Taipan
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Taipan-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0034
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/19/19_taipan.png"

    DPI_MAX = 8200


class RazerDeathAdderElite(__RazerDevice):
    """
    Class for the Razer DeathAdder Elite
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathAdder_Elite-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x005C
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 2]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/724/724_deathadderelite_500x500.png"

    DPI_MAX = 16000


class RazerDiamondbackChroma(__RazerDevice):
    """
    Class for the Razer Diamondback Chroma
    """
    USB_VID = 0x1532
    USB_PID = 0x004C
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 21]
    METHODS = ['get_device_type_mouse', 'get_backlight_brightness', 'set_backlight_brightness', 'set_backlight_wave',
               'set_backlight_static', 'set_backlight_spectrum', 'set_backlight_reactive', 'set_backlight_none', 'set_backlight_breath_random',
               'set_backlight_breath_single', 'set_backlight_breath_dual', 'set_custom_effect', 'set_key_row',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy']

    DPI_MAX = 16000

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/613/613_diamondback.png"


class RazerDeathAdder3_5G(__RazerDevice):
    """
    Class for the Razer DeathAdder 3.5G
    """
    USB_VID = 0x1532
    USB_PID = 0x0016
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse',
               'get_poll_rate', 'set_poll_rate', 'get_dpi_xy', 'set_dpi_xy', 'available_dpi', 'max_dpi',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    AVAILABLE_DPI = [450, 900, 1800, 3500]
    DPI_MAX = 3500

    DEVICE_IMAGE = "https://assets2.razerzone.com/images/da10m/carousel/razer-death-adder-gallery-04.png"


class RazerDeathAdder3_5GBlack(__RazerDevice):
    """
    Class for the Razer DeathAdder 3.5G Black
    """
    USB_VID = 0x1532
    USB_PID = 0x0029
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse',
               'get_poll_rate', 'set_poll_rate', 'get_dpi_xy', 'set_dpi_xy', 'available_dpi', 'max_dpi']

    AVAILABLE_DPI = [450, 900, 1800, 3500]
    DPI_MAX = 3500

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/33/razer-deathadder-be-gallery-3.png"


class RazerMamba2012Wireless(__RazerDevice):
    """
    Class for the Razer Mamba 2012 (Wireless)
    """
    USB_VID = 0x1532
    USB_PID = 0x0025
    METHODS = ['get_device_type_mouse', 'get_battery', 'is_charging',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_spectrum']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/192/192_mamba_2012.png"

    DPI_MAX = 6400


class RazerMamba2012Wired(__RazerDevice):
    """
    Class for the Razer Mamba 2012 (Wired)
    """
    USB_VID = 0x1532
    USB_PID = 0x0024
    METHODS = ['get_device_type_mouse',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_spectrum',
               'get_battery', 'is_charging']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/192/192_mamba_2012.png"

    DPI_MAX = 6400


class RazerMambaWirelessWired(__RazerDevice):
    """
    Class for the Razer Mamba Wireless (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Mamba_Wireless_000000000000-if0(1|2)-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x0073
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 16]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1404/1404_mamba_wireless.png"

    DPI_MAX = 16000


class RazerMambaWirelessReceiver(RazerMambaWirelessWired):
    """
    Class for the Razer Mamba Wireless (Receiver)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Mamba_Wireless_Receiver-if0(1|2)-event-kbd')
    USB_PID = 0x0072
    METHODS = RazerMambaWirelessWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerNaga2014(__RazerDevice):
    """
    Class for the Razer Naga 2014
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Naga_2014-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0040
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on', 'set_backlight_none', 'set_backlight_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/227/227_razer_naga_2014.png"

    DPI_MAX = 8200


class RazerOrochi2011(__RazerDevice):
    """
    Class for the Razer Orochi 2011
    """
    USB_VID = 0x1532
    USB_PID = 0x0013
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Orochi-if01-event-kbd')

    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on', 'set_scroll_none', 'set_scroll_on']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/612/612_orochi_2015.png"

    DPI_MAX = 4000


class RazerAbyssusV2(__RazerDevice):
    """
    Class for the Razer Abyssus V2
    """
    USB_VID = 0x1532
    USB_PID = 0x005B
    METHODS = ['get_device_type_mouse',
               'get_logo_brightness', 'set_logo_brightness', 'set_logo_none', 'set_logo_static', 'set_logo_breath_single', 'set_logo_blinking', 'set_logo_spectrum',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_breath_single', 'set_scroll_blinking', 'set_scroll_spectrum',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/721/721_abyssusv2.png"

    DPI_MAX = 5000


class RazerAbyssus1800(__RazerDevice):
    """
    Class for the Razer Abyssus 1800
    """
    USB_VID = 0x1532
    USB_PID = 0x0020
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on']

    DPI_MAX = 1800

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1277/1277_abyssus_2000.png"


class RazerAbyssus2000(__RazerDevice):
    """
    Class for the Razer Abyssus 2000
    """
    USB_VID = 0x1532
    USB_PID = 0x005E
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on']

    DPI_MAX = 2000

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1277/1277_abyssus_2000.png"


class RazerDeathAdder3500(__RazerDevice):
    """
    Class for the Razer DeathAdder 3500
    """
    USB_VID = 0x1532
    USB_PID = 0x0054
    METHODS = ['get_device_type_mouse',
               'get_logo_brightness', 'set_logo_brightness', 'set_logo_none', 'set_logo_static', 'set_logo_breath_single', 'set_logo_blinking',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_breath_single', 'set_scroll_blinking',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/561/561_deathadder_classic.png"

    DPI_MAX = 3500


class RazerViperUltimateWired(__RazerDevice):
    """
    Class for the Razer Viper Ultimate (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Viper_Ultimate_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x007A
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1577/ee_photo.png"

    DPI_MAX = 20000


class RazerViperUltimateWireless(RazerViperUltimateWired):
    """
    Class for the Razer Viper Ultimate (Wireless)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Viper_Ultimate_Dongle-if0(1|2)-event-kbd')

    USB_PID = 0x007B
    METHODS = RazerViperUltimateWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerViper(__RazerDevice):
    """
    Class for the Razer Viper
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Viper-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0078
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1539/1539_viper.png"

    DPI_MAX = 16000


class RazerDeathAdderEssential(__RazerDevice):
    """
    Class for the Razer DeathAdder Essential
    """
    USB_VID = 0x1532
    USB_PID = 0x006E
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_static', 'set_logo_none', 'set_logo_breath_single',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_static', 'set_scroll_none', 'set_scroll_breath_single']

    DPI_MAX = 6400

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1385/1385_deathadderessential.png"


class RazerDeathAdderEssential2021(__RazerDevice):
    """
    Class for the Razer DeathAdder Essential (2021)
    """
    USB_VID = 0x1532
    USB_PID = 0x0098
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_static', 'set_logo_none', 'set_logo_breath_single']

    DPI_MAX = 6400

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1385/1385_deathadderessential.png"


class RazerMambaElite(__RazerDevice):
    """
    Class for the Razer Mamba Elite
    """
    USB_VID = 0x1532
    USB_PID = 0x006C
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 20]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate',
               # Logo logo_led_brightness/logo_matrix_effect_breath/...
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel scroll_led_brightness/scroll_matrix_effect_breath/...
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Left side left_led_brightness/left_matrix_effect_breath/...
               'get_left_brightness', 'set_left_brightness',
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Right side right_led_brightness/right_matrix_effect_breath/...
               'get_right_brightness', 'set_right_brightness',
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DPI_MAX = 16000

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1390/1390_mamba_elite.png"


class RazerNagaLeftHanded2020(__RazerDevice):
    """
    Class for the Razer Naga Left Handed Edition 2020
    """
    USB_VID = 0x1532
    USB_PID = 0x008D
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 3]

    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_poll_rate', 'set_poll_rate',
               # Macros
               'get_macros', 'delete_macro', 'add_macro',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Right side = thumbgrid
               'get_right_brightness', 'set_right_brightness',
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DPI_MAX = 20000

    DEVICE_IMAGE = "https://rzrwarranty.s3.amazonaws.com/cee694cd7526df413008167b7566af310985321b20c57f3dc42e5cbd773f2417.png"


class RazerNagaProWired(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Naga Pro (Wired)
    """
    USB_VID = 0x1532
    USB_PID = 0x008F
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 3]

    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate', 'get_dpi_stages', 'set_dpi_stages',
               # Macros
               'get_macros', 'delete_macro', 'add_macro',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Thumbgrid
               'set_static_effect', 'set_spectrum_effect', 'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DPI_MAX = 20000

    DEVICE_IMAGE = "https://hybrismediaprod.blob.core.windows.net/sys-master-phoenix-images-container/hfd/ha6/9080569528350/razer-naga-pro-500x500.png"


class RazerNagaProWireless(RazerNagaProWired):
    """
    Class for the Razer Naga Pro (Wireless)
    """
    USB_PID = 0x0090
    METHODS = RazerNagaProWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerDeathAdder1800(__RazerDevice):
    """
    Class for the Razer DeathAdder 1800
    """
    USB_VID = 0x1532
    USB_PID = 0x0038
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'set_logo_none', 'set_logo_on']

    DPI_MAX = 1800

    DEVICE_IMAGE = "https://rzrwarranty.s3.amazonaws.com/a7daf40ad78c9584a693e310effa956019cdcd081391f93f71a7cd36d3dc577e.png"


class RazerBasilisk(__RazerDevice):
    """
    Class for the Razer Basilisk
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0064
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 2]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1241/1241_basilisk.png"

    DPI_MAX = 16000


class RazerBasiliskEssential(__RazerDevice):
    """
    Class for the Razer Basilisk Essential
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_Essential-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0065
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate',
               # Logo
               'get_logo_brightness', 'set_logo_brightness', 'set_logo_spectrum', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual', 'set_logo_static', 'set_logo_none',
               # Can set LOGO with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://hybrismediaprod.blob.core.windows.net/sys-master-phoenix-images-container/h19/h61/9080617500702/Basilisk-Essential-500x500.png"

    DPI_MAX = 6400


class RazerBasiliskUltimateWired(__RazerDevice):
    """
    Class for the Razer Basilisk Ultimate
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_Ultimate_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0086
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 14]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               # Spectrum
               'set_logo_spectrum',
               # Reactive
               'set_logo_reactive',
               # Breath
               'set_logo_breath_random',
               'set_logo_breath_single',
               'set_logo_breath_dual',
               # Static
               'set_logo_static',
               # None
               'set_logo_none',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               # Spectrum
               'set_scroll_spectrum',
               # Reactive
               'set_scroll_reactive',
               # Breath
               'set_scroll_breath_random',
               'set_scroll_breath_single',
               'set_scroll_breath_dual',
               # Static
               'set_scroll_static',
               # None
               'set_scroll_none',
               'get_left_brightness', 'set_left_brightness', 'get_right_brightness', 'set_right_brightness',
               # Left side
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Right side
               'set_right_wave', 'set_right_static', 'set_right_spectrum', 'set_right_none', 'set_right_reactive', 'set_right_breath_random', 'set_right_breath_single', 'set_right_breath_dual',

               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1590/1590_basilisk_ultimate.png"

    DPI_MAX = 20000


class RazerBasiliskUltimateReceiver(RazerBasiliskUltimateWired):
    USB_PID = 0x0088
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_Ultimate_Dongle-if0(1|2)-event-kbd')
    METHODS = RazerBasiliskUltimateWired.METHODS + \
        ['set_charge_effect', 'set_charge_colour']


class RazerBasiliskV2(__RazerDevice):
    """
    Class for the Razer Basilisk V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_V2-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0085
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 2]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1617/1617_basilisk-v2.png"

    DPI_MAX = 20000


class RazerBasiliskV3(__RazerDevice):
    """
    Class for the Razer Basilisk V3
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_V3-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0099
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 11]
    METHODS = ['get_device_type_mouse',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_brightness', 'set_brightness',
               'get_logo_brightness', 'set_logo_brightness',
               'get_scroll_brightness', 'set_scroll_brightness',
               # Scroll wheel controls
               'get_scroll_mode', 'set_scroll_mode',
               'get_scroll_acceleration', 'set_scroll_acceleration',
               'get_scroll_smart_reel', 'set_scroll_smart_reel',
               # All LEDs (partial support)
               'set_static_effect', 'set_wave_effect', 'set_spectrum_effect',
               # Logo (partial support)
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum',
               # Scroll wheel (partial support)
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum',
               # Can set custom matrix effects
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets2.razerzone.com/pages/basilisk-v3-4D578898E8144Le8da21dde32b7a9f5f/basilisk.png"

    DPI_MAX = 26000


class RazerDeathAdderV2(__RazerDevice):
    """
    Class for the Razer DeathAdder V2
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathAdder_V2-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness', 'get_scroll_brightness', 'set_scroll_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive', 'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Scroll wheel
               'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Can set LOGO and Scroll with custom
               'set_custom_effect', 'set_key_row']

    USB_VID = 0x1532
    USB_PID = 0x0084
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1612/1612_razerdeathadderv2.png"

    DPI_MAX = 20000


class RazerDeathAdderV2ProWired(__RazerDevice):
    """
    Class for the Razer DeathAdder V2 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*1532_Razer_DeathAdder_V2_Pro_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x007C
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate', 'set_poll_rate', 'get_logo_brightness', 'set_logo_brightness',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1714/comp_1_00000.png"

    DPI_MAX = 20000


class RazerDeathAdderV2ProWireless(RazerDeathAdderV2ProWired):
    """
    Class for the Razer DeathAdder V2 Pro (Wireless)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_DeathAdder_V2_Pro_000000000000-if0(1|2)-event-kbd')

    USB_PID = 0x007D
    METHODS = RazerDeathAdderV2ProWired.METHODS + ['set_charge_effect', 'set_charge_colour']


class RazerAtherisReceiver(__RazerDevice):
    """
    Class for the Razer Atheris (Receiver)
    """
    USB_VID = 0x1532
    USB_PID = 0x0062
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Atheris_-_Mobile_Gaming_Mouse-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'get_poll_rate', 'set_poll_rate',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time',
               'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1234/1234_atheris.png"

    DPI_MAX = 7200


class RazerBasiliskXHyperSpeed(__RazerDevice):
    """
    Class for the Razer Basilisk X HyperSpeed
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_X_HyperSpeed-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0083
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate',
               'set_poll_rate', 'get_battery', 'is_charging', 'get_idle_time',
               'set_idle_time', 'get_low_battery_threshold',
               'set_low_battery_threshold']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1589/1589_basilisk_x__hyperspeed.png"

    DPI_MAX = 16000


class RazerOrochiV2Receiver(__RazerDevice):
    """
    Class for the Razer Orochi V2 (Receiver)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Orochi_V2_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x0094
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages', 'get_poll_rate',
               'set_poll_rate', 'get_battery', 'is_charging', 'get_idle_time',
               'set_idle_time', 'get_low_battery_threshold',
               'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/OrochiV2-1-en-v1.png"

    DPI_MAX = 18000


class RazerOrochiV2Bluetooth(RazerOrochiV2Receiver):
    """
    Class for the Razer Orochi V2 (Bluetooth)
    """
    USB_PID = 0x0095


class RazerNagaX(__RazerDevice):
    """
    Class for the Razer Naga X
    """
    USB_VID = 0x1532
    USB_PID = 0x0096
    HAS_MATRIX = True
    WAVE_DIRS = (1, 2)
    MATRIX_DIMS = [1, 2]

    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               # Macros
               'get_macros', 'delete_macro', 'add_macro',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual',
               # Left side = thumbgrid
               'get_left_brightness', 'set_left_brightness',
               'set_left_wave', 'set_left_static', 'set_left_spectrum', 'set_left_none', 'set_left_reactive', 'set_left_breath_random', 'set_left_breath_single', 'set_left_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    DPI_MAX = 18000

    DEVICE_IMAGE = "https://dl.razerzone.com/src/3993-1-EN-V2.png"


class RazerDeathAdderV2Mini(__RazerDevice):
    """
    Class for the Razer DeathAdder V2 Mini
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathAdder_V2_Mini-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    USB_VID = 0x1532
    USB_PID = 0x008C
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1692/deathadder-v2-mini.png"

    DPI_MAX = 8500


class RazerViper8KHz(__RazerDevice):
    """
    Class for the Razer Viper 8KHz
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Viper_8KHz-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_logo_brightness', 'set_logo_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual']
    USB_VID = 0x1532
    USB_PID = 0x0091
    DEVICE_IMAGE = "https://dl.razerzone.com/Images/Viper%208KHz/Viper8khz.png"

    DPI_MAX = 20000

    POLL_RATES = [125, 500, 1000, 2000, 4000, 8000]


class RazerViperMiniSEWired(__RazerDevice):
    """
    Class for the Razer Viper Mini SE (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Viper_Mini_Signature_Edition_000000000000-if0(1|2)-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x009E
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/9682/9682-1-en-v1.png"

    DPI_MAX = 30000

    POLL_RATES = [125, 500, 1000]


class RazerViperMiniSEWireless(RazerViperMiniSEWired):
    """
    Class for the Razer Viper Mini SE (Wireless)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Viper_Mini_Signature_Edition-if0(1|2)-event-kbd')
    USB_PID = 0x009F

    METHODS = RazerViperMiniSEWired.METHODS + ['set_hyperpolling_wireless_dongle_indicator_led_mode']

    POLL_RATES = [125, 500, 1000, 2000, 4000, 8000]


class RazerNagaEpicChromaWired(__RazerDevice):
    """
    Class for the Razer Naga Epic Chroma (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Naga_Epic_Chroma-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x003E
    METHODS = ['get_firmware', 'get_matrix_dims', 'has_matrix', 'get_device_name', 'get_device_type_mouse', 'get_battery', 'is_charging',
               'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_poll_rate', 'set_poll_rate',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_breath_single', 'set_scroll_spectrum',
               'get_backlight_brightness', 'set_backlight_brightness', 'set_backlight_none', 'set_backlight_static', 'set_backlight_breath_single', 'set_backlight_spectrum']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/products/20776/rzrnagaepicchroma_04.png"

    DPI_MAX = 8200


class RazerNagaEpicChromaWireless(RazerNagaEpicChromaWired):
    """
    Class for the Razer Naga Epic Chroma (Wireless)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Naga_Epic_Chroma_Dock-if0(1|2)-event-kbd')

    USB_PID = 0x003F


class RazerProClickReceiver(__RazerDevice):
    """
    Class for the Razer Pro Click (Receiver)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Pro_Click-if0(1|2)-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x0077
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_idle_time', 'set_idle_time',

               'get_battery', 'is_charging',
               'get_low_battery_threshold', 'set_low_battery_threshold']
    DEVICE_IMAGE = "https://dl.razerzone.com/src/3802-0-en-v1.png"

    DPI_MAX = 16000

    POLL_RATES = [125, 500, 1000]


class RazerProClickWired(RazerProClickReceiver):
    """
    Class for the Razer Pro Click (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*1532_Razer_Pro_Click_000000000000-if0(1|2)-event-kbd')
    USB_PID = 0x0080


class RazerDeathAdderV2XHyperSpeed(__RazerDevice):
    """
    Class for the Razer DeathAdder V2 X HyperSpeed
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathAdder_V2_X_HyperSpeed_000000000000-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    USB_VID = 0x1532
    USB_PID = 0x009C
    DEVICE_IMAGE = "https://hybrismediaprod.blob.core.windows.net/sys-master-phoenix-images-container/he8/h51/9250345058334/deathadder-v2-x-hyperspeed-500x500.png"

    DPI_MAX = 14000


class RazerNagaEpic(__RazerDevice):
    """
    Class for the Razer Naga Epic
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Razer_Naga_Epic-if01-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x001F
    DEDICATED_MACRO_KEYS = True
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy_byte', 'set_dpi_xy_byte', 'get_poll_rate', 'set_poll_rate',
               'get_scroll_brightness', 'set_scroll_brightness', 'set_scroll_none', 'set_scroll_static', 'set_scroll_spectrum',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://hwimg.nl/Razer_naga-epic.png"

    DPI_MAX = 5600


class RazerViperV2ProWired(__RazerDevice):
    """
    Class for the Razer Viper V2 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Viper_V2_Pro_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00A5
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6048-1-en-v10.png"

    DPI_MAX = 30000


class RazerViperV2ProWireless(RazerViperV2ProWired):
    """
    Class for the Razer Viper V2 Pro (Wireless)
    """

    USB_PID = 0x00A6


class RazerCobraProWired(__RazerDevice):
    """
    Class for the Razer Cobra Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Cobra_Pro-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00AF

    METHODS = ['get_device_type_mouse',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_brightness', 'set_brightness',
               'get_logo_brightness', 'set_logo_brightness',
               'get_scroll_brightness', 'set_scroll_brightness',
               # All LEDs (partial support)
               'set_static_effect', 'set_wave_effect', 'set_spectrum_effect', 'set_none_effect',
               # Logo (partial support)
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none',
               # Scroll wheel (partial support)
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13182/13182-1-en-v2.png"

    DPI_MAX = 30000


class RazerCobraProWireless(RazerCobraProWired):
    """
    Class for the Razer Cobra Pro (Wireless)
    """
    USB_PID = 0x00B0


class RazerDeathAdderV3(__RazerDevice):
    """
    Class for the Razer DeathAdder V3
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_DeathAdder_V3-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B2
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates']

    POLL_RATES = [125, 500, 1000, 2000, 4000, 8000]

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6124/6124-1-en-v2.png"

    DPI_MAX = 30000


class RazerDeathAdderV3ProWired(__RazerDevice):
    """
    Class for the Razer DeathAdder V3 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_DeathAdder_V3_Pro_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B6
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6130/6130-1-en-v2.png"

    DPI_MAX = 30000


class RazerDeathAdderV3ProWireless(RazerDeathAdderV3ProWired):
    """
    Class for the Razer DeathAdder V3 Pro (Wireless)
    """

    USB_PID = 0x00B7


class RazerDeathAdderV3ProWired_Alternate(RazerDeathAdderV3ProWired):
    """
    Class for the Razer DeathAdder V3 Pro (Wired)
    """
    USB_PID = 0x00C2


class RazerDeathAdderV3ProWireless_Alternate(RazerDeathAdderV3ProWireless):
    """
    Class for the Razer DeathAdder V3 Pro (Wireless)
    """
    USB_PID = 0x00C3


class RazerBasiliskV3ProWired(__RazerDevice):
    """
    Class for the Razer Basilisk V3 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_Basilisk_V3_Pro_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00AA
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 13]
    METHODS = ['get_device_type_mouse',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_brightness', 'set_brightness',
               'get_logo_brightness', 'set_logo_brightness',
               'get_scroll_brightness', 'set_scroll_brightness',
               # Scroll wheel controls
               'get_scroll_mode', 'set_scroll_mode',
               'get_scroll_acceleration', 'set_scroll_acceleration',
               'get_scroll_smart_reel', 'set_scroll_smart_reel',
               # All LEDs (partial support)
               'set_static_effect', 'set_wave_effect', 'set_spectrum_effect', 'set_none_effect',
               # Logo (partial support)
               'set_logo_wave', 'set_logo_static', 'set_logo_spectrum', 'set_logo_none',
               # Scroll wheel (partial support)
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none',
               # Can set custom matrix effects
               'set_custom_effect', 'set_key_row',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/6220/6220-4-en-v1.png"

    DPI_MAX = 30000


class RazerBasiliskV3ProWireless(RazerBasiliskV3ProWired):
    """
    Class for the Razer Basilisk V3 Pro (Wireless)
    """

    USB_PID = 0x00AB


class RazerHyperPollingWirelessDongle(__RazerDevice):
    """
    Class for the Razer HyperPolling Wireless Dongle
    """

    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_HyperPolling_Wireless_Dongle-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B3
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               'set_hyperpolling_wireless_dongle_indicator_led_mode', 'set_hyperpolling_wireless_dongle_pair', 'set_hyperpolling_wireless_dongle_unpair']

    POLL_RATES = [125, 500, 1000, 2000, 4000, 8000]

    DEVICE_IMAGE = "https://dl.razerzone.com/src/6141/6141-1-en-v1.png"  # HyperPolling Wireless Dongle

    # TODO: After pairing, should find a way to use the image of the mouse instead of the dongle
    # DEVICE_IMAGE_DAV3PRO = "https://dl.razerzone.com/src/6130/6130-1-en-v2.png"  # DeathAdder V3 Pro
    # DEVICE_IMAGE_VIPERV2PRO = "https://dl.razerzone.com/src/6048-1-en-v10.png"  # Viper V2 Pro

    DPI_MAX = 30000


class RazerProClickMiniReceiver(__RazerDevice):
    """
    Class for the Razer Pro Click Mini (Receiver)
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_ProClickM-if0(1|2)-event-kbd')
    USB_VID = 0x1532
    USB_PID = 0x009A
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_idle_time', 'set_idle_time',
               'get_battery', 'is_charging',
               'get_low_battery_threshold', 'set_low_battery_threshold']
    DEVICE_IMAGE = "https://dl.razerzone.com/src/5763/5763-1-en-v1.png"

    DPI_MAX = 12000

    POLL_RATES = [125, 500, 1000]


class RazerDeathAdderV2Lite(__RazerDevice):
    """
    Class for the Razer DeathAdder V2 Lite
    """
    EVENT_FILE_REGEX = re.compile(r'.*Razer_DeathAdder_V2_Lite-if0(1|2)-event-kbd')
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_logo_brightness', 'set_logo_brightness',
               # Logo
               'set_logo_static', 'set_logo_spectrum', 'set_logo_none', 'set_logo_reactive',
               'set_logo_breath_random', 'set_logo_breath_single', 'set_logo_breath_dual',
               # Custom frame
               'set_custom_effect', 'set_key_row']

    USB_VID = 0x1532
    USB_PID = 0x00A1
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1692/deathadder-v2-mini.png"

    DPI_MAX = 8500


class RazerCobra(__RazerDevice):
    """
    Class for the Razer Cobra
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Cobra-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00A3
    METHODS = ['get_device_type_mouse',
               'max_dpi', 'get_dpi_xy', 'set_dpi_xy',
               'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               # Logo
               'get_logo_brightness', 'set_logo_brightness',
               'set_logo_breath_random', 'set_logo_breath_dual', 'set_logo_breath_single',
               'set_logo_reactive', 'set_logo_spectrum', 'set_logo_static', 'set_logo_none']

    DEVICE_IMAGE = "https://hybrismediaprod.blob.core.windows.net/sys-master-phoenix-images-container/h54/h60/9591466950686/cobra-500x500.png"

    POLL_RATES = [125, 500, 1000]
    DPI_MAX = 8500


class RazerNagaV2HyperSpeedReceiver(__RazerDevice):
    """
    Class for the Razer Naga V2 HyperSpeed (Receiver)
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Naga_V2_HyperSpeed_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B4
    HAS_MATRIX = False
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://hybrismediaprod.blob.core.windows.net/sys-master-phoenix-images-container%2Fh4c%2Fh44%2F9451887460382%2Fnaga-v2-hyperspeed-500x500.png"

    DPI_MAX = 30000


class RazerViperV3HyperSpeed(__RazerDevice):
    """
    Class for the Razer Viper V3 HyperSpeed
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Viper_V3_HyperSpeed_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B8
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/13432/13432-1-en-v3.png"

    DPI_MAX = 30000


class RazerBasiliskV3XHyperSpeed(__RazerDevice):
    """
    Class for the Razer Basilisk V3 X HyperSpeed
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Basilisk_V3_X_HyperSpeed_000000000000-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00B9
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate',
               # Battery
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold',
               # Scroll wheel
               'get_scroll_brightness', 'set_scroll_brightness',
               'set_scroll_wave', 'set_scroll_static', 'set_scroll_spectrum', 'set_scroll_none', 'set_scroll_reactive', 'set_scroll_breath_random', 'set_scroll_breath_single', 'set_scroll_breath_dual']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/9766/9766-1-en-v1.png"

    DPI_MAX = 18000


class RazerViperV3ProWired(__RazerDevice):
    """
    Class for the Razer Viper V3 Pro (Wired)
    """
    EVENT_FILE_REGEX = re.compile(r'.*usb-Razer_Razer_Viper_V3_Pro-if0(1|2)-event-kbd')

    USB_VID = 0x1532
    USB_PID = 0x00C0
    METHODS = ['get_device_type_mouse', 'max_dpi', 'get_dpi_xy', 'set_dpi_xy', 'get_dpi_stages', 'set_dpi_stages',
               'get_poll_rate', 'set_poll_rate', 'get_supported_poll_rates',
               'get_battery', 'is_charging', 'get_idle_time', 'set_idle_time', 'get_low_battery_threshold', 'set_low_battery_threshold']

    DEVICE_IMAGE = "https://dl.razerzone.com/src2/14044/14044-1-en-v1.png"

    POLL_RATES = [125, 500, 1000]
    DPI_MAX = 35000


class RazerViperV3ProWireless(RazerViperV3ProWired):
    """
    Class for the Razer Viper V3 Pro (Wireless)
    """

    USB_PID = 0x00C1

    METHODS = RazerViperV3ProWired.METHODS + ['set_hyperpolling_wireless_dongle_indicator_led_mode']

    POLL_RATES = [125, 500, 1000, 2000, 4000, 8000]
