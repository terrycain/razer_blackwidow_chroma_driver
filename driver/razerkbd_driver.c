// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Terri Cain <terri@dolphincorp.co.uk>
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/dmi.h>
#include <linux/input-event-codes.h>

#include "usb_hid_keys.h"

#include "razerkbd_driver.h"
#include "razercommon.h"
#include "razerchromacommon.h"

/*
 * Version Information
 */
#define DRIVER_DESC "Razer Keyboard Device Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

// KEY_MACRO* has been added in Linux 5.5, so define ourselves for older kernels.
// See also https://git.kernel.org/torvalds/c/b5625db
#ifndef KEY_MACRO1
#define KEY_MACRO1  0x290
#define KEY_MACRO2  0x291
#define KEY_MACRO3  0x292
#define KEY_MACRO4  0x293
#define KEY_MACRO5  0x294
#define KEY_MACRO6  0x295
#define KEY_MACRO7  0x296
#define KEY_MACRO8  0x297
#define KEY_MACRO9  0x298
#define KEY_MACRO10 0x299
#define KEY_MACRO11 0x2a0
#define KEY_MACRO12 0x2a1
// ...
#define KEY_MACRO27 0x2aa
#define KEY_MACRO28 0x2ab
#define KEY_MACRO29 0x2ac
#define KEY_MACRO30 0x2ad
#endif

// These are evdev key codes, not HID key codes.
// Lower macro key codes are intended for the actual macro keys
// Higher macro key codes are inteded for Chroma functions
#define RAZER_MACRO_KEY KEY_MACRO30 // TODO maybe KEY_MACRO_RECORD_START?
#define RAZER_GAME_KEY KEY_MACRO29 // TODO maybe KEY_GAMES?
#define RAZER_BRIGHTNESS_DOWN KEY_MACRO28
#define RAZER_BRIGHTNESS_UP KEY_MACRO27

/**
 * List of keys to swap
 */
static const struct razer_key_translation chroma_keys[] = {
    { KEY_F1,    KEY_MUTE },
    { KEY_F2,    KEY_VOLUMEDOWN },
    { KEY_F3,    KEY_VOLUMEUP },

    { KEY_F5,    KEY_PREVIOUSSONG },
    { KEY_F6,    KEY_PLAYPAUSE },
    { KEY_F7,    KEY_NEXTSONG },

    { KEY_F9,   RAZER_MACRO_KEY },
    { KEY_F10,  RAZER_GAME_KEY },
    { KEY_F11,  RAZER_BRIGHTNESS_DOWN },
    { KEY_F12,  RAZER_BRIGHTNESS_UP },

    { KEY_PAUSE, KEY_SLEEP },

    // Custom bind
    { KEY_KPENTER, KEY_CALC },
    { 0 }
};

static const struct razer_key_translation chroma_keys_2[] = {
    { KEY_F1,    KEY_MUTE },
    { KEY_F2,    KEY_VOLUMEDOWN },
    { KEY_F3,    KEY_VOLUMEUP },

    { KEY_F5,    KEY_PLAYPAUSE },
    { KEY_F6,    KEY_STOPCD },
    { KEY_F7,    KEY_PREVIOUSSONG },
    { KEY_F8,    KEY_NEXTSONG },
    { KEY_F11,   RAZER_GAME_KEY },
//     { KEY_F12,   RAZER_EFFECT_KEY }, // enable if daemon supports, see #577
    { KEY_RIGHTALT,    RAZER_MACRO_KEY },

    { KEY_PAUSE, KEY_SLEEP },
    { 0 }
};

// Huntsman Mini Fn keys
static const struct razer_key_translation chroma_keys_3[] = {
    { KEY_ESC, KEY_GRAVE },
    { KEY_1, KEY_F1 },
    { KEY_2, KEY_F2 },
    { KEY_3, KEY_F3 },
    { KEY_4, KEY_F4 },
    { KEY_5, KEY_F5 },
    { KEY_6, KEY_F6 },
    { KEY_7, KEY_F7 },
    { KEY_8, KEY_F8 },
    { KEY_9, KEY_F9 },
    { KEY_0, KEY_F10 },
    { KEY_MINUS, KEY_F11 },
    { KEY_EQUAL, KEY_F12 },
    { KEY_BACKSPACE, KEY_DELETE },
    { KEY_TAB, KEY_MUTE },
    { KEY_Q, KEY_VOLUMEDOWN },
    { KEY_W, KEY_VOLUMEUP },
    { KEY_E, KEY_PREVIOUSSONG },
    { KEY_R, KEY_PLAYPAUSE },
    { KEY_T, KEY_NEXTSONG },
    { KEY_Y, RAZER_MACRO_KEY },
    { KEY_U, RAZER_GAME_KEY },
    { KEY_I, KEY_UP },
    { KEY_O, KEY_SCROLLLOCK },
    { KEY_P, KEY_SYSRQ },
    { KEY_LEFTBRACE, KEY_PAGEUP },
    { KEY_RIGHTBRACE, KEY_HOME },
    { KEY_G, RAZER_BRIGHTNESS_DOWN },
    { KEY_H, RAZER_BRIGHTNESS_UP },
    { KEY_J, KEY_LEFT },
    { KEY_K, KEY_DOWN },
    { KEY_L, KEY_RIGHT },
    { KEY_SEMICOLON, KEY_PAGEDOWN },
    { KEY_APOSTROPHE, KEY_END },
    { KEY_Z, KEY_SLEEP },
    { KEY_DOT, KEY_PAUSE },
    { KEY_SLASH, KEY_INSERT },
    { 0 }
};

// Blackwidow V3 Mini Fn keys
static const struct razer_key_translation chroma_keys_4[] = {
    { KEY_ESC, KEY_GRAVE },
    { KEY_1, KEY_F1 },
    { KEY_2, KEY_F2 },
    { KEY_3, KEY_F3 },
    { KEY_4, KEY_F4 },
    { KEY_5, KEY_F5 },
    { KEY_6, KEY_F6 },
    { KEY_7, KEY_F7 },
    { KEY_8, KEY_F8 },
    { KEY_9, KEY_F9 },
    { KEY_0, KEY_F10 },
    { KEY_MINUS, KEY_F11 },
    { KEY_EQUAL, KEY_F12 },
    { KEY_DELETE, KEY_MACRO1 },
    { KEY_Y, RAZER_MACRO_KEY },
    { KEY_U, RAZER_GAME_KEY },
    { KEY_I, KEY_UP },
    { KEY_O, KEY_SYSRQ },
    { KEY_P, KEY_SCROLLLOCK },
    { KEY_LEFTBRACE, KEY_PAUSE },
    { KEY_RIGHTBRACE, KEY_SLEEP },
    { KEY_PAGEUP, KEY_MACRO2 },
    { KEY_G, RAZER_BRIGHTNESS_DOWN },
    { KEY_H, RAZER_BRIGHTNESS_UP },
    { KEY_APOSTROPHE, KEY_HOME },
    { KEY_PAGEDOWN, KEY_MACRO3 },
    { KEY_V, KEY_MUTE },
    { KEY_B, KEY_VOLUMEDOWN },
    { KEY_N, KEY_VOLUMEUP },
    { KEY_M, KEY_PREVIOUSSONG },
    { KEY_COMMA, KEY_PLAYPAUSE },
    { KEY_DOT, KEY_NEXTSONG },
    { KEY_SLASH, KEY_END },
    { KEY_INSERT, KEY_MACRO4 },
    { KEY_RIGHTALT, KEY_BLUETOOTH },
    { 0 }
};

// Razer BlackWidow V3 (Full size)
static const struct razer_key_translation chroma_keys_5[] = {
    { KEY_F9, RAZER_MACRO_KEY },
    { KEY_F10, RAZER_GAME_KEY },
    { KEY_F11, RAZER_BRIGHTNESS_DOWN },
    { KEY_F12, RAZER_BRIGHTNESS_UP },
    { KEY_PAUSE, KEY_SLEEP },
    // TODO - Add KEY_CONTEXT_MENU when we figure out what it is supposed to be doing
    { 0 }
};

/**
 * Essentially search through the struct array above.
 */
static const struct razer_key_translation *find_translation(const struct razer_key_translation *key_table, u16 from)
{
    const struct razer_key_translation *result;

    for (result = key_table; result->from; result++) {
        if (result->from == from) {
            return result;
        }
    }

    return NULL;
}

static bool is_blade_laptop(struct razer_kbd_device *device)
{
    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_2018:
    case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
    case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
    case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
    case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
    case USB_DEVICE_ID_RAZER_BLADE_QHD:
    case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
    case USB_DEVICE_ID_RAZER_BLADE_2019_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2019:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
    case USB_DEVICE_ID_RAZER_BOOK_2020:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
    case USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
    case USB_DEVICE_ID_RAZER_BLADE_14_2021:
    case USB_DEVICE_ID_RAZER_BLADE_17_2022:
    case USB_DEVICE_ID_RAZER_BLADE_14_2022:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
    case USB_DEVICE_ID_RAZER_BLADE_14_2023:
    case USB_DEVICE_ID_RAZER_BLADE_14_2024:
    case USB_DEVICE_ID_RAZER_BLADE_15_2023:
    case USB_DEVICE_ID_RAZER_BLADE_16_2023:
    case USB_DEVICE_ID_RAZER_BLADE_18_2023:
        return true;
    }
    return false;
}

/**
 * Get request/response indices and timing parameters for the device
 */
static void razer_get_report_params(struct usb_device *usb_dev, uint *report_index, uint *response_index, ulong *wait_min, ulong *wait_max)
{
    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        *report_index = 0x03;
        *response_index = 0x03;
        *wait_min = RAZER_BLACKWIDOW_CHROMA_WAIT_MIN_US;
        *wait_max = RAZER_BLACKWIDOW_CHROMA_WAIT_MAX_US;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
        *report_index = 0x03;
        *response_index = 0x03;
        *wait_min = RAZER_BLACKWIDOW_V3_WIRELESS_WAIT_MIN_US;
        *wait_max = RAZER_BLACKWIDOW_V3_WIRELESS_WAIT_MAX_US;
        break;
    case USB_DEVICE_ID_RAZER_ANANSI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
        *report_index = 0x02;
        *response_index = 0x02;
        *wait_min = RAZER_BLACKWIDOW_CHROMA_WAIT_MIN_US;
        *wait_max = RAZER_BLACKWIDOW_CHROMA_WAIT_MAX_US;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
        *report_index = 0x02;
        *response_index = 0x02;
        *wait_min = RAZER_BLACKWIDOW_V3_WIRELESS_WAIT_MIN_US;
        *wait_max = RAZER_BLACKWIDOW_V3_WIRELESS_WAIT_MAX_US;
        break;
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        *report_index = 0x02;
        *response_index = 0x02;
        *wait_min = RAZER_DEATHSTALKER_V2_WIRELESS_WAIT_MIN_US;
        *wait_max = RAZER_DEATHSTALKER_V2_WIRELESS_WAIT_MAX_US;
        break;
    default:
        *report_index = 0x01;
        *response_index = 0x01;
        *wait_min = RAZER_BLACKWIDOW_CHROMA_WAIT_MIN_US;
        *wait_max = RAZER_BLACKWIDOW_CHROMA_WAIT_MAX_US;
        break;
    }
}

/**
 * Send report to the keyboard
 */
static int razer_get_report(struct usb_device *usb_dev, struct razer_report *request, struct razer_report *response)
{
    uint report_index, response_index;
    ulong wait_min, wait_max;
    razer_get_report_params(usb_dev, &report_index, &response_index, &wait_min, &wait_max);
    return razer_get_usb_response(usb_dev, report_index, request, response_index, response, wait_min, wait_max);
}

/**
 * Send report to the keyboard, but without even reading the response
 */
static int razer_send_payload_no_response(struct razer_kbd_device *device, struct razer_report *request)
{
    uint report_index, response_index;
    ulong wait_min, wait_max;

    /* Except the caller to have set the transaction_id */
    WARN_ON(request->transaction_id.id == 0x00);

    razer_get_report_params(device->usb_dev, &report_index, &response_index, &wait_min, &wait_max);
    return razer_send_control_msg(device->usb_dev, request, report_index, wait_min, wait_max);
}

/**
 * Function to send to device, get response, and actually check the response
 */
static int razer_send_payload(struct razer_kbd_device *device, struct razer_report *request, struct razer_report *response)
{
    int err;

    request->crc = razer_calculate_crc(request);

    mutex_lock(&device->lock);
    err = razer_get_report(device->usb_dev, request, response);
    mutex_unlock(&device->lock);
    if (err) {
        print_erroneous_report(response, "razerkbd", "Invalid Report Length");
        return err;
    }

    /* Check the packet number, class and command are the same */
    if (response->remaining_packets != request->remaining_packets ||
        response->command_class != request->command_class ||
        response->command_id.id != request->command_id.id) {
        print_erroneous_report(response, "razerkbd", "Response doesn't match request");
        return -EIO;
    }

    switch (response->status) {
    case RAZER_CMD_BUSY:
        // TODO: Check if this should be an error.
        // print_erroneous_report(&response, "razermouse", "Device is busy");
        break;
    case RAZER_CMD_FAILURE:
        print_erroneous_report(response, "razerkbd", "Command failed");
        return -EIO;
    case RAZER_CMD_NOT_SUPPORTED:
        print_erroneous_report(response, "razerkbd", "Command not supported");
        return -EIO;
    case RAZER_CMD_TIMEOUT:
        print_erroneous_report(response, "razerkbd", "Command timed out");
        return -EIO;
    }

    return 0;
}

/**
 * Reads the physical layout of the keyboard.
 *
 * Returns a string
 */
static ssize_t razer_attr_read_kbd_layout(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = get_razer_report(0x00, 0x86, 0x02);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%02x\n", response.arguments[0]);
}

/**
 * Device mode function
 */
static void razer_set_device_mode(struct razer_kbd_device *device, unsigned char mode, unsigned char param)
{
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (is_blade_laptop(device)) {
        return;
    }

    request = razer_chroma_standard_set_device_mode(mode, param);

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
        request.transaction_id.id = 0x3F;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
        request.transaction_id.id = 0x1F;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request.transaction_id.id = 0x9F;
        break;
    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);
}

/**
 * Read device file "charge_level"
 *
 * Returns an integer which needs to be scaled from 0-255 -> 0-100
 */
static ssize_t razer_attr_read_charge_level(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_battery_level();

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request.transaction_id.id = 0x1f;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request.transaction_id.id = 0x9f;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        request.transaction_id.id = 0x3f;
        break;
    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Read device file "charge_status"
 *
 * Returns 0 when not charging, 1 when charging
 */
static ssize_t razer_attr_read_charge_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_charging_status();

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request.transaction_id.id = 0x1f;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request.transaction_id.id = 0x9f;
        break;
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        request.transaction_id.id = 0x3f;
        break;
    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Write device file "charge_effect"
 *
 * Sets charging effect.
 */
static ssize_t razer_attr_write_charge_effect(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 1) {
        printk(KERN_WARNING "razerkbd: Incorrect number of bytes for setting the charging effect\n");
        return -EINVAL;
    }

    request = razer_chroma_misc_set_dock_charge_type(buf[0]);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charge_colour"
 *
 * Sets charging colour using 3 RGB bytes
 */
static ssize_t razer_attr_write_charge_colour(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    // First enable static charging effect
    request = razer_chroma_misc_set_dock_charge_type(0x01);
    request.transaction_id.id = 0xFF;
    razer_send_payload(device, &request, &response);

    if (count != 3) {
        printk(KERN_WARNING "razerkbd: Charging colour mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    request = razer_chroma_standard_set_led_rgb(NOSTORE, BATTERY_LED, (struct razer_rgb*)&buf[0]);
    request.transaction_id.id = 0xFF;
    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "charge_low_threshold"
 */
static ssize_t razer_attr_read_charge_low_threshold(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_low_battery_threshold();
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[0]);
}

/**
 * Write device file "charge_low_threshold"
 *
 * Sets the low battery blink threshold to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_charge_low_threshold(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char threshold = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_low_battery_threshold(threshold);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);
    return count;
}

/**
 * Write device file "game_led_state"
 *
 * When 1 is written (as a character, 0x31) Game mode will be enabled, if 0 is written (0x30)
 * then game mode will be disabled
 *
 * The reason the keyboard appears as 2 keyboard devices is that one of those devices is used by
 * game mode as that keyboard device is missing a super key. A hacky and over-the-top way to disable
 * the super key if you ask me.
 */
static ssize_t razer_attr_write_game_led_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        request = razer_chroma_standard_set_led_state(NOSTORE, GAME_LED, enabled);
        request.transaction_id.id = 0x1f;
        break;
    default:
        request = razer_chroma_standard_set_led_state(VARSTORE, GAME_LED, enabled);
        request.transaction_id.id = 0xFF;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "game_led_state"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_game_led_state(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        request = razer_chroma_standard_get_led_state(NOSTORE, GAME_LED);
        request.transaction_id.id = 0x1f;
        break;
    default:
        request = razer_chroma_standard_get_led_state(VARSTORE, GAME_LED);
        request.transaction_id.id = 0xFF;
    }

    razer_send_payload(device, &request, &response);
    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Write device file "keyswitch_optimization"
 */
static ssize_t razer_attr_write_keyswitch_optimization(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char mode = (unsigned char)simple_strtoul(buf, NULL, 10);

    // Toggle Keyswitch Optimization
    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        request = razer_chroma_misc_set_keyswitch_optimization_command1(mode);
        request.transaction_id.id = 0x1f;
        razer_send_payload(device, &request, &response);
        request = razer_chroma_misc_set_keyswitch_optimization_command2(mode);
        request.transaction_id.id = 0x1f;
        razer_send_payload(device, &request, &response);
        break;
    default:
        return -ENOSYS;
    }

    return count;
}

/**
 * Read device file "keyswitch_optimization"
 */
static ssize_t razer_attr_read_keyswitch_optimization(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    int state;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        request = razer_chroma_misc_get_keyswitch_optimization();
        request.transaction_id.id = 0x1f;
        break;
    default:
        request = razer_chroma_misc_get_keyswitch_optimization();
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    if(response.arguments[1] == 0x14) { // Either 0x00 or 0x14
        state = 0; // Typing
    } else {
        state = 1; // Gaming
    }

    return sprintf(buf, "%d\n", state);
}

/**
 * Write device file "macro_led_state"
 *
 * When 1 is written (as a character, 0x31) Macro mode will be enabled, if 0 is written (0x30)
 * then game mode will be disabled
 */
static ssize_t razer_attr_write_macro_led_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_set_led_state(VARSTORE, MACRO_LED, enabled);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "macro_led_state"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_macro_led_state(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_led_state(VARSTORE, MACRO_LED);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);
    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Read device file "version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", DRIVER_VERSION);
}

/**
 * Read device file "device_type"
 *
 * Returns friendly string of device type
 */
static ssize_t razer_attr_read_device_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    char *device_type;

    switch (device->usb_pid) {

    case USB_DEVICE_ID_RAZER_NOSTROMO:
        device_type = "Razer Nostromo\n";
        break;

    case USB_DEVICE_ID_RAZER_ORBWEAVER:
        device_type = "Razer Orbweaver\n";
        break;

    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
        device_type = "Razer Orbweaver Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
        device_type = "Razer BlackWidow Stealth\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
        device_type = "Razer BlackWidow Stealth Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
        device_type = "Razer BlackWidow Ultimate 2012\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
        device_type = "Razer BlackWidow Ultimate 2013\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016:
        device_type = "Razer BlackWidow Ultimate 2016\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE:
        device_type = "Razer BlackWidow X Ultimate\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        device_type = "Razer BlackWidow Tournament Edition 2014\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
        device_type = "Razer Blade Stealth\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019:
        device_type = "Razer Blade Stealth (Late 2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020:
        device_type = "Razer Blade Stealth (Early 2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020:
        device_type = "Razer Blade Stealth (Late 2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BOOK_2020:
        device_type = "Razer Book 13 (2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
        device_type = "Razer Blade Stealth (Late 2016)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
        device_type = "Razer Blade Stealth (Mid 2017)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_QHD:
        device_type = "Razer Blade Stealth (QHD)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
        device_type = "Razer Blade Pro (Late 2016)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_2018:
        device_type = "Razer Blade 15 (2018)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
        device_type = "Razer Blade 15 (2018) Mercury\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
        device_type = "Razer Blade 15 (2018) Base Model\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
        device_type = "Razer Blade 15 (2019) Advanced\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_2019_BASE:
        device_type = "Razer Blade 15 (2019) Base Model\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE:
        device_type = "Razer Blade 15 Base (Early 2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
        device_type = "Razer Blade 15 (Mid 2019) Mercury White\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
        device_type = "Razer Blade 15 Studio Edition (2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
        device_type = "Razer Blade (Late 2016)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
        device_type = "Razer Blade Pro (2017)\n";
        break;
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
        device_type = "Razer Blade Pro FullHD (2017)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_PRO_2019:
        device_type = "Razer Blade Pro (2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
        device_type = "Razer Blade Pro (Late 2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
        device_type = "Razer Blade Advanced (Late 2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
        device_type = "Razer Blade Pro (Early 2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
        device_type = "Razer Blade Stealth (Late 2017)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
        device_type = "Razer Blade Stealth (2019)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
        device_type = "Razer Blade 15 Advanced (2020)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
        device_type = "Razer Blade 15 Advanced (Early 2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
        device_type = "Razer Blade 15 Advanced (Mid 2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021:
        device_type = "Razer Blade 15 Base (Early 2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
        device_type = "Razer Blade 15 Base (2022)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
        device_type = "Razer Blade 17 Pro (Early 2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
        device_type = "Razer Blade 17 Pro (Mid 2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_14_2021:
        device_type = "Razer Blade 14 (2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS:
        device_type = "Razer Tartarus\n";
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        device_type = "Razer Tartarus Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        device_type = "Razer Tartarus V2\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_OVERWATCH:
        device_type = "Razer BlackWidow Chroma (Overwatch)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA:
        device_type = "Razer BlackWidow Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_ESSENTIAL:
        device_type = "Razer Deathstalker (Essential)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_EXPERT:
        device_type = "Razer Deathstalker Expert\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA:
        device_type = "Razer DeathStalker Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE:
        device_type = "Razer BlackWidow Chroma Tournament Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA:
        device_type = "Razer BlackWidow X Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA_TE:
        device_type = "Razer BlackWidow X Chroma Tournament Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
        device_type = "Razer BlackWidow Lite\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
        device_type = "Razer BlackWidow 2019\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
        device_type = "Razer BlackWidow Essential\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA:
        device_type = "Razer Ornata\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
        device_type = "Razer Ornata Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_V2:
        device_type = "Razer Ornata V2\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_V3:
        device_type = "Razer Ornata V3\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
        device_type = "Razer Ornata V3 X\n";
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
        device_type = "Razer Ornata V3 Tenkeyless\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
        device_type = "Razer Huntsman Elite\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
        device_type = "Razer Huntsman Tournament Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
        device_type = "Razer BlackWidow Elite\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN:
        device_type = "Razer Huntsman\n";
        break;

    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
        device_type = "Razer Cynosa Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
        device_type = "Razer Cynosa Chroma Pro\n";
        break;

    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
        device_type = "Razer Cynosa Lite\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        device_type = "Razer BlackWidow Chroma V2\n";
        break;

    case USB_DEVICE_ID_RAZER_ANANSI:
        device_type = "Razer Anansi\n";
        break;

    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
        device_type = "Razer Cynosa V2\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
        device_type = "Razer Huntsman Mini\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
        device_type = "Razer Huntsman Mini (JP)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
        device_type = "Razer BlackWidow V3\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        device_type = "Razer BlackWidow V3 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
        device_type = "Razer BlackWidow V3 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
        device_type = "Razer BlackWidow V3 Tenkeyless\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
        device_type = "Razer Huntsman V2 Tenkeyless\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        device_type = "Razer Huntsman V2\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
        device_type = "Razer Huntsman V2 Analog\n";
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
        device_type = "Razer Huntsman Mini Analog\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
        device_type = "Razer BlackWidow V3 Mini Hyperspeed (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
        device_type = "Razer BlackWidow V3 Mini Hyperspeed (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_17_2022:
        device_type = "Razer Blade 17 (2022)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_14_2022:
        device_type = "Razer Blade 14 (2022)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
        device_type = "Razer Blade 15 Advanced (Early 2022)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_14_2023:
        device_type = "Razer Blade 14 (2023)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_14_2024:
        device_type = "Razer Blade 14 (2024)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_15_2023:
        device_type = "Razer Blade 15 (2023)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
        device_type = "Razer DeathStalker V2\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
        device_type = "Razer DeathStalker V2 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
        device_type = "Razer DeathStalker V2 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
        device_type = "Razer BlackWidow V4\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
        device_type = "Razer BlackWidow V4 X\n";
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        device_type = "Razer BlackWidow V4 Pro\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        device_type = "Razer DeathStalker V2 Pro TKL (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        device_type = "Razer DeathStalker V2 Pro TKL (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_16_2023:
        device_type = "Razer Blade 16 (2023)\n";
        break;

    case USB_DEVICE_ID_RAZER_BLADE_18_2023:
        device_type = "Razer Blade 18 (2023)\n";
        break;

    default:
        device_type = "Unknown Device\n";
    }

    return sprintf(buf, device_type);
}

/**
 * Write device file "macro_led_effect"
 *
 * When 1 is written the LED will blink, 0 will static
 */
static ssize_t razer_attr_write_macro_led_effect(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
        request = razer_chroma_standard_set_led_effect(NOSTORE, MACRO_LED, enabled);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
        request = razer_chroma_standard_set_led_effect(NOSTORE, MACRO_LED, enabled);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_ANANSI:
        request = razer_chroma_standard_set_led_effect(NOSTORE, MACRO_LED, enabled);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_blinking(NOSTORE, MACRO_LED);
        request.transaction_id.id = 0xFF;
        break;

    default:
        request = razer_chroma_standard_set_led_effect(VARSTORE, MACRO_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    }
    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "macro_led_effect"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_macro_led_effect(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_led_effect(VARSTORE, MACRO_LED);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Write device file "matrix_effect_pulsate"
 *
 * The brightness oscillates between fully on and fully off generating a pulsing effect
 */
static ssize_t razer_attr_write_matrix_effect_pulsate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        request = razer_chroma_standard_set_led_effect(VARSTORE, LOGO_LED, CLASSIC_EFFECT_BREATHING);
        request.transaction_id.id = 0xFF;
        break;

    default:
        request = razer_chroma_standard_set_led_effect(VARSTORE, BACKLIGHT_LED, CLASSIC_EFFECT_BREATHING);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "matrix_effect_pulsate"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_matrix_effect_pulsate(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_led_effect(VARSTORE, LOGO_LED);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Read device file "profile_led_red"
 *
 * Actually a Yellow LED
 *
 * Returns a string
 */
static ssize_t razer_attr_read_profile_led_red(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_get_led_state(VARSTORE, BLUE_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_get_led_state(VARSTORE, RED_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Read device file "profile_led_green"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_profile_led_green(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_get_led_state(VARSTORE, RED_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_get_led_state(VARSTORE, GREEN_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Read device file "profile_led_blue"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_profile_led_blue(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_get_led_state(VARSTORE, GREEN_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_get_led_state(VARSTORE, BLUE_PROFILE_LED);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Write device file "profile_led_red"
 */
static ssize_t razer_attr_write_profile_led_red(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_set_led_state(VARSTORE, BLUE_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_set_led_state(VARSTORE, RED_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "profile_led_green"
 */
static ssize_t razer_attr_write_profile_led_green(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_set_led_state(VARSTORE, RED_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_set_led_state(VARSTORE, GREEN_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);
    return count;
}

/**
 * Write device file "profile_led_blue"
 */
static ssize_t razer_attr_write_profile_led_blue(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
        request = razer_chroma_standard_set_led_state(VARSTORE, GREEN_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    default:
        request = razer_chroma_standard_set_led_state(VARSTORE, BLUE_PROFILE_LED, enabled);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);
    return count;
}

/**
 * Read device file "device_serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    char serial_string[51];
    struct razer_report request = {0};
    struct razer_report response = {0};

    /* For Blade laptops we get the serial number from DMI */
    if (is_blade_laptop(device)) {
        strncpy(&serial_string[0], dmi_get_system_info(DMI_PRODUCT_SERIAL), 50);
        goto exit;
    }

    request = razer_chroma_standard_get_serial();
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    strncpy(&serial_string[0], &response.arguments[0], 22);
    serial_string[22] = '\0';

exit:
    return sprintf(buf, "%s\n", &serial_string[0]);
}

/**
 * Read device file "firmware_version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_firmware_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_firmware_version();
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "v%d.%d\n", response.arguments[0], response.arguments[1]);
}

/**
 * Write device file "matrix_effect_none"
 *
 * No keyboard effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_ANANSI:
        request = razer_chroma_standard_set_led_state(VARSTORE, BACKLIGHT_LED, OFF);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        request = razer_chroma_standard_matrix_effect_none();
        request.transaction_id.id = 0x3F;
        break;

    default:
        request = razer_chroma_standard_matrix_effect_none();
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_wave"
 *
 * When 1 is written (as a character, 0x31) the wave effect is displayed moving left across the keyboard
 * if 2 is written (0x32) then the wave effect goes right
 *
 * For the extended its 0x00 and 0x01
 */
static ssize_t razer_attr_write_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, BACKLIGHT_LED, direction);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        request = razer_chroma_standard_matrix_effect_wave(direction);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_V3:
        // Direction values are flipped compared to other devices
        direction ^= ((1<<0) | (1<<1));
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, BACKLIGHT_LED, direction);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, BACKLIGHT_LED, direction);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, BACKLIGHT_LED, direction);
        request.transaction_id.id = 0x9F;
        break;

    default:
        request = razer_chroma_standard_matrix_effect_wave(direction);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_wheel"
 *
 * When 1 is written (as a character, 0x31) the wheel effect is turning right
 * if 2 is written (0x32) then the wheel effect goes left.
 */
static ssize_t razer_attr_write_matrix_effect_wheel(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_wheel(VARSTORE, BACKLIGHT_LED, direction);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razerkbd: matrix_effect_wheel not supported for this model\n");
        return -EINVAL;
    }
    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_spectrum"
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_ANANSI:
        request = razer_chroma_standard_set_led_state(VARSTORE, BACKLIGHT_LED, ON);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        request = razer_chroma_standard_set_led_effect(VARSTORE, BACKLIGHT_LED, CLASSIC_EFFECT_SPECTRUM);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        request = razer_chroma_standard_matrix_effect_spectrum();
        request.transaction_id.id = 0x3F;
        break;

    default:
        request = razer_chroma_standard_matrix_effect_spectrum();
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_reactive"
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char speed;

    if (count != 4) {
        printk(KERN_WARNING "razerkbd: Reactive only accepts Speed, RGB (4byte)\n");
        return -EINVAL;
    }

    speed = (unsigned char)buf[0];

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, BACKLIGHT_LED, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, BACKLIGHT_LED, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, BACKLIGHT_LED, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        request = razer_chroma_standard_matrix_effect_reactive(speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x3F;
        break;
    default:
        request = razer_chroma_standard_matrix_effect_reactive(speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_static"
 *
 * Set the keyboard to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORBWEAVER:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_EXPERT:
        request = razer_chroma_standard_set_led_effect(VARSTORE, BACKLIGHT_LED, CLASSIC_EFFECT_STATIC);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013: // Doesn't need any parameters as can only do one type of static
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        request = razer_chroma_standard_set_led_effect(VARSTORE, LOGO_LED, CLASSIC_EFFECT_STATIC);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_OVERWATCH:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020:
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020:
    case USB_DEVICE_ID_RAZER_BOOK_2020:
    case USB_DEVICE_ID_RAZER_BLADE_QHD:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_2018:
    case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
    case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
    case USB_DEVICE_ID_RAZER_BLADE_2019_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE:
    case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
    case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
    case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_2019:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021:
    case USB_DEVICE_ID_RAZER_BLADE_14_2021:
    case USB_DEVICE_ID_RAZER_TARTARUS:
    case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
    case USB_DEVICE_ID_RAZER_BLADE_17_2022:
    case USB_DEVICE_ID_RAZER_BLADE_14_2022:
    case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
    case USB_DEVICE_ID_RAZER_BLADE_14_2023:
    case USB_DEVICE_ID_RAZER_BLADE_15_2023:
    case USB_DEVICE_ID_RAZER_BLADE_16_2023:
    case USB_DEVICE_ID_RAZER_BLADE_18_2023:
    case USB_DEVICE_ID_RAZER_BLADE_14_2024:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_standard_matrix_effect_static((struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_standard_matrix_effect_static((struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x1F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x9F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_ANANSI:
        if (count != 3) {
            printk(KERN_WARNING "razerkbd: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_standard_set_led_state(VARSTORE, BACKLIGHT_LED, ON);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        request = razer_chroma_standard_set_led_effect(VARSTORE, BACKLIGHT_LED, CLASSIC_EFFECT_STATIC);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        request = razer_chroma_standard_set_led_rgb(VARSTORE, BACKLIGHT_LED, (struct razer_rgb *) &buf[0]);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;

    default:
        printk(KERN_WARNING "razerkbd: Cannot set static mode for this device\n");
        return -EINVAL;
    }

    return count;
}

/**
 * Write device file "matrix_effect_starlight"
 *
 * Starlight keyboard effect is activated whenever this file is written to (for bw2016)
 *
 * Or if an Ornata
 * 7 bytes, speed, rgb, rgb
 * 4 bytes, speed, rgb
 * 1 byte, speed
 */
static ssize_t razer_attr_write_matrix_effect_starlight(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_rgb rgb1 = {.r = 0x00, .g = 0xFF, .b = 0x00};
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORNATA:
        if (count != 4) {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        if(count == 7) {
            request = razer_chroma_extended_matrix_effect_starlight_dual(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else if(count == 4) {
            request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else if(count == 1) {
            request = razer_chroma_extended_matrix_effect_starlight_random(VARSTORE, BACKLIGHT_LED, buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        if (count == 7) {
            request = razer_chroma_extended_matrix_effect_starlight_dual(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
        } else if(count == 4) {
            request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1]);
        } else if(count == 1) {
            request = razer_chroma_extended_matrix_effect_starlight_random(VARSTORE, BACKLIGHT_LED, buf[0]);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        request.transaction_id.id = 0x1F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        if (count == 7) {
            request = razer_chroma_extended_matrix_effect_starlight_dual(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
        } else if(count == 4) {
            request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1]);
        } else if(count == 1) {
            request = razer_chroma_extended_matrix_effect_starlight_random(VARSTORE, BACKLIGHT_LED, buf[0]);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        request.transaction_id.id = 0x9F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        if(count == 7) {
            request = razer_chroma_extended_matrix_effect_starlight_dual(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
            request.transaction_id.id = 0x1F;
            razer_send_payload(device, &request, &response);
        } else if(count == 4) {
            request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, BACKLIGHT_LED, buf[0], (struct razer_rgb*)&buf[1]);
            request.transaction_id.id = 0x1F;
            razer_send_payload(device, &request, &response);
        } else if(count == 1) {
            request = razer_chroma_extended_matrix_effect_starlight_random(VARSTORE, BACKLIGHT_LED, buf[0]);
            request.transaction_id.id = 0x1F;
            razer_send_payload(device, &request, &response);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
    case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
        if(count == 7) {
            request = razer_chroma_standard_matrix_effect_starlight_dual(buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
        } else if(count == 4) {
            request = razer_chroma_standard_matrix_effect_starlight_single(buf[0], (struct razer_rgb*)&buf[1]);
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
        } else if(count == 1) {
            request = razer_chroma_standard_matrix_effect_starlight_random(buf[0]);
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        if(count == 7) {
            request = razer_chroma_standard_matrix_effect_starlight_dual(buf[0], (struct razer_rgb*)&buf[1], (struct razer_rgb*)&buf[4]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else if(count == 4) {
            request = razer_chroma_standard_matrix_effect_starlight_single(buf[0], (struct razer_rgb*)&buf[1]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else if(count == 1) {
            request = razer_chroma_standard_matrix_effect_starlight_random(buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
        } else {
            printk(KERN_WARNING "razerkbd: Starlight only accepts Speed (1byte). Speed, RGB (4byte). Speed, RGB, RGB (7byte)\n");
            return -EINVAL;
        }
        break;

    default: // BW2016 can do normal starlight
        request = razer_chroma_standard_matrix_effect_starlight_single(0x01, &rgb1);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;
    }

    return count;
}

/**
 * Write device file "matrix_effect_breath"
 */
static ssize_t razer_attr_write_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        default:
            printk(KERN_WARNING "razerkbd: Breathing only accepts '1' (1byte). RGB (3byte). RGB, RGB (6byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            request.transaction_id.id = 0x1F;
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            request.transaction_id.id = 0x1F;
            break;

        case 1: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, BACKLIGHT_LED);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            request.transaction_id.id = 0x1F;
            break;

        default:
            printk(KERN_WARNING "razerkbd: Breathing only accepts '1' (1byte). RGB (3byte). RGB, RGB (6byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        case 1: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, BACKLIGHT_LED);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        default:
            printk(KERN_WARNING "razerkbd: Breathing only accepts '1' (1byte). RGB (3byte). RGB, RGB (6byte)\n");
            return -EINVAL;
        }
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        if (count == 3) { // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
        } else if (count == 6) { // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
        } else if (count == 1) { // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, BACKLIGHT_LED);
        } else {
            printk(KERN_WARNING "razerkbd: Breathing only accepts '1' (1byte). RGB (3byte). RGB, RGB (6byte)\n");
            return -EINVAL;
        }
        request.transaction_id.id = 0x1F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        if (count == 3) { // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
        } else if (count == 6) { // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
        } else if (count == 1) { // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, BACKLIGHT_LED);
        } else {
            printk(KERN_WARNING "razerkbd: Breathing only accepts '1' (1byte). RGB (3byte). RGB, RGB (6byte)\n");
            return -EINVAL;
        }
        request.transaction_id.id = 0x9F;
        razer_send_payload(device, &request, &response);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_standard_matrix_effect_breathing_single((struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_standard_matrix_effect_breathing_dual((struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;

        default: // "Random" colour mode
            request = razer_chroma_standard_matrix_effect_breathing_random();
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            break;
            // TODO move default to case 1:. Then default: printk(warning). Also remove pointless buffer
        }
        break;

    default:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_standard_matrix_effect_breathing_single((struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_standard_matrix_effect_breathing_dual((struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
            break;

        default: // "Random" colour mode
            request = razer_chroma_standard_matrix_effect_breathing_random();
            request.transaction_id.id = 0xFF;
            razer_send_payload(device, &request, &response);
            break;
            // TODO move default to case 1:. Then default: printk(warning). Also remove pointless buffer
        }
        break;
    }

    return count;
}

static int has_inverted_led_state(struct device *dev)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
    case USB_DEVICE_ID_RAZER_BLADE_QHD:
    case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
        return 1;
    default:
        return 0;
    }
}

/**
 * Reads device file "logo_led_state"
 *
 * Reads the logo lighting state (the ASCII number) written to this file.
 */
static ssize_t razer_attr_read_logo_led_state(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    int state;

    request = razer_chroma_standard_get_led_effect(VARSTORE, LOGO_LED);
    request.transaction_id.id = 0xFF;

    // Blade laptops don't use effect for logo on/off, and mode 2 ("blink") is technically unsupported.
    if (is_blade_laptop(device)) {
        request = razer_chroma_standard_get_led_state(VARSTORE, LOGO_LED);
        request.transaction_id.id = 0xFF;
    }

    razer_send_payload(device, &request, &response);
    state = response.arguments[2];

    if (has_inverted_led_state(dev) && (state == 0 || state == 1))
        state = !state;

    return sprintf(buf, "%d\n", state);
}

/**
 * Write device file "logo_led_state"
 *
 * Sets the logo lighting state to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_logo_led_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char state = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (has_inverted_led_state(dev) && (state == 0 || state == 1))
        state = !state;

    // Blade laptops are... different. They use state instead of effect.
    // Note: This does allow setting of mode 2 ("blink"), but this is an undocumented feature.
    if (is_blade_laptop(device) && (state == 0 || state == 1)) {
        request = razer_chroma_standard_set_led_state(VARSTORE, LOGO_LED, state);
        request.transaction_id.id = 0xFF;
    } else {
        request = razer_chroma_standard_set_led_effect(VARSTORE, LOGO_LED, state);
        request.transaction_id.id = 0xFF;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_effect_custom"
 *
 * Sets the keyboard to custom mode whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    bool want_response = true;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x1F;
        want_response = false;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
    case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0x3F;
        break;

    default:
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0xFF;
        break;
    }

    /* See comment in razer_attr_write_matrix_custom_frame for want_response */
    if (want_response)
        razer_send_payload(device, &request, &response);
    else
        razer_send_payload_no_response(device, &request);

    return count;
}

/**
 * Write device file "fn_toggle"
 *
 * Sets the logo lighting state to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_fn_toggle(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char state = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_fn_key_toggle(state);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "test"
 *
 * Does nothing
 */
static ssize_t razer_attr_write_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}

/**
 * Read device file "test"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_test(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = get_razer_report(0x00, 0x86, 0x02);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    print_erroneous_report(&response, "razerkbd", "Test");
    return sprintf(buf, "%02x%02x%02x\n", response.arguments[0], response.arguments[1], response.arguments[2]);
}

/**
 * Write device file "matrix_brightness"
 *
 * Sets the brightness to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_matrix_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char brightness = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        request = razer_chroma_standard_set_led_brightness(VARSTORE, LOGO_LED, brightness);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NOSTROMO:
    default:
        if (is_blade_laptop(device)) {
            request = razer_chroma_misc_set_blade_brightness(brightness);
        } else {
            request = razer_chroma_standard_set_led_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        }
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "matrix_brightness"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_matrix_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned char brightness = 0;
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {

    case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
    case USB_DEVICE_ID_RAZER_ORNATA:
    case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
    case USB_DEVICE_ID_RAZER_HUNTSMAN:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
    case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
    case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_CYNOSA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x9F;
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        request = razer_chroma_standard_get_led_brightness(VARSTORE, LOGO_LED);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NOSTROMO:
    default:
        if (is_blade_laptop(device)) {
            request = razer_chroma_misc_get_blade_brightness();
        } else {
            request = razer_chroma_standard_get_led_brightness(VARSTORE, BACKLIGHT_LED);
        }
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    // Brightness is stored elsewhere for the stealth cmds
    if (is_blade_laptop(device)) {
        brightness = response.arguments[1];
    } else {
        brightness = response.arguments[2];
    }

    return sprintf(buf, "%d\n", brightness);
}

/**
 * Write device file "device_mode"
 */
static ssize_t razer_attr_write_device_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 2) {
        printk(KERN_WARNING "razerkbd: Device mode only takes 2 bytes.\n");
        return -EINVAL;
    }

    // No-op on Blades
    if (is_blade_laptop(device)) {
        return count;
    }

    request = razer_chroma_standard_set_device_mode(buf[0], buf[1]);
    request.transaction_id.id = 0xFF;
    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "device_mode"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_device_mode();
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    buf[0] = response.arguments[0];
    buf[1] = response.arguments[1];

    return 2;
}

/**
 * Write device file "matrix_custom_frame"
 *
 * Format
 * ROW_ID START_COL STOP_COL RGB...
 */
static ssize_t razer_attr_write_matrix_custom_frame(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    size_t offset = 0;
    unsigned char row_id;
    unsigned char start_col;
    unsigned char stop_col;
    unsigned char row_length;
    bool want_response = true;

    //printk(KERN_ALERT "razerkbd: Total count: %d\n", (unsigned char)count);

    while(offset < count) {
        if(offset + 3 > count) {
            printk(KERN_ALERT "razerkbd: Wrong Amount of data provided: Should be ROW_ID, START_COL, STOP_COL, N_RGB\n");
            return -EINVAL;
        }

        row_id = buf[offset++];
        start_col = buf[offset++];
        stop_col = buf[offset++];
        row_length = ((stop_col+1) - start_col) * 3;

        // printk(KERN_ALERT "razerkbd: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_id, start_col, stop_col, row_length);

        if(start_col > stop_col) {
            printk(KERN_ALERT "razerkbd: Start column is greater than end column\n");
            return -EINVAL;
        }

        if(offset + row_length > count) {
            printk(KERN_ALERT "razerkbd: Not enough RGB to fill row\n");
            return -EINVAL;
        }

        // Offset now at beginning of RGB data
        switch (device->usb_pid) {
        case USB_DEVICE_ID_RAZER_ORNATA:
        case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
        case USB_DEVICE_ID_RAZER_HUNTSMAN:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x3F;
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS_V2:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
        case USB_DEVICE_ID_RAZER_CYNOSA_V2:
        case USB_DEVICE_ID_RAZER_ORNATA_V2:
        case USB_DEVICE_ID_RAZER_ORNATA_V3:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x1F;
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x1F;
            want_response = false;
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x9F;
            break;

        case USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA:
            request = razer_chroma_misc_one_row_set_custom_frame(start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0xFF;
            break;

        case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
            request = razer_chroma_standard_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x3F;
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
        case USB_DEVICE_ID_RAZER_BLADE_QHD:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_2018:
        case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
        case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_14_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
            // FIXME this seems not to do anything?
            request.transaction_id.id = 0x80; // Fall into the 2016/blade/blade2016 to set device id
            fallthrough;
        default:
            request = razer_chroma_standard_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0xFF;
            break;
        }

        /*
         * Some devices don't like us asking for responses for custom frame
         * requests. And in any case it shouldn't be necessary for most devices
         * but let's keep it enabled by default for now to not potentially
         * break anything.
         */
        if (want_response)
            razer_send_payload(device, &request, &response);
        else
            razer_send_payload_no_response(device, &request);

        // *3 as its 3 bytes per col (RGB)
        offset += row_length;
    }

    return count;
}

/**
 * Read device file "poll_rate"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_poll_rate(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short polling_rate = 0;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_misc_get_polling_rate2();
        request.transaction_id.id = 0x1f;
        break;

    default:
        request = razer_chroma_misc_get_polling_rate();
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    switch(response.arguments[1]) {
    case 0x01:
        polling_rate = 8000;
        break;
    case 0x02:
        polling_rate = 4000;
        break;
    case 0x04:
        polling_rate = 2000;
        break;
    case 0x08:
        polling_rate = 1000;
        break;
    case 0x10:
        polling_rate = 500;
        break;
    case 0x20:
        polling_rate = 250;
        break;
    case 0x40:
        polling_rate = 125;
        break;
    }

    return sprintf(buf, "%d\n", polling_rate);
}

/**
 * Write device file "poll_rate"
 *
 * Sets the poll rate
 */
static ssize_t razer_attr_write_poll_rate(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);
    unsigned short polling_rate = (unsigned short)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
        request = razer_chroma_misc_set_polling_rate2(polling_rate, 0x00);
        request.transaction_id.id = 0x1f;
        break;
    default:
        request = razer_chroma_misc_set_polling_rate(polling_rate);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "key_super"
 */
static ssize_t razer_attr_write_key_super(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    if (count < 1) {
        printk(KERN_ALERT "razerkbd: Failed to provide argument\n");
        return -EINVAL;
    }

    device->block_keys[0] = buf[0];

    return count;
}

/**
 * Read device file "key_super"
 */
static ssize_t razer_attr_read_key_super(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    buf[0] = device->block_keys[0];

    return 1;
}

/**
 * Write device file "key_alt_tab"
 */
static ssize_t razer_attr_write_key_alt_tab(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    if (count < 1) {
        printk(KERN_ALERT "razerkbd: Failed to provide argument\n");
        return -EINVAL;
    }

    printk(KERN_WARNING "razerkbd: Settings block_keys[1] to %u\n", buf[0]);
    device->block_keys[1] = buf[0];

    return count;
}

/**
 * Read device file "read_key_alt_tab"
 */
static ssize_t razer_attr_read_key_alt_tab(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    buf[0] = device->block_keys[1];

    return 1;
}

/**
 * Write device file "write_key_alt_f4"
 */
static ssize_t razer_attr_write_key_alt_f4(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    if (count < 1) {
        printk(KERN_ALERT "razerkbd: Failed to provide argument\n");
        return -EINVAL;
    }

    device->block_keys[2] = buf[0];

    return count;
}

/**
 * Read device file "read_key_alt_f4"
 */
static ssize_t razer_attr_read_key_alt_f4(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kbd_device *device = dev_get_drvdata(dev);

    buf[0] = device->block_keys[2];

    return 1;
}

/**
 * Set up the device driver files

 *
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */
static DEVICE_ATTR(game_led_state,          0660, razer_attr_read_game_led_state,             razer_attr_write_game_led_state);
static DEVICE_ATTR(macro_led_state,         0660, razer_attr_read_macro_led_state,            razer_attr_write_macro_led_state);
static DEVICE_ATTR(macro_led_effect,        0660, razer_attr_read_macro_led_effect,           razer_attr_write_macro_led_effect);
static DEVICE_ATTR(logo_led_state,          0660, razer_attr_read_logo_led_state,             razer_attr_write_logo_led_state);
static DEVICE_ATTR(profile_led_red,         0660, razer_attr_read_profile_led_red,            razer_attr_write_profile_led_red);
static DEVICE_ATTR(profile_led_green,       0660, razer_attr_read_profile_led_green,          razer_attr_write_profile_led_green);
static DEVICE_ATTR(profile_led_blue,        0660, razer_attr_read_profile_led_blue,           razer_attr_write_profile_led_blue);

static DEVICE_ATTR(test,                    0660, razer_attr_read_test,                       razer_attr_write_test);
static DEVICE_ATTR(version,                 0440, razer_attr_read_version,                    NULL);
static DEVICE_ATTR(kbd_layout,              0440, razer_attr_read_kbd_layout,                 NULL);

static DEVICE_ATTR(firmware_version,        0440, razer_attr_read_firmware_version,           NULL);
static DEVICE_ATTR(fn_toggle,               0220, NULL,                                       razer_attr_write_fn_toggle);
static DEVICE_ATTR(poll_rate,               0660, razer_attr_read_poll_rate,                  razer_attr_write_poll_rate);
static DEVICE_ATTR(keyswitch_optimization,  0660, razer_attr_read_keyswitch_optimization,     razer_attr_write_keyswitch_optimization);

static DEVICE_ATTR(device_type,             0440, razer_attr_read_device_type,                NULL);
static DEVICE_ATTR(device_mode,             0660, razer_attr_read_device_mode,                razer_attr_write_device_mode);
static DEVICE_ATTR(device_serial,           0440, razer_attr_read_device_serial,              NULL);

static DEVICE_ATTR(matrix_effect_none,      0220, NULL,                                       razer_attr_write_matrix_effect_none);
static DEVICE_ATTR(matrix_effect_wave,      0220, NULL,                                       razer_attr_write_matrix_effect_wave);
static DEVICE_ATTR(matrix_effect_wheel,     0220, NULL,                                       razer_attr_write_matrix_effect_wheel);
static DEVICE_ATTR(matrix_effect_spectrum,  0220, NULL,                                       razer_attr_write_matrix_effect_spectrum);
static DEVICE_ATTR(matrix_effect_reactive,  0220, NULL,                                       razer_attr_write_matrix_effect_reactive);
static DEVICE_ATTR(matrix_effect_static,    0220, NULL,                                       razer_attr_write_matrix_effect_static);
static DEVICE_ATTR(matrix_effect_starlight, 0220, NULL,                                       razer_attr_write_matrix_effect_starlight);
static DEVICE_ATTR(matrix_effect_breath,    0220, NULL,                                       razer_attr_write_matrix_effect_breath);
static DEVICE_ATTR(matrix_effect_pulsate,   0660, razer_attr_read_matrix_effect_pulsate,      razer_attr_write_matrix_effect_pulsate);
static DEVICE_ATTR(matrix_brightness,       0660, razer_attr_read_matrix_brightness,          razer_attr_write_matrix_brightness);
static DEVICE_ATTR(matrix_effect_custom,    0220, NULL,                                       razer_attr_write_matrix_effect_custom);
static DEVICE_ATTR(matrix_custom_frame,     0220, NULL,                                       razer_attr_write_matrix_custom_frame);

static DEVICE_ATTR(key_super,               0660, razer_attr_read_key_super,                  razer_attr_write_key_super);
static DEVICE_ATTR(key_alt_tab,             0660, razer_attr_read_key_alt_tab,                razer_attr_write_key_alt_tab);
static DEVICE_ATTR(key_alt_f4,              0660, razer_attr_read_key_alt_f4,                 razer_attr_write_key_alt_f4);

static DEVICE_ATTR(charge_level,            0440, razer_attr_read_charge_level,               NULL);
static DEVICE_ATTR(charge_status,           0440, razer_attr_read_charge_status,              NULL);
static DEVICE_ATTR(charge_effect,           0220, NULL,                                       razer_attr_write_charge_effect);
static DEVICE_ATTR(charge_colour,           0220, NULL,                                       razer_attr_write_charge_colour);
static DEVICE_ATTR(charge_low_threshold,    0660, razer_attr_read_charge_low_threshold,       razer_attr_write_charge_low_threshold);

/**
 * Deal with FN toggle
 */
static int razer_event(struct hid_device *hdev, struct hid_field *field, struct hid_usage *usage, __s32 value)
{
    struct razer_kbd_device *device = hid_get_drvdata(hdev);
    const struct razer_key_translation *translation;

    // No translations needed on the Blades
    if (is_blade_laptop(device)) {
        return 0;
    }

    if(device->usb_interface_protocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        // Skip this if its control (mouse) interface
        return 0;
    }

    // Block win key
    if(device->block_keys[0] && (usage->code == KEY_LEFTMETA || usage->code == KEY_RIGHTMETA)) {
        return 1;
    }

    // Store Alt state
    if(usage->code == KEY_LEFTALT) {
        device->left_alt_on = value;
    }
    // Block Alt-Tab
    if(device->block_keys[1] && device->left_alt_on && usage->code == KEY_TAB) {
        return 1;
    }
    // Block Alt-F4
    if(device->block_keys[2] && device->left_alt_on && usage->code == KEY_F4) {
        return 1;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
        translation = find_translation(chroma_keys_2, usage->code);
        break;

    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
        translation = find_translation(chroma_keys_3, usage->code);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
        translation = find_translation(chroma_keys_4, usage->code);
        break;

    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
        translation = find_translation(chroma_keys_5, usage->code);
        break;

    default:
        translation = find_translation(chroma_keys, usage->code);
        break;
    }

    if(translation) {
        if (test_bit(usage->code, device->pressed_fn) || device->fn_on) {
            if (value) {
                set_bit(usage->code, device->pressed_fn);
            } else {
                clear_bit(usage->code, device->pressed_fn);
            }

            input_event(field->hidinput->input, usage->type, translation->to, value);
            return 1;
        }
    }

    return 0;
}

/**
 * Standard raw event function
 *
 * Bastard function. Could most probably be done a load better.
 * Basically it shifts all of the key's in the 04... event to the right 1, and then sets the first 2 bytes to 0x0100. This then allows the keys to be processed with the above normal event function
 *
 * Converts M1-M5 into F13-F17. It also blanks out FN keypresses so it acts more like the modifier it should be.
 * 04012000000000000000 FN is pressed, M1 pressed
 * 04010000000000000000 M1 is released
 * goes to
 * 01000068000000000000 FN is pressed (blanked), M1 pressed (converted to F13)
 * 01000000000000000000 M1 is released
 *
 * Converts Mute/Next/Play/Prev into multimedia keys
 *   04 00 52 00  ... 00 - Mute key pressed
 *   04 00 00 00 ... 00 - Mute key released
 * goes to
 *   01 00 00 E2 00 ... 00 - Mute pressed (converted to KEY_MEDIA_MUTE)
 *   01 00 00 00 00 ... 00
 * they key codes are
 *   0x52 - Mute
 *   0x53 - Next song
 *   0x55 - Play/Pause
 *   0x54 - Prev song
 *
 * HID Usage Table http://www.freebsddiary.org/APC/usb_hid_usages.php
 */
static int razer_raw_event_standard(struct hid_device *hdev, struct razer_kbd_device *device, struct usb_interface *intf, struct hid_report *report, u8 *data, int size)
{
    // The event were looking for is 16 or 22 bytes long and starts with 0x04.
    // Newer firmware seems to use 22 bytes.
    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD &&
       ((size == 22) || (size == 16)) && data[0] == 0x04) {
        // Convert 04... to 0100...
        int index = size-1; // This way we start at 2nd last value, does subtract 1 from the 15key rollover though (not an issue cmon)
        int found_fn = 0x00;

        while(--index > 0) {
            u8 cur_value = data[index];
            if(cur_value == 0x00) { // Skip 0x00
                continue;
            }

            switch(cur_value) {
            case 0x01: // FN
                //cur_value = 0x73; // F24
                cur_value = 0x00;
                found_fn = 0x01;
                break;
            case 0x20: // M1
                cur_value = USB_HID_KEY_F13; // F13
                break;
            case 0x21: // M2
                cur_value = USB_HID_KEY_F14; // F14
                break;
            case 0x22: // M3
                cur_value = USB_HID_KEY_F15; // F15
                break;
            case 0x23: // M4
                cur_value = USB_HID_KEY_F16; // F16
                break;
            case 0x24: // M5
                cur_value = USB_HID_KEY_F17; // F17
                break;
            case 0x25: // BlackWidow V4 (non-Pro) M6
                cur_value = USB_HID_KEY_F18; // F18
                break;
            case 0x50: // Volume Down
                cur_value = USB_HID_KEY_MEDIA_VOLUMEDOWN; // F17
                break;
            case 0x51: // Volume Up
                cur_value =  USB_HID_KEY_MEDIA_VOLUMEUP; // F17
                break;
            case 0x52: // Mute
                cur_value = USB_HID_KEY_MEDIA_MUTE;
                break;
            case 0x53: // Next (song)
                cur_value = USB_HID_KEY_MEDIA_NEXTSONG;
                break;
            case 0x55: // Play/Pause
                cur_value = USB_HID_KEY_MEDIA_PLAYPAUSE;
                break;
            case 0x54: // Prev (song)
                cur_value = USB_HID_KEY_MEDIA_PREVIOUSSONG;
                break;
            case 0x60: // BlackWidow V4 Pro command dial button
                cur_value = USB_HID_KEY_F24; // F24 (not sure if we want it this way)
                break;
            case 0x63: // BlackWidow V4 Pro Side button 1
                cur_value = USB_HID_KEY_F18; // F18
                break;
            case 0x64: // BlackWidow V4 Pro Side button 2
                cur_value = USB_HID_KEY_F19; // F19
                break;
            case 0x65: // BlackWidow V4 Pro Side button 3
                cur_value = USB_HID_KEY_F20; // F20
                break;
            }

            data[index+1] = cur_value;
        }

        device->fn_on = !!found_fn;

        data[0] = 0x01;
        data[1] = 0x00;

        // Some reason just by editing data, it generates a normal event above. (Could quite possibly work like that, no clue)
        //hid_report_raw_event(hdev, HID_INPUT_REPORT, data, size, 0);
        return 1;
    }

    return 0;
}

/**
 * Bitfield raw event function
 *
 * Handles raw events very similarly to razer_raw_event_standard, but for size 22, handles the data as a bit field,
 * instead of an array of values.
 *
 * When the rewritten value does not fit the bit field, a key-down and a key-up event is reported separately.
 */
static int razer_raw_event_bitfield(struct hid_device *hdev, struct razer_kbd_device *device, struct usb_interface *intf, struct hid_report *report, u8 *data, int size)
{
    u8 bitfield[20] = { 0 };

    // The event were looking for is 16 or 22 bytes long and starts with 0x04.
    // Newer firmware seems to use 22 bytes.
    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD &&
       ((size == 22) || (size == 16)) && data[0] == 0x04) {
        // Convert 04... to 0100...
        int index = size-1; // This way we start at 2nd last value, does subtract 1 from the 15key rollover though (not an issue cmon)
        int found_fn = 0x00;

        while(--index > 0) {
            bool write_bitfield = true;
            u8 cur_value = data[index];
            if(cur_value == 0x00) { // Skip 0x00
                continue;
            }

            switch(cur_value) {
            case 0x01: // FN
                //cur_value = 0x73; // F24
                cur_value = 0x00;
                found_fn = 0x01;
                write_bitfield = false;
                break;
            case 0x20: // M1
                cur_value = USB_HID_KEY_F13; // F13
                break;
            case 0x21: // M2
                cur_value = USB_HID_KEY_F14; // F14
                break;
            case 0x22: // M3
                cur_value = USB_HID_KEY_F15; // F15
                break;
            case 0x23: // M4
                cur_value = USB_HID_KEY_F16; // F16
                break;
            case 0x24: // M5
                cur_value = USB_HID_KEY_F17; // F17
                break;
            case 0x25: // BlackWidow V4 (non-Pro) M6
                cur_value = USB_HID_KEY_F18; // F18
                break;
            case 0x50: // Volume Down
                cur_value = USB_HID_KEY_MEDIA_VOLUMEDOWN;
                break;
            case 0x51: // Volume Up
                cur_value =  USB_HID_KEY_MEDIA_VOLUMEUP;
                break;
            case 0x52: // Mute
                cur_value = USB_HID_KEY_MEDIA_MUTE;
                break;
            case 0x53: // Next (song)
                cur_value = USB_HID_KEY_MEDIA_NEXTSONG;
                break;
            case 0x55: // Play/Pause
                cur_value = USB_HID_KEY_MEDIA_PLAYPAUSE;
                break;
            case 0x54: // Prev (song)
                cur_value = USB_HID_KEY_MEDIA_PREVIOUSSONG;
                break;
            case 0x60: // BlackWidow V4 Pro command dial button
                cur_value = USB_HID_KEY_F24; // F24 (not sure if we want it this way)
                break;
            case 0x63: // BlackWidow V4 Pro Side button 1
                cur_value = USB_HID_KEY_F18; // F18
                break;
            case 0x64: // BlackWidow V4 Pro Side button 2
                cur_value = USB_HID_KEY_F19; // F19
                break;
            case 0x65: // BlackWidow V4 Pro Side button 3
                cur_value = USB_HID_KEY_F20; // F20
                break;
            default:
                write_bitfield = false;
            }

            // data of size 22 starting with 0x01 is a bit field so we need to handle that separately
            if (size == 22) {
                if (write_bitfield) {
                    if (cur_value < (sizeof(bitfield) * BITS_PER_BYTE)) {
                        // value fits the bit field, so we can use that
                        set_bit(cur_value, (unsigned long*) bitfield);
                    } else {
                        // value does not fit the bit field, so we need extra handling
                        int report_extra = 1;

                        switch (cur_value) {
                        case USB_HID_KEY_MEDIA_VOLUMEUP:
                            cur_value = USB_HID_USAGE_MEDIA_VOLUMEUP;
                            break;
                        case USB_HID_KEY_MEDIA_VOLUMEDOWN:
                            cur_value = USB_HID_USAGE_MEDIA_VOLUMEDOWN;
                            break;
                        case USB_HID_KEY_MEDIA_MUTE:
                            cur_value = USB_HID_USAGE_MEDIA_MUTE;
                            break;
                        case USB_HID_KEY_MEDIA_NEXTSONG:
                            cur_value = USB_HID_USAGE_MEDIA_NEXTSONG;
                            break;
                        case USB_HID_KEY_MEDIA_PLAYPAUSE:
                            cur_value = USB_HID_USAGE_MEDIA_PLAYPAUSE;
                            break;
                        case USB_HID_KEY_MEDIA_PREVIOUSSONG:
                            cur_value = USB_HID_USAGE_MEDIA_PREVIOUSSONG;
                            break;
                        default:
                            report_extra = 0;
                        }

                        if (report_extra) {
                            u8 xdata[22] = { 0x02 };

                            // report key down
                            xdata[1] = cur_value;
                            hid_report_raw_event(hdev, HID_INPUT_REPORT, xdata, sizeof(xdata), 0);

                            // report key up
                            xdata[1] = 0x00;
                            hid_report_raw_event(hdev, HID_INPUT_REPORT, xdata, sizeof(xdata), 0);
                        }
                    }
                }
            } else { // size 16
                data[index+1] = cur_value;
            }
        }

        device->fn_on = !!found_fn;

        data[0] = 0x01;
        data[1] = 0x00;
        if (size == 22) {
            memcpy(data + 2, bitfield, sizeof(bitfield));
        }

        // Some reason just by editing data, it generates a normal event above. (Could quite possibly work like that, no clue)
        //hid_report_raw_event(hdev, HID_INPUT_REPORT, data, size, 0);
        return 1;
    }

    return 0;
}

/**
 * Raw event function
 *
 * Handles provided HID reports, branched out for specific keyboard models, since some keyboards need specific handling.
 */
static int razer_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
    struct razer_kbd_device *device = hid_get_drvdata(hdev);
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);

    // No translations needed on the Pro...
    if (is_blade_laptop(device)) {
        return 0;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_ORNATA_V3:
    case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
        return razer_raw_event_bitfield(hdev, device, intf, report, data, size);
    default:
        return razer_raw_event_standard(hdev, device, intf, report, data, size);
    }
}

/**
 * Set static hid-events translation map
 *
 * Some keyboards generates wheel-events for volume control knob
 */
static int razer_kbd_input_mapping(struct hid_device *hdev, struct hid_input *hidinput, struct hid_field *field,
                                   struct hid_usage *usage, unsigned long **bit, int *max)
{
    switch (hdev->product) {
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
    case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
    case USB_DEVICE_ID_RAZER_ORNATA_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
    case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
        if (hdev->type == HID_TYPE_USBMOUSE && usage->hid == HID_GD_WHEEL) {
            hid_map_usage(hidinput, usage, bit, max, EV_ABS, ABS_VOLUME);
            return 1;
        }
        return 0;

    default:
        return 0;
    }
}

static void razer_kbd_init(struct razer_kbd_device *dev, struct usb_interface *intf, struct hid_device *hdev)
{
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    // Initialise mutex
    mutex_init(&dev->lock);
    // Setup values
    dev->usb_dev = usb_dev;
    dev->usb_vid = usb_dev->descriptor.idVendor;
    dev->usb_pid = usb_dev->descriptor.idProduct;
    dev->usb_interface_protocol = intf->cur_altsetting->desc.bInterfaceProtocol;
}

/**
 * Probe method is ran whenever a device is binded to the driver
 */
static int razer_kbd_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_kbd_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_kbd_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        return -ENOMEM;
    }

    // Init data
    razer_kbd_init(dev, intf, hdev);

    // Other interfaces are actual key-emitting devices
    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        // If the currently bound device is the control (mouse) interface
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);                      // Get the firmware version
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);                         // Get serial number
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);                     // Gets and sets the brightness
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_test);                                  // Test mode
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);                           // Get device mode
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_kbd_layout);                            // Gets the physical layout

        switch(usb_dev->descriptor.idProduct) {

        case USB_DEVICE_ID_RAZER_NOSTROMO:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            break;

        case USB_DEVICE_ID_RAZER_ORBWEAVER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            break;

        case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ANANSI:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_EXPERT:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020:
        case USB_DEVICE_ID_RAZER_BOOK_2020:
        case USB_DEVICE_ID_RAZER_BLADE_2019_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE:
        case USB_DEVICE_ID_RAZER_ORNATA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_super);                     // Super Key
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_tab);                   // Alt + Tab
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_f4);                    // Alt + F4
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);                  // Charge level
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);                 // Charge status
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);                     // Poll Rate
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_keyswitch_optimization);        // Keyswitch Optimization
            fallthrough;
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_super);                     // Super Key
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_tab);                   // Alt + Tab
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_f4);                    // Alt + F4
            fallthrough;
        case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
        case USB_DEVICE_ID_RAZER_ORNATA_V2:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
        case USB_DEVICE_ID_RAZER_HUNTSMAN:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        case USB_DEVICE_ID_RAZER_CYNOSA_V2:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ORNATA_V3:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wheel);           // Wheel effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wheel);           // Wheel effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);                     // Poll Rate
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);                  // Battery charge level
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);                 // Battery charge status
            break;

        case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);                  // Charge level
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);                 // Charge status
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);                 // Charge effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);                 // Charge colour
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);          // Charge low threshold
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            break;

        case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
        case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fn_toggle);                     // Sets whether FN is requires for F-Keys
            break;

        case USB_DEVICE_ID_RAZER_BLADE_QHD:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fn_toggle);                     // Sets whether FN is requires for F-Keys
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_state);                // Enable/Disable the logo
            break;

        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_2018:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
        case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2019:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_14_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_2022:
        case USB_DEVICE_ID_RAZER_BLADE_14_2022:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
        case USB_DEVICE_ID_RAZER_BLADE_14_2023:
        case USB_DEVICE_ID_RAZER_BLADE_15_2023:
        case USB_DEVICE_ID_RAZER_BLADE_16_2023:
        case USB_DEVICE_ID_RAZER_BLADE_18_2023:
        case USB_DEVICE_ID_RAZER_BLADE_14_2024:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_state);                // Enable/Disable the logo
            break;

        case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_OVERWATCH:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA_TE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;
        }

        // Set device to regular mode, not driver mode
        // When the daemon discovers the device it will instruct it to enter driver mode
        razer_set_device_mode(dev, 0x00, 0x00);
    } else if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD) {
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_super);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_tab);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_key_alt_f4);
    }

    hid_set_drvdata(hdev, dev);
    dev_set_drvdata(&hdev->dev, dev);

    if(hid_parse(hdev)) {
        hid_err(hdev, "parse failed\n");
        goto exit_free;
    }

    if (hid_hw_start(hdev, HID_CONNECT_DEFAULT)) {
        hid_err(hdev, "hw start failed\n");
        goto exit_free;
    }

    // Leave autosuspend on for laptops
    if (!is_blade_laptop(dev)) {
        usb_disable_autosuspend(usb_dev);
    }

    //razer_activate_macro_keys(usb_dev);
    //msleep(3000);
    return 0;

exit_free:
    kfree(dev);
    return retval;
}

/**
 * Unbind function
 */
static void razer_kbd_disconnect(struct hid_device *hdev)
{
    struct razer_kbd_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);

    // Other interfaces are actual key-emitting devices
    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        // If the currently bound device is the control (mouse) interface
        device_remove_file(&hdev->dev, &dev_attr_version);
        device_remove_file(&hdev->dev, &dev_attr_firmware_version);                      // Get the firmware version
        device_remove_file(&hdev->dev, &dev_attr_device_serial);                         // Get serial number
        device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);                     // Gets and sets the brightness
        device_remove_file(&hdev->dev, &dev_attr_test);                                  // Test mode
        device_remove_file(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        device_remove_file(&hdev->dev, &dev_attr_device_mode);                           // Get device mode
        device_remove_file(&hdev->dev, &dev_attr_kbd_layout);                            // Gets the physical layout

        switch(usb_dev->descriptor.idProduct) {

        case USB_DEVICE_ID_RAZER_NOSTROMO:
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS:
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            break;

        case USB_DEVICE_ID_RAZER_ORBWEAVER:
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            break;

        case USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ANANSI:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_EXPERT:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_pulsate);         // Pulsate effect, like breathing
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLADE_2018_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020:
        case USB_DEVICE_ID_RAZER_BOOK_2020:
        case USB_DEVICE_ID_RAZER_BLADE_2019_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE:
        case USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE:
        case USB_DEVICE_ID_RAZER_ORNATA:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_key_super);                     // Super Key
            device_remove_file(&hdev->dev, &dev_attr_key_alt_tab);                   // Alt + Tab
            device_remove_file(&hdev->dev, &dev_attr_key_alt_f4);                    // Alt + F4
            device_remove_file(&hdev->dev, &dev_attr_charge_level);                  // Charge level
            device_remove_file(&hdev->dev, &dev_attr_charge_status);                 // Charge status
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);                     // Poll Rate
            device_remove_file(&hdev->dev, &dev_attr_keyswitch_optimization);        // Keyswitch Optimization
            fallthrough;
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3:
        case USB_DEVICE_ID_RAZER_DEATHSTALKER_V2:
            device_remove_file(&hdev->dev, &dev_attr_key_super);                     // Super Key
            device_remove_file(&hdev->dev, &dev_attr_key_alt_tab);                   // Alt + Tab
            device_remove_file(&hdev->dev, &dev_attr_key_alt_f4);                    // Alt + F4
            fallthrough;
        case USB_DEVICE_ID_RAZER_ORNATA_CHROMA:
        case USB_DEVICE_ID_RAZER_ORNATA_V2:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_TE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_2019:
        case USB_DEVICE_ID_RAZER_HUNTSMAN:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA:
        case USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2:
        case USB_DEVICE_ID_RAZER_CYNOSA_V2:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ORNATA_V3:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wheel);           // Wheel effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wheel);           // Wheel
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);                     // Poll Rate
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            device_remove_file(&hdev->dev, &dev_attr_charge_level);                  // Battery charge level
            device_remove_file(&hdev->dev, &dev_attr_charge_status);                 // Battery charge status
            break;

        case USB_DEVICE_ID_RAZER_CYNOSA_LITE:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_ORNATA_V3_X:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            device_remove_file(&hdev->dev, &dev_attr_charge_level);                  // Charge level
            device_remove_file(&hdev->dev, &dev_attr_charge_status);                 // Charge status
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);                 // Charge effect
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);                 // Charge colour
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);          // Charge low threshold
            break;

        case USB_DEVICE_ID_RAZER_TARTARUS_V2:
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_profile_led_red);               // Profile/Macro LED Red
            device_remove_file(&hdev->dev, &dev_attr_profile_led_green);             // Profile/Macro LED Green
            device_remove_file(&hdev->dev, &dev_attr_profile_led_blue);              // Profile/Macro LED Blue
            break;

        case USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_2019_ADV:
        case USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLADE_LATE_2016:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_fn_toggle);                     // Sets whether FN is requires for F-Keys
            break;

        case USB_DEVICE_ID_RAZER_BLADE_QHD:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017:
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_fn_toggle);                     // Sets whether FN is requires for F-Keys
            device_remove_file(&hdev->dev, &dev_attr_logo_led_state);                // Enable/Disable the logo
            break;

        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016:
        case USB_DEVICE_ID_RAZER_BLADE_2018:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD:
        case USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_2019:
        case USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021:
        case USB_DEVICE_ID_RAZER_BLADE_14_2021:
        case USB_DEVICE_ID_RAZER_BLADE_17_2022:
        case USB_DEVICE_ID_RAZER_BLADE_14_2022:
        case USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022:
        case USB_DEVICE_ID_RAZER_BLADE_14_2023:
        case USB_DEVICE_ID_RAZER_BLADE_15_2023:
        case USB_DEVICE_ID_RAZER_BLADE_16_2023:
        case USB_DEVICE_ID_RAZER_BLADE_18_2023:
        case USB_DEVICE_ID_RAZER_BLADE_14_2024:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);       // Starlight effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_logo_led_state);                // Enable/Disable the logo
            break;

        case USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            break;

        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_OVERWATCH:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA:
        case USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA_TE:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG:
        case USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG:
        case USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);            // Wave effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);        // Reactive effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);           // Set LED matrix
            device_remove_file(&hdev->dev, &dev_attr_game_led_state);                // Enable game mode & LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_state);               // Enable macro LED
            device_remove_file(&hdev->dev, &dev_attr_macro_led_effect);              // Change macro LED effect (static, flashing)
            break;
        }
    } else if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD) {
        device_remove_file(&hdev->dev, &dev_attr_key_super);
        device_remove_file(&hdev->dev, &dev_attr_key_alt_tab);
        device_remove_file(&hdev->dev, &dev_attr_key_alt_f4);
    }

    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

/**
 * Setup input device keybit mask
 */
static void razer_setup_key_bits(struct input_dev *input)
{
    __set_bit(EV_KEY, input->evbit);

    // Chroma keys
    __set_bit(RAZER_MACRO_KEY, input->keybit);
    __set_bit(RAZER_GAME_KEY, input->keybit);
    __set_bit(RAZER_BRIGHTNESS_DOWN, input->keybit);
    __set_bit(RAZER_BRIGHTNESS_UP, input->keybit);
}

/**
 * Setup the input device now that its been added to our struct
 */
static int razer_input_configured(struct hid_device *hdev, struct hid_input *hi)
{
    razer_setup_key_bits(hi->input);
    return 0;
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORBWEAVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORBWEAVER_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NOSTROMO) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_STEALTH_EDITION) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2012) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2013) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ULTIMATE_2016) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_X_ULTIMATE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_TE_2014) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2016) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_QHD) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2016) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_2018) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_2018_MERCURY) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_2018_BASE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_2019_ADV) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_2019_BASE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_MID_2019_MERCURY) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_LATE_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_ADV_LATE_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STUDIO_EDITION_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_ADV_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_TARTARUS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_TARTARUS_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_TARTARUS_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_ESSENTIAL) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_EXPERT) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_OVERWATCH) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_TE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_X_CHROMA_TE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_V3) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_V3_X) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_V3_X_ALT) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA_V3_TENKEYLESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CYNOSA_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CYNOSA_CHROMA_PRO) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CYNOSA_LITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CYNOSA_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_LITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ESSENTIAL) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ORNATA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ANANSI) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_LATE_2016) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_MID_2017) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_2017) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_ELITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_TE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_MINI) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_JP) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_ELITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_2017_FULLHD) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2017) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2019) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_EARLY_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_STEALTH_LATE_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BOOK_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_PRO_EARLY_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_EARLY_2020_BASE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_ADV_MID_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_BASE_EARLY_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_BASE_2022) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_17_PRO_EARLY_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_17_PRO_MID_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_14_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_TK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V3_MINI_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_V2_TENKEYLESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_V2_ANALOG) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HUNTSMAN_MINI_ANALOG) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_17_2022) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_14_2022) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_ADV_EARLY_2022) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_14_2023) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_15_2023) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_14_2024) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V4) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_X) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLACKWIDOW_V4_PRO) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHSTALKER_V2_PRO_TKL_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_16_2023) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BLADE_18_2023) },
    { 0 }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_kbd_driver = {
    .name = "razerkbd",
    .id_table = razer_devices,
    .input_mapping = razer_kbd_input_mapping,
    .probe = razer_kbd_probe,
    .remove = razer_kbd_disconnect,
    .event = razer_event,
    .raw_event = razer_raw_event,
    .input_configured = razer_input_configured,
};

module_hid_driver(razer_kbd_driver);
