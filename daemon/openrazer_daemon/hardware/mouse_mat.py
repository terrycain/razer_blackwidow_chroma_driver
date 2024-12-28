# SPDX-License-Identifier: GPL-2.0-or-later

"""
Mousemat class
"""
from openrazer_daemon.hardware.device_base import RazerDeviceBrightnessSuspend as __RazerDeviceBrightnessSuspend


class RazerFirefly(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Firefly (2013)
    """
    USB_VID = 0x1532
    USB_PID = 0x0C00
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 15]
    METHODS = ['get_device_type_mousemat', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_reactive_effect', 'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/594/594_firefly_500x500.png"


class RazerFireflyV2(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Firefly V2
    """
    USB_VID = 0x1532
    USB_PID = 0x0C04
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 19]
    METHODS = ['get_device_type_mousemat', 'set_wave_effect', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_reactive_effect', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1594/1594_firefly_v2.png"


class RazerFireflyHyperflux(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Firefly Hyperflux (2018)
    """
    USB_VID = 0x1532
    USB_PID = 0x0068
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 17]
    METHODS = ['get_device_type_mousemat', 'set_static_effect', 'set_spectrum_effect', 'set_key_row', 'set_custom_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_reactive_effect', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/594/594_firefly_500x500.png"


class RazerGoliathus(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Goliathus (2018)
    """
    USB_VID = 0x1532
    USB_PID = 0x0C01
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mousemat', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_reactive_effect', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1394/1394_goliathus_chroma.png"


class RazerGoliathusExtended(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Goliathus Extended (2018)
    """
    USB_VID = 0x1532
    USB_PID = 0x0C02
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mousemat', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_reactive_effect', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1395/1395_goliathusextended.png"


class RazerGoliathusChroma3XL(__RazerDeviceBrightnessSuspend):
    """
    Class for the Razer Goliathus Chroma 3XL
    """
    USB_VID = 0x1532
    USB_PID = 0x0C06
    HAS_MATRIX = True
    MATRIX_DIMS = [1, 1]
    METHODS = ['get_device_type_mousemat', 'set_static_effect', 'set_spectrum_effect',
               'set_none_effect', 'set_breath_random_effect', 'set_breath_single_effect', 'set_breath_dual_effect',
               'set_custom_effect', 'set_key_row', 'set_reactive_effect', 'trigger_reactive_effect']

    DEVICE_IMAGE = "https://assets.razerzone.com/eeimages/support/products/1395/1395_goliathusextended.png"
