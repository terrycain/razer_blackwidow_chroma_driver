// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Terry Cain <terrys-home.co.uk>
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/hrtimer.h>
#include <linux/random.h>

#include "razermouse_driver.h"
#include "razercommon.h"
#include "razerchromacommon.h"

/*
 * Version Information
 */
#define DRIVER_DESC "Razer Mouse Device Driver"

/* REL_HWHEEL_HI_RES was added in Linux 5.0, so define ourselves for older kernels
 * See also https://git.kernel.org/torvalds/c/52ea899 */
#ifndef REL_HWHEEL_HI_RES
#define REL_HWHEEL_HI_RES 0x0c
#endif

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

/**
 * Send report to the mouse
 */
static int razer_get_report(struct usb_device *usb_dev, struct razer_report *request, struct razer_report *response)
{
    unsigned int index = 0;
    switch (usb_dev->descriptor.idProduct) {
    // These devices require longer waits to read their firmware, serial, and other setting values
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        return razer_get_usb_response(usb_dev, index, request, index, response, RAZER_NEW_MOUSE_RECEIVER_WAIT_MIN_US, RAZER_NEW_MOUSE_RECEIVER_WAIT_MAX_US);
        break;

    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
        return razer_get_usb_response(usb_dev, index, request, index, response, RAZER_ATHERIS_RECEIVER_WAIT_MIN_US, RAZER_ATHERIS_RECEIVER_WAIT_MAX_US);
        break;

    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
        return razer_get_usb_response(usb_dev, index, request, index, response, RAZER_VIPER_MOUSE_RECEIVER_WAIT_MIN_US, RAZER_VIPER_MOUSE_RECEIVER_WAIT_MAX_US);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
        index = 0x03;
        return razer_get_usb_response(usb_dev, index, request, index, response, RAZER_MOUSE_WAIT_MIN_US, RAZER_MOUSE_WAIT_MAX_US);
        break;

    default:
        return razer_get_usb_response(usb_dev, index, request, index, response, RAZER_MOUSE_WAIT_MIN_US, RAZER_MOUSE_WAIT_MAX_US);
    }
}

/**
 * Function to send to device, get response, and actually check the response
 */
static int razer_send_payload(struct razer_mouse_device *device, struct razer_report *request, struct razer_report *response)
{
    int err;

    request->crc = razer_calculate_crc(request);

    mutex_lock(&device->lock);
    err = razer_get_report(device->usb_dev, request, response);
    mutex_unlock(&device->lock);
    if (err) {
        print_erroneous_report(response, "razermouse", "Invalid Report Length");
        return err;
    }

    /* Check the packet number, class and command are the same */
    if (response->remaining_packets != request->remaining_packets ||
        response->command_class != request->command_class ||
        response->command_id.id != request->command_id.id) {
        print_erroneous_report(response, "razermouse", "Response doesn't match request");
        return -EIO;
    }

    switch (response->status) {
    case RAZER_CMD_BUSY:
        // TODO: Check if this should be an error.
        // print_erroneous_report(&response, "razermouse", "Device is busy");
        break;
    case RAZER_CMD_FAILURE:
        print_erroneous_report(response, "razermouse", "Command failed");
        return -EIO;
    case RAZER_CMD_NOT_SUPPORTED:
        print_erroneous_report(response, "razermouse", "Command not supported");
        return -EIO;
    case RAZER_CMD_TIMEOUT:
        print_erroneous_report(response, "razermouse", "Command timed out");
        return -EIO;
    }

    return 0;
}

/*
 * Specific functions for ancient devices
 *
 */
static int deathadder3_5g_set_led_state(struct razer_mouse_device *device, unsigned char led_id, bool enabled)
{
    switch (led_id) {
    case SCROLL_WHEEL_LED:
        if (enabled) {
            device->da3_5g.leds |= 0x02;
        } else {
            device->da3_5g.leds &= ~(0x02);
        }
        break;

    case LOGO_LED:
        if (enabled) {
            device->da3_5g.leds |= 0x01;
        } else {
            device->da3_5g.leds &= ~(0x01);
        }
        break;

    default:
        printk(KERN_WARNING "razermouse: Invalid led_id on DeathAdder 3.5G\n");
        return -EINVAL;
    }

    mutex_lock(&device->lock);
    razer_send_control_msg_old_device(device->usb_dev, &device->da3_5g, 0x10, 0x00, 4, 3000, 3000);
    mutex_unlock(&device->lock);

    return 0;
}

static void deathadder3_5g_set_poll_rate(struct razer_mouse_device *device, unsigned short poll_rate)
{
    switch(poll_rate) {
    case 1000:
        device->da3_5g.poll = 1;
        break;
    case 500:
        device->da3_5g.poll = 2;
        break;
    case 125:
        device->da3_5g.poll = 3;
        break;
    default: // 500
        device->da3_5g.poll = 2;
        break;
    }

    mutex_lock(&device->lock);
    razer_send_control_msg_old_device(device->usb_dev, &device->da3_5g, 0x10, 0x00, 4, 3000, 3000);
    mutex_unlock(&device->lock);
}

static void deathadder3_5g_set_dpi(struct razer_mouse_device *device, unsigned short dpi)
{
    switch(dpi) {
    case 450:
        device->da3_5g.dpi = 4;
        break;
    case 900:
        device->da3_5g.dpi = 3;
        break;
    case 1800:
        device->da3_5g.dpi = 2;
        break;
    case 3500:
    default:
        device->da3_5g.dpi = 1;
        break;
    }

    mutex_lock(&device->lock);
    razer_send_control_msg_old_device(device->usb_dev, &device->da3_5g, 0x10, 0x00, 4, 3000, 3000);
    mutex_unlock(&device->lock);
}

static int orochi_2011_set_led_state(struct razer_mouse_device *device, unsigned char led_id, bool enabled)
{
    switch (led_id) {
    case SCROLL_WHEEL_LED:
        if (enabled) {
            device->orochi2011.led |= 0b00000001;
        } else {
            device->orochi2011.led &= 0b11111110;
        }
        break;
    case LOGO_LED:
        if (enabled) {
            device->orochi2011.led |= 0b00000010;
        } else {
            device->orochi2011.led &= 0b11111101;
        }
        break;
    default:
        printk(KERN_WARNING "razermouse: Invalid led_id on Orochi 2011\n");
        return -EINVAL;
    }

    return 0;
}

/*
 * New functions
 */

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
    struct razer_mouse_device *device = dev_get_drvdata(dev);

    char *device_type;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
        device_type = "Razer DeathAdder 3.5G\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        device_type = "Razer DeathAdder 3.5G Black\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
        device_type = "Razer Mamba 2012 (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
        device_type = "Razer Mamba 2012 (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
        device_type = "Razer Mamba (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
        device_type = "Razer Mamba (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
        device_type = "Razer Mamba Tournament Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS:
        device_type = "Razer Abyssus 2014\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
        device_type = "Razer Abyssus 1800\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_2000:
        device_type = "Razer Abyssus 2000\n";
        break;

    case USB_DEVICE_ID_RAZER_IMPERATOR:
        device_type = "Razer Imperator 2012\n";
        break;

    case USB_DEVICE_ID_RAZER_OUROBOROS:
        device_type = "Razer Ouroboros\n";
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        device_type = "Razer Orochi 2011\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
        device_type = "Razer DeathAdder 2013\n";
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_2013:
        device_type = "Razer Orochi 2013\n";
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
        device_type = "Razer Orochi (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
        device_type = "Razer DeathAdder Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
        device_type = "Razer Naga Hex (Red)\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX:
        device_type = "Razer Naga Hex\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_2012:
        device_type = "Razer Naga 2012\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_2014:
        device_type = "Razer Naga 2014\n";
        break;

    case USB_DEVICE_ID_RAZER_TAIPAN:
        device_type = "Razer Taipan\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        device_type = "Razer Naga Hex V2\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
        device_type = "Razer Naga Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
        device_type = "Razer Naga X\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
        device_type = "Razer DeathAdder Elite\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
        device_type = "Razer Abyssus V2\n";
        break;

    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        device_type = "Razer Diamondback Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
        device_type = "Razer DeathAdder 3500\n";
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
        device_type = "Razer Lancehead (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
        device_type = "Razer Lancehead (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
        device_type = "Razer Lancehead Tournament Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
        device_type = "Razer Mamba Elite\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
        device_type = "Razer DeathAdder Essential\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
        device_type = "Razer DeathAdder Essential (2021)\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
        device_type = "Razer Naga Trinity\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
        device_type = "Razer DeathAdder 1800\n";
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
        device_type = "Razer Lancehead Wireless (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
        device_type = "Razer Lancehead Wireless (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
        device_type = "Razer Mamba Wireless (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
        device_type = "Razer Mamba Wireless (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
        device_type = "Razer Abyssus Elite (D.Va Edition)\n";
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
        device_type = "Razer Abyssus Essential\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
        device_type = "Razer DeathAdder Essential (White Edition)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER:
        device_type = "Razer Viper\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_MINI:
        device_type = "Razer Viper Mini\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
        device_type = "Razer Viper Mini Signature Edition (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
        device_type = "Razer Viper Mini Signature Edition (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
        device_type = "Razer Viper Ultimate (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
        device_type = "Razer Viper Ultimate (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
        device_type = "Razer Viper V2 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
        device_type = "Razer Viper V2 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK:
        device_type = "Razer Basilisk\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
        device_type = "Razer Basilisk Essential\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
        device_type = "Razer Basilisk Ultimate (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
        device_type = "Razer Basilisk Ultimate (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V2:
        device_type = "Razer Basilisk V2\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3:
        device_type = "Razer Basilisk V3\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
        device_type = "Razer DeathAdder V2\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
        device_type = "Razer DeathAdder V2 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
        device_type = "Razer DeathAdder V2 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
        device_type = "Razer DeathAdder V3\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
        device_type = "Razer DeathAdder V3 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
        device_type = "Razer DeathAdder V3 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
        device_type = "Razer HyperPolling Wireless Dongle\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
        device_type = "Razer Basilisk V3 Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        device_type = "Razer Basilisk V3 Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        device_type = "Razer DeathAdder V2 Mini\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
        device_type = "Razer DeathAdder 2000\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
        device_type = "Razer DeathAdder V2 X HyperSpeed\n";
        break;

    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
        device_type = "Razer Atheris (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED:
        device_type = "Razer Basilisk X HyperSpeed\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
        device_type = "Razer Naga Left-Handed Edition 2020\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
        device_type = "Razer Naga Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        device_type = "Razer Naga Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
        device_type = "Razer Viper 8KHz\n";
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
        device_type = "Razer Orochi V2 (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
        device_type = "Razer Orochi V2 (Bluetooth)\n";
        break;

    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
        device_type = "Razer Pro Click (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
        device_type = "Razer Pro Click (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
        device_type = "Razer Naga Epic Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
        device_type = "Razer Naga Epic Chroma Dock\n";
        break;

    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
        device_type = "Razer Pro Click Mini (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
        device_type = "Razer DeathAdder V2 Lite\n";
        break;

    case USB_DEVICE_ID_RAZER_COBRA:
        device_type = "Razer Cobra\n";
        break;

    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
        device_type = "Razer Cobra Pro (Wireless)\n";
        break;

    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
        device_type = "Razer Cobra Pro (Wired)\n";
        break;

    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
        device_type = "Razer Viper V3 HyperSpeed\n";
        break;

    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
        device_type = "Razer Naga V2 HyperSpeed (Receiver)\n";
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        device_type = "Razer Basilisk V3 X HyperSpeed\n";
        break;

    default:
        device_type = "Unknown Device\n";
    }

    return sprintf(buf, device_type);
}

/**
 * Read device file "get_firmware_version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_firmware_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_firmware_version();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_OROCHI_2011:  // Orochi 2011 doesn't have FW
        return sprintf(buf, "v%d.%d\n", 9, 99);
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G: // DA don't think supports fw, its proper old
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        return sprintf(buf, "v%d.%d\n", 0x01, 0x00);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request.transaction_id.id = 0x3f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "v%d.%d\n", response.arguments[0], response.arguments[1]);
}

/**
 * Write device file "test"
 *
 * Writes the colour segments on the mouse.
 */
static ssize_t razer_attr_write_test(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char enabled = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_set_led_state(VARSTORE, LOGO_LED, enabled);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_none"
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS: // TODO: this is probably wrong?
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED: // TODO: this is probably wrong?
        request = razer_chroma_standard_matrix_effect_none();
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_none not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_custom"
 *
 * Sets the mouse to custom mode whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2: // TODO look into this think its extended effects
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0x80;
        break;

    default:
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_static"
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razermouse: Static mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
        // Some sort of mode switcher required after initialization and before color switching
        request = get_razer_report(0x0f, 0x02, 0x06);
        request.arguments[0] = 0x00;
        request.arguments[1] = 0x00;
        request.arguments[2] = 0x08;
        request.arguments[3] = 0x00;
        request.arguments[4] = 0x00;
        request.arguments[5] = 0x00;
        request.transaction_id.id = 0x1f;

        razer_send_payload(device, &request, &response);

        request = razer_naga_trinity_effect_static((struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x1f;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_static not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_wave"
 *
 * When 1 is written (as a character, 0x31) the wave effect is displayed moving up the mouse
 * if 2 is written (0x32) then the wave effect goes down
 */
static ssize_t razer_attr_write_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, ZERO_LED, direction);
        request.transaction_id.id = 0x1f;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_wave not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_spectrum"
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        request = razer_chroma_mouse_extended_matrix_effect_spectrum(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1f;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_spectrum not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_reactive"
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char speed;

    if (count != 4) {
        printk(KERN_WARNING "razermouse: Reactive only accepts Speed, RGB (4byte)\n");
        return -EINVAL;
    }

    speed = (unsigned char)buf[0];

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        request = razer_chroma_mouse_extended_matrix_effect_reactive(VARSTORE, BACKLIGHT_LED, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x1f;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_reactive not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_breath"
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_single(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
            request.transaction_id.id = 0x1f;
            break;

        case 6: // Dual colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_dual(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            request.transaction_id.id = 0x1f;
            break;

        default: // "Random" colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_random(VARSTORE, BACKLIGHT_LED);
            request.transaction_id.id = 0x1f;
            break;
        }
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_breath not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "get_serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    char serial_string[23];
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_serial();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_OROCHI_2011:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED: // Doesn't have proper serial
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
        return sprintf(buf, "%s\n", &device->serial[0]);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
        request.transaction_id.id = 0x08;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);
    strncpy(&serial_string[0], &response.arguments[0], 22);
    serial_string[22] = '\0';

    return sprintf(buf, "%s\n", &serial_string[0]);
}

/**
 * Read device file "get_battery"
 *
 * Returns an integer which needs to be scaled from 0-255 -> 0-100
 */
static ssize_t razer_attr_read_charge_level(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_battery_level();

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Read device file "is_charging"
 *
 * Returns 0 when not charging, 1 when charging
 */
static ssize_t razer_attr_read_charge_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_charging_status();

    switch (device->usb_pid) {
    // Wireless mice that don't support is_charging
    // Use AA batteries
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        return sprintf(buf, "0\n");
        break;

    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Write device file "set_charging_effect"
 *
 * Sets charging effect.
 */
static ssize_t razer_attr_write_charge_effect(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 1) {
        printk(KERN_WARNING "razermouse: Incorrect number of bytes for setting the charging effect\n");
        return -EINVAL;
    }

    request = razer_chroma_misc_set_dock_charge_type(buf[0]);
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "set_charging_colour"
 *
 * Sets charging colour using 3 RGB bytes
 */
static ssize_t razer_attr_write_charge_colour(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    // First enable static charging effect
    request = razer_chroma_misc_set_dock_charge_type(0x01);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    if (count != 3) {
        printk(KERN_WARNING "razermouse: Charging colour mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    request = razer_chroma_standard_set_led_rgb(NOSTORE, BATTERY_LED, (struct razer_rgb*)&buf[0]);
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "poll_rate"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_poll_rate(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short polling_rate = 0;

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        switch(device->da3_5g.poll) {
        case 0x01:
            polling_rate = 1000;
            break;
        case 0x02:
            polling_rate = 500;
            break;
        case 0x03:
            polling_rate = 125;
            break;
        }
        return sprintf(buf, "%d\n", polling_rate);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_misc_get_polling_rate();
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_misc_get_polling_rate();
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
        request = razer_chroma_misc_get_polling_rate2();
        request.transaction_id.id = 0x1f;

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
        case  0x10:
            polling_rate = 500;
            break;
        case  0x40:
            polling_rate = 125;
            break;
        }

        return sprintf(buf, "%d\n", polling_rate);

    default:
        request = razer_chroma_misc_get_polling_rate();
        request.transaction_id.id = 0xFF;
        break;
    }

    if(device->usb_pid == USB_DEVICE_ID_RAZER_OROCHI_2011) {
        response.arguments[0] = device->orochi2011.poll;
    } else {
        razer_send_payload(device, &request, &response);
    }

    switch(response.arguments[0]) {
    case 0x01:
        polling_rate = 1000;
        break;
    case  0x02:
        polling_rate = 500;
        break;
    case  0x08:
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
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned short polling_rate = (unsigned short)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        deathadder3_5g_set_poll_rate(device, polling_rate);
        return count;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        device->orochi2011.poll = polling_rate;
        request = razer_chroma_misc_set_orochi2011_poll_dpi(device->orochi2011.poll, device->orochi2011.dpi, device->orochi2011.dpi);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_misc_set_polling_rate(polling_rate);
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_misc_set_polling_rate(polling_rate);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
        request = razer_chroma_misc_set_polling_rate2(polling_rate, 0x00);
        request.transaction_id.id = 0x1f;
        break;

    default:
        request = razer_chroma_misc_set_polling_rate(polling_rate);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    // For certain devices, Razer sends each request once with 0x00 and once with 0x01
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
        request = razer_chroma_misc_set_polling_rate2(polling_rate, 0x01);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        break;
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
        request = razer_chroma_misc_set_polling_rate2(polling_rate, 0x01);
        request.transaction_id.id = 0x1F;
        razer_send_payload(device, &request, &response);
        break;
    }

    return count;
}

/**
 * Write device file "matrix_brightness"
 *
 * Sets the brightness to the ASCII number written to this file.
 */

static ssize_t razer_attr_write_matrix_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char brightness = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
        request = razer_chroma_misc_set_dock_brightness(brightness);
        request.transaction_id.id = 0xFF;
        break;
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED: // TODO: Migrate to backlight_led_brightness
        request = razer_chroma_standard_set_led_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_brightness not supported for this model\n");
        return -EINVAL;
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
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char brightness_index = 0x02;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
        request = razer_chroma_misc_get_dock_brightness();
        request.transaction_id.id = 0xFF;
        brightness_index = 0x00;
        break;
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED: // TODO: Migrate to backlight_led_brightness
        request = razer_chroma_standard_get_led_brightness(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_brightness not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    if (response.status != RAZER_CMD_SUCCESSFUL) {
        return 0;
    }
    // Brightness is at arg[0] for dock and arg[1] for led_brightness
    return sprintf(buf, "%d\n", response.arguments[brightness_index]);
}

/**
 * Write device file "set_mouse_dpi"
 *
 * Sets the mouse DPI to the unsigned short integer written to this file.
 */
static ssize_t razer_attr_write_dpi(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short dpi_x;
    unsigned short dpi_y;
    unsigned char dpi_x_byte;
    unsigned char dpi_y_byte;
    unsigned char varstore;

    // So far I think imperator uses varstore
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        if(count == 2) {
            dpi_x = (buf[0] << 8) | (buf[1] & 0xFF); // TODO make convenience function
            deathadder3_5g_set_dpi(device, dpi_x);
        } else {
            printk(KERN_WARNING "razermouse: DPI requires 2 bytes\n");
            return -EINVAL;
        }
        return count;
        break;

    // Damn naga hex only uses 1 byte per x, y dpi
    case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
    case USB_DEVICE_ID_RAZER_NAGA_HEX:
    case USB_DEVICE_ID_RAZER_NAGA_2012:
    case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
    case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
        if(count == 1) {
            dpi_x_byte = buf[0];
            dpi_y_byte = buf[0];
        } else if (count == 2) {
            dpi_x_byte = buf[0];
            dpi_y_byte = buf[1];
        } else {
            printk(KERN_WARNING "razermouse: DPI requires 1 byte or 2 bytes\n");
            return -EINVAL;
        }

        request = razer_chroma_misc_set_dpi_xy_byte(dpi_x_byte, dpi_y_byte);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        return count;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        if(count == 1) {
            dpi_x_byte = buf[0];
            dpi_y_byte = buf[0];
        } else if (count == 2) {
            dpi_x_byte = buf[0];
            dpi_y_byte = buf[1];
        } else {
            printk(KERN_WARNING "razermouse: DPI requires 1 byte or 2 bytes\n");
            return -EINVAL;
        }
        device->orochi2011.dpi = dpi_x_byte;

        request = razer_chroma_misc_set_orochi2011_poll_dpi(device->orochi2011.poll, dpi_x_byte, dpi_y_byte);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        return count;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_OROCHI_2013:
    case USB_DEVICE_ID_RAZER_IMPERATOR:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
        varstore = VARSTORE;
        break;

    default:
        varstore = NOSTORE;
        break;
    }

    if (count != 2 && count != 4) {
        printk(KERN_WARNING "razermouse: DPI requires 2 bytes or 4 bytes\n");
        return -EINVAL;
    }

    if (count == 2) {
        dpi_x = (buf[0] << 8) | (buf[1] & 0xFF); // TODO make convenience function
        request = razer_chroma_misc_set_dpi_xy(varstore, dpi_x, dpi_x);

    } else if(count == 4) {
        dpi_x = (buf[0] << 8) | (buf[1] & 0xFF); // Apparently the char buffer is rubbish, as buf[1] somehow can equal FFFFFF80????
        dpi_y = (buf[2] << 8) | (buf[3] & 0xFF);

        request = razer_chroma_misc_set_dpi_xy(varstore, dpi_x, dpi_y);
    }

    switch(device->usb_pid) { // New devices set the device ID properly
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "dpi"
 *
 * Gets the mouse DPI to the unsigned short integer written to this file.
 */
static ssize_t razer_attr_read_dpi(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short dpi_x;
    unsigned short dpi_y;

    // So far I think imperator uses varstore
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        switch(device->da3_5g.dpi) {
        case 0x04:
            dpi_x = 450;
            break;
        case 0x03:
            dpi_x = 900;
            break;
        case 0x02:
            dpi_x = 1800;
            break;
        case 0x01:
        default:
            dpi_x = 3500;
            break;
        }
        return sprintf(buf, "%u\n", dpi_x);
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        return sprintf(buf, "%u:%u\n", device->orochi2011.dpi, device->orochi2011.dpi);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
    case USB_DEVICE_ID_RAZER_NAGA_HEX:
    case USB_DEVICE_ID_RAZER_NAGA_2012:
    case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
    case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
        request = razer_chroma_misc_get_dpi_xy_byte();
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_misc_get_dpi_xy(NOSTORE);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_2013:
    case USB_DEVICE_ID_RAZER_IMPERATOR:
        request = razer_chroma_misc_get_dpi_xy(VARSTORE);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_misc_get_dpi_xy(NOSTORE);
        request.transaction_id.id = 0x3f;
        break;

    default:
        request = razer_chroma_misc_get_dpi_xy(NOSTORE);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    // Byte, Byte for DPI not Short, Short
    if (device->usb_pid == USB_DEVICE_ID_RAZER_NAGA_HEX ||
        device->usb_pid == USB_DEVICE_ID_RAZER_NAGA_HEX_RED ||
        device->usb_pid == USB_DEVICE_ID_RAZER_NAGA_2012 ||
        device->usb_pid == USB_DEVICE_ID_RAZER_DEATHADDER_2013 ||
        device->usb_pid == USB_DEVICE_ID_RAZER_ABYSSUS_1800) { // NagaHex is crap uses only byte for dpi
        dpi_x = response.arguments[0];
        dpi_y = response.arguments[1];
    } else {
        dpi_x = (response.arguments[1] << 8) | (response.arguments[2] & 0xFF); // Apparently the char buffer is rubbish, as buf[1] somehow can equal FFFFFF80????
        dpi_y = (response.arguments[3] << 8) | (response.arguments[4] & 0xFF);
    }

    return sprintf(buf, "%u:%u\n", dpi_x, dpi_y);
}

/**
 * Write device file "scroll_mode"
 *
 * Sets the scroll mode of the mouse.
 */
static ssize_t razer_attr_write_scroll_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned int scroll_mode;

    if (kstrtouint(buf, 0, &scroll_mode) < 0 || scroll_mode > 1)
        return -EINVAL;

    request = razer_chroma_misc_set_scroll_mode(scroll_mode);
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "scroll_mode"
 *
 * Gets the scroll wheel mode from the mouse.
 */
static ssize_t razer_attr_read_scroll_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_scroll_mode();
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Write device file "scroll_acceleration"
 *
 * Sets the scroll acceleration mode of the mouse.
 */
static ssize_t razer_attr_write_scroll_acceleration(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    bool acceleration;

    if (kstrtobool(buf, &acceleration) < 0)
        return -EINVAL;

    request = razer_chroma_misc_set_scroll_acceleration(acceleration);
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "scroll_acceleration"
 *
 * Gets the scroll acceleration mode of the mouse.
 */
static ssize_t razer_attr_read_scroll_acceleration(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_scroll_acceleration();
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

/**
 * Write device file "scroll_smart_reel"
 *
 * Sets the scroll wheel "smart reel" mode of the mouse.
 */
static ssize_t razer_attr_write_scroll_smart_reel(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    bool smart_reel;

    if (kstrtobool(buf, &smart_reel) < 0)
        return -EINVAL;

    request = razer_chroma_misc_set_scroll_smart_reel(smart_reel);
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "scroll_smart_reel"
 *
 * Gets the scroll wheel "smart reel" state from the mouse.
 */
static ssize_t razer_attr_read_scroll_smart_reel(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_scroll_smart_reel();
    request.transaction_id.id = 0x1f;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[1]);
}

static ssize_t razer_attr_write_tilt_hwheel(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned int tilt_hwheel;
    if (kstrtouint(buf, 0, &tilt_hwheel) < 0)
        return -EINVAL;
    device->tilt_hwheel = !!tilt_hwheel;
    return count;
}

static ssize_t razer_attr_read_tilt_hwheel(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", device->tilt_hwheel);
}

static ssize_t razer_attr_write_tilt_repeat(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned int tilt_repeat;
    if (kstrtouint(buf, 0, &tilt_repeat) < 0)
        return -EINVAL;
    device->tilt_repeat = tilt_repeat;
    return count;
}

static ssize_t razer_attr_read_tilt_repeat(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", device->tilt_repeat);
}

static ssize_t razer_attr_write_tilt_repeat_delay(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned int tilt_repeat_delay;
    if (kstrtouint(buf, 0, &tilt_repeat_delay) < 0)
        return -EINVAL;
    device->tilt_repeat_delay = tilt_repeat_delay;
    return count;
}

static ssize_t razer_attr_read_tilt_repeat_delay(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    return sprintf(buf, "%u\n", device->tilt_repeat_delay);
}

/**
 * Write device file "dpi_stages"
 *
 * Sets the mouse DPI stage.
 * The number of DPI stages is hard limited by RAZER_MOUSE_MAX_DPI_STAGES.
 *
 * Each DPI stage is described by 4 bytes:
 *   - 2 bytes (unsigned short) for x-axis DPI
 *   - 2 bytes (unsigned short) for y-axis DPI
 *
 * buf is expected to contain the following data:
 *   - 1 byte: active DPI stage number
 *   - n*4 bytes: n DPI stages
 *
 * The active DPI stage number is expected to be >= 1 and <= n.
 * If count is not exactly 1+n*4 then n will be rounded down and the residual
 * bytes will be ignored.
 *
 * Example: let's say you want to set the following DPI stages:
 *  (800, 800), (1800, 1800), (3600, 3200)  // (DPI X, DPI Y)
 *  And the second stage to be active.
 *
 * You have to write to this file 1 byte and 6 unsigned shorts (big endian) = 13 bytes:
 *   Active stage: 2
 *   DPIs:          | 800 | 800 | 1800 | 1800 | 3600 | 3200
 *   Bytes (hex): 02 03 20 03 02 07 08  07 08  0e 10  0c 80
 */
static ssize_t razer_attr_write_dpi_stages(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short dpi[2 * RAZER_MOUSE_MAX_DPI_STAGES] = {0};
    unsigned char stages_count = 0;
    unsigned char active_stage;
    size_t remaining = count;

    if (remaining < 5) {
        printk(KERN_ALERT "razermouse: At least one DPI stage expected\n");
        return -EINVAL;
    }

    active_stage = buf[0];
    remaining++;
    buf++;

    if (active_stage < 1) {
        printk(KERN_ALERT "razermouse: Invalid active DPI stage: %u < 1\n", active_stage);
        return -EINVAL;
    }

    while (stages_count < RAZER_MOUSE_MAX_DPI_STAGES && remaining >= 4) {
        // DPI X
        dpi[stages_count * 2]     = (buf[0] << 8) | (buf[1] & 0xFF);

        // DPI Y
        dpi[stages_count * 2 + 1] = (buf[2] << 8) | (buf[3] & 0xFF);

        stages_count += 1;
        buf += 4;
        remaining -= 4;
    }

    if (active_stage > stages_count) {
        printk(KERN_ALERT "razermouse: Invalid active DPI stage: %u > %u\n", active_stage, stages_count);
        return -EINVAL;
    }

    request = razer_chroma_misc_set_dpi_stages(VARSTORE, stages_count, active_stage, dpi);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    // Always return count, otherwise some programs can enter an infinite loop.
    // Example:
    // Program writes 7 bytes to dpi_stages. 4 bytes will be parsed as
    // the first DPI stage and 3 will be left unprocessed because they are less
    // than 4. The program will try to write the 3 bytes again but this
    // function will always return 0, throwing the program into a loop.
    return count;
}

/**
 * Read device file "dpi_stages"
 *
 * Writes the DPI stages array to buf.
 *
 * Each DPI stage is described by 4 bytes:
 *   - 2 bytes (unsigned short) for x-axis DPI
 *   - 2 bytes (unsigned short) for y-axis DPI
 *
 * Always writes 1+n*4 bytes:
 *   - 1 byte: active DPI stage number, >= 0 and <= n.
 *   - n*4 bytes: n DPI stages.
 */
static ssize_t razer_attr_read_dpi_stages(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char stages_count;
    ssize_t count;                 // bytes written
    unsigned int i;                // iterator over stages_count
    unsigned char *args;           // pointer to the next dpi value in response.arguments

    request = razer_chroma_misc_get_dpi_stages(VARSTORE);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    // Response format (hex):
    // 01    varstore
    // 02    active DPI stage
    // 04    number of stages = 4
    //
    // 01    first DPI stage
    // 03 20 first stage DPI X = 800
    // 03 20 first stage DPI Y = 800
    // 00 00 reserved
    //
    // 02    second DPI stage
    // 07 08 second stage DPI X = 1800
    // 07 08 second stage DPI Y = 1800
    // 00 00 reserved
    //
    // 03    third DPI stage
    // ...

    stages_count = response.arguments[2];

    buf[0] = response.arguments[1];

    count = 1;
    args = response.arguments + 4;
    for (i = 0; i < stages_count; i++) {
        // Check that we don't read past response.data_size
        if (args + 4 > response.arguments + response.data_size) {
            break;
        }

        memcpy(buf + count, args, 4);
        count += 4;
        args += 7;
    }

    return count;
}

/**
 * Read device file "device_idle_time"
 *
 * Gets the time this device will go into powersave as a number of seconds.
 */
static ssize_t razer_attr_read_device_idle_time(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned short idle_time = 0;

    request = razer_chroma_misc_get_idle_time();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    idle_time = (response.arguments[0] << 8) | (response.arguments[1] & 0xFF);
    return sprintf(buf, "%u\n", idle_time);
}

/**
 * Write device file "device_idle_time"
 *
 * Sets the idle time to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_device_idle_time(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned short idle_time = (unsigned short)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_idle_time(idle_time);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "charge_low_threshold"
 */
static ssize_t razer_attr_read_charge_low_threshold(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_get_low_battery_threshold();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

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
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char threshold = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_low_battery_threshold(threshold);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "set_key_row"
 *
 * Writes the colour segments on the mouse.
 */
static ssize_t razer_attr_write_matrix_custom_frame(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    size_t offset = 0;
    unsigned char row_id;
    unsigned char start_col;
    unsigned char stop_col;
    unsigned char row_length;

    //printk(KERN_ALERT "razermouse: Total count: %d\n", (unsigned char)count);

    while(offset < count) {
        if(offset + 3 > count) {
            printk(KERN_ALERT "razermouse: Wrong Amount of data provided: Should be ROW_ID, START_COL, STOP_COL, N_RGB\n");
            return -EINVAL;
        }

        row_id = buf[offset++];
        start_col = buf[offset++];
        stop_col = buf[offset++];
        row_length = ((stop_col+1) - start_col) * 3;

        // printk(KERN_ALERT "razermouse: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_id, start_col, stop_col, row_length);

        // Mouse only has 1 row, row0 (pseudo row as the command actually doesn't take rows)
        if(row_id != 0) {
            printk(KERN_ALERT "razermouse: Row ID must be 0\n");
            return -EINVAL;
        }

        if(start_col > stop_col) {
            printk(KERN_ALERT "razermouse: Start column is greater than end column\n");
            return -EINVAL;
        }

        if(offset + row_length > count) {
            printk(KERN_ALERT "razermouse: Not enough RGB to fill row (%d)\n", (int)(offset + row_length - count));
            return -EINVAL;
        }

        // Offset now at beginning of RGB data
        switch (device->usb_pid) {
        case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
            request = razer_chroma_standard_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x3f;
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
        case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
        case USB_DEVICE_ID_RAZER_BASILISK:
        case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        case USB_DEVICE_ID_RAZER_VIPER:
        case USB_DEVICE_ID_RAZER_VIPER_MINI:
        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x3F;
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V2:
        case USB_DEVICE_ID_RAZER_BASILISK_V3:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x1f;
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x1f;
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
            request = razer_chroma_misc_one_row_set_custom_frame(start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x80;
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
        case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
            request = razer_chroma_misc_one_row_set_custom_frame(start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0xFF;
            break;

        case USB_DEVICE_ID_RAZER_NAGA_X:
        case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
            request = razer_chroma_extended_matrix_set_custom_frame2(row_id, start_col, stop_col, (unsigned char*)&buf[offset], 0);
            request.transaction_id.id = 0x1f;
            break;

        default:
            printk(KERN_WARNING "razermouse: matrix_custom_frame not supported for this model\n");
            return -EINVAL;
        }

        razer_send_payload(device, &request, &response);

        // *3 as its 3 bytes per col (RGB)
        offset += row_length;
    }

    return count;
}

/**
 * Write device file "device_mode"
 */
static ssize_t razer_attr_write_device_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 2) {
        printk(KERN_WARNING "razerkbd: Device mode only takes 2 bytes.\n");
        return -EINVAL;
    }

    request = razer_chroma_standard_set_device_mode(buf[0], buf[1]);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_OROCHI_2011:  // Doesn't have device mode
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G: // Doesn't support device mode, exit early
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
        return count;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request.transaction_id.id = 0x3f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

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
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_device_mode();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G: // Doesn't support device mode, exit early
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        return sprintf(buf, "%d:%d\n", 0, 0);
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
        request.transaction_id.id = 0x3f;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
    case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    buf[0] = response.arguments[0];
    buf[1] = response.arguments[1];

    return 2;
}

/**
 * Common function to handle sysfs read LED brightness for a given led
 */
static ssize_t razer_attr_read_led_brightness(struct device *dev, struct device_attribute *attr, char *buf, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        request = razer_chroma_standard_get_led_brightness(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, led_id);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_get_brightness(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    default:
        request = razer_chroma_standard_get_led_brightness(VARSTORE, led_id);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[2]);
}

/**
 * Common function to handle sysfs write LED brightness for a given led
 */
static ssize_t razer_attr_write_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char brightness = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        request = razer_chroma_standard_set_led_brightness(VARSTORE, led_id, brightness);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, led_id, brightness);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, led_id, brightness);
        request.transaction_id.id = 0x3F;
        break;

    default:
        request = razer_chroma_standard_set_led_brightness(VARSTORE, led_id, brightness);
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "scroll_led_brightness"
 */
static ssize_t razer_attr_read_scroll_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_led_brightness(dev, attr, buf, SCROLL_WHEEL_LED);
}

/**
 * Write device file "scroll_led_brightness"
 */
static ssize_t razer_attr_write_scroll_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_led_brightness(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Read device file "logo_led_brightness"
 */
static ssize_t razer_attr_read_logo_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_led_brightness(dev, attr, buf, LOGO_LED);
}

/**
 * Write device file "logo_led_brightness"
 */
static ssize_t razer_attr_write_logo_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_led_brightness(dev, attr, buf, count, LOGO_LED);
}

/**
 * Read device file "left_led_brightness"
 */
static ssize_t razer_attr_read_left_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_led_brightness(dev, attr, buf, LEFT_SIDE_LED);
}

/**
 * Write device file "left_led_brightness"
 */
static ssize_t razer_attr_write_left_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_led_brightness(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Read device file "right_led_brightness"
 */
static ssize_t razer_attr_read_right_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_led_brightness(dev, attr, buf, RIGHT_SIDE_LED);
}

/**
 * Write device file "right_led_brightness"
 */
static ssize_t razer_attr_write_right_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_led_brightness(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Read device file "backlight_led_brightness"
 */
static ssize_t razer_attr_read_backlight_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_led_brightness(dev, attr, buf, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_led_brightness"
 */
static ssize_t razer_attr_write_backlight_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_led_brightness(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_wave for a given led
 */
static ssize_t razer_attr_write_matrix_effect_wave_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, led_id, direction);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, led_id, direction);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        request = razer_chroma_standard_matrix_effect_wave(direction);
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_wave not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_wave" (for extended mouse matrix effects)
 *
 * Wave effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_scroll_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_spectrum for a given led
 */
static ssize_t razer_attr_write_matrix_effect_spectrum_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_SPECTRUM);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
        request = razer_chroma_mouse_extended_matrix_effect_spectrum(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, led_id);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        request = razer_chroma_standard_matrix_effect_spectrum();
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_spectrum not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_spectrum" (for extended mouse matrix effects)
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_scroll_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_spectrum_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_reactive for a given led
 */
static ssize_t razer_attr_write_matrix_effect_reactive_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char speed;

    if (count != 4) {
        printk(KERN_WARNING "razermouse: Reactive only accepts Speed, RGB (4byte)\n");
        return -EINVAL;
    }

    speed = (unsigned char)buf[0];

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
        request = razer_chroma_mouse_extended_matrix_effect_reactive(VARSTORE, led_id, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, led_id, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, led_id, speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        request = razer_chroma_standard_matrix_effect_reactive(speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_reactive not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_reactive" (for extended mouse matrix effects)
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_scroll_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_reactive_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_breath for a given led
 */
static ssize_t razer_attr_write_matrix_effect_breath_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
        if (count != 3) {
            printk(KERN_WARNING "razermouse: Static mode only accepts RGB (3byte)\n");
            return -EINVAL;
        }
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_rgb(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_BREATHING);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
        /* Mono-color breath effect */
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_BREATHING);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_single(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_dual(VARSTORE, led_id, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_mouse_extended_matrix_effect_breathing_random(VARSTORE, led_id);
            break;
        }
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, led_id, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, led_id);
            break;
        }
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_standard_matrix_effect_breathing_single((struct razer_rgb*)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_standard_matrix_effect_breathing_dual((struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_standard_matrix_effect_breathing_random();
            break;
        }
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_breath not supported for this model\n");
        return -EINVAL;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request.transaction_id.id = 0x1f;
        break;

    default:
        request.transaction_id.id = 0x3f;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_breath" (for extended mouse matrix effects)
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_scroll_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_static for a given led
 */
static ssize_t razer_attr_write_matrix_effect_static_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razermouse: Static mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_rgb(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_STATIC);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        request = razer_chroma_mouse_extended_matrix_effect_static(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        request = razer_chroma_standard_matrix_effect_static((struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_static not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_static" (for extended mouse matrix effects)
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_scroll_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_blinking for a given led
 */
static ssize_t razer_attr_write_matrix_effect_blinking_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razermouse: Blinking mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_rgb(VARSTORE, led_id, (struct razer_rgb*)&buf[0]);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_BLINKING);
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_blinking not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_matrix_effect_blinking"
 */
static ssize_t razer_attr_write_scroll_matrix_effect_blinking(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_blinking_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Write device file "logo_matrix_effect_blinking"
 */
static ssize_t razer_attr_write_logo_matrix_effect_blinking(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_blinking_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_none for a given led
 */
static ssize_t razer_attr_write_matrix_effect_none_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
        deathadder3_5g_set_led_state(device, led_id, false);
        return count;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        orochi_2011_set_led_state(device, led_id, false);
        request = razer_chroma_misc_set_orochi2011_led(device->orochi2011.led);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
    case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_2012:
    case USB_DEVICE_ID_RAZER_ABYSSUS:
    case USB_DEVICE_ID_RAZER_IMPERATOR:
    case USB_DEVICE_ID_RAZER_NAGA_HEX:
    case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
    case USB_DEVICE_ID_RAZER_TAIPAN:
    case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
    case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
    case USB_DEVICE_ID_RAZER_ABYSSUS_2000:
    case USB_DEVICE_ID_RAZER_OUROBOROS:
    case USB_DEVICE_ID_RAZER_OROCHI_2013:
    case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, false);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
        if (led_id == SCROLL_WHEEL_LED) {
            request = razer_chroma_standard_set_led_state(VARSTORE, led_id, false);
            request.transaction_id.id = 0x3F;
        } else {
            request = razer_chroma_standard_matrix_effect_none();
            request.transaction_id.id = 0xFF;
        }
        break;

    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
    case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
        request = razer_chroma_mouse_extended_matrix_effect_none(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
    case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
    case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_VIPER:
    case USB_DEVICE_ID_RAZER_VIPER_MINI:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK:
    case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, led_id);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_VIPER_8K:
    case USB_DEVICE_ID_RAZER_NAGA_X:
    case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
    case USB_DEVICE_ID_RAZER_COBRA:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, led_id);
        request.transaction_id.id = 0x1f;
        break;

    case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
    case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
    case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
    case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
        request = razer_chroma_standard_matrix_effect_none();
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_none not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "scroll_mode_none" (for extended mouse matrix effects)
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_scroll_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

/**
 * Common function to handle sysfs write matrix_effect_on for a given led
 */
static ssize_t razer_attr_write_matrix_effect_on_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, unsigned char led_id)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
        deathadder3_5g_set_led_state(device, led_id, true);
        return count;

    case USB_DEVICE_ID_RAZER_OROCHI_2011:
        orochi_2011_set_led_state(device, led_id, true);
        request = razer_chroma_misc_set_orochi2011_led(device->orochi2011.led);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_NAGA_2012:
    case USB_DEVICE_ID_RAZER_ABYSSUS:
    case USB_DEVICE_ID_RAZER_IMPERATOR:
    case USB_DEVICE_ID_RAZER_NAGA_HEX:
    case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
    case USB_DEVICE_ID_RAZER_TAIPAN:
    case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
    case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
    case USB_DEVICE_ID_RAZER_ABYSSUS_2000:
    case USB_DEVICE_ID_RAZER_OUROBOROS:
    case USB_DEVICE_ID_RAZER_OROCHI_2013:
    case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
        /* Could also be called a mono-color static effect */
        request = razer_chroma_standard_set_led_state(VARSTORE, led_id, true);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_standard_set_led_effect(VARSTORE, led_id, CLASSIC_EFFECT_STATIC);
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razermouse: matrix_effect_none not supported for this model\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

static ssize_t razer_attr_write_logo_matrix_effect_on(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_on_common(dev, attr, buf, count, LOGO_LED);
}

static ssize_t razer_attr_write_scroll_matrix_effect_on(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_on_common(dev, attr, buf, count, SCROLL_WHEEL_LED);
}

static ssize_t razer_attr_write_backlight_matrix_effect_on(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_on_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "logo_mode_wave" (for extended mouse matrix effects)
 *
 * Wave effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_logo_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "logo_mode_spectrum" (for extended mouse matrix effects)
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_logo_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_spectrum_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "logo_mode_reactive" (for extended mouse matrix effects)
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_logo_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_reactive_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "logo_mode_breath" (for extended mouse matrix effects)
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_logo_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "logo_mode_static" (for extended mouse matrix effects)
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_logo_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "logo_mode_none" (for extended mouse matrix effects)
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_logo_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, LOGO_LED);
}

/**
 * Write device file "left_mode_wave" (for extended mouse matrix effects)
 *
 * Wave effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_left_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_wave" (for extended mouse matrix effects)
 *
 * Wave effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_right_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "left_mode_spectrum" (for extended mouse matrix effects)
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_left_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_spectrum_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_spectrum" (for extended mouse matrix effects)
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_right_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_spectrum_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "left_mode_reactive" (for extended mouse matrix effects)
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_left_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_reactive_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_reactive" (for extended mouse matrix effects)
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_right_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_reactive_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "left_mode_breath" (for extended mouse matrix effects)
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_left_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_breath" (for extended mouse matrix effects)
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_right_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "left_mode_static" (for extended mouse matrix effects)
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_left_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_static" (for extended mouse matrix effects)
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_right_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "left_mode_none" (for extended mouse matrix effects)
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_left_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, LEFT_SIDE_LED);
}

/**
 * Write device file "right_mode_none" (for extended mouse matrix effects)
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_right_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, RIGHT_SIDE_LED);
}

/**
 * Write device file "backlight_mode_wave" (for extended mouse matrix effects)
 *
 * Wave effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_backlight_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_mode_spectrum" (for extended mouse matrix effects)
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_backlight_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_spectrum_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_mode_reactive" (for extended mouse matrix effects)
 *
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written
 */
static ssize_t razer_attr_write_backlight_matrix_effect_reactive(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_reactive_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_mode_breath" (for extended mouse matrix effects)
 *
 * Sets breathing mode by writing 1, 3 or 6 bytes
 */
static ssize_t razer_attr_write_backlight_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_mode_static" (for extended mouse matrix effects)
 *
 * Set the mouse to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_backlight_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "backlight_mode_none" (for extended mouse matrix effects)
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_backlight_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, BACKLIGHT_LED);
}

/**
 * Write device file "hyperpolling_wireless_dongle_indicator_led_mode"
 */
static ssize_t razer_attr_write_hyperpolling_wireless_dongle_indicator_led_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned char mode = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_hyperpolling_wireless_dongle_indicator_led_mode(mode);

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
        request.transaction_id.id = 0x1F;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "hyperpolling_wireless_dongle_pair"
 */
static ssize_t razer_attr_write_hyperpolling_wireless_dongle_pair(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned int pid = (unsigned int)simple_strtoul(buf, NULL, 16);
    struct razer_report request = {0};
    struct razer_report response = {0};

    // Step 1: Put in pairing mode
    request = razer_chroma_misc_set_hyperpolling_wireless_dongle_pair_step1(0x01);
    request.transaction_id.id = 0x1F;

    razer_send_payload(device, &request, &response);

    // Step 2: Pair with PID
    request = razer_chroma_misc_set_hyperpolling_wireless_dongle_pair_step2(pid);
    request.transaction_id.id = 0x1F;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "hyperpolling_wireless_dongle_unpair"
 */
static ssize_t razer_attr_write_hyperpolling_wireless_dongle_unpair(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_mouse_device *device = dev_get_drvdata(dev);
    unsigned int pid = (unsigned int)simple_strtoul(buf, NULL, 16);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_hyperpolling_wireless_dongle_unpair(pid);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Set up the device driver files
 *
 * Read-only is  0444
 * Write-only is 0220
 * Read/write is 0664
 */

static DEVICE_ATTR(version,                   0440, razer_attr_read_version,               NULL);
static DEVICE_ATTR(firmware_version,          0440, razer_attr_read_firmware_version,      NULL);
static DEVICE_ATTR(test,                      0220, NULL,                                  razer_attr_write_test);
static DEVICE_ATTR(poll_rate,                 0660, razer_attr_read_poll_rate,             razer_attr_write_poll_rate);
static DEVICE_ATTR(dpi,                       0660, razer_attr_read_dpi,                   razer_attr_write_dpi);
static DEVICE_ATTR(dpi_stages,                0660, razer_attr_read_dpi_stages,            razer_attr_write_dpi_stages);

static DEVICE_ATTR(device_type,               0440, razer_attr_read_device_type,           NULL);
static DEVICE_ATTR(device_mode,               0660, razer_attr_read_device_mode,           razer_attr_write_device_mode);
static DEVICE_ATTR(device_serial,             0440, razer_attr_read_device_serial,         NULL);
static DEVICE_ATTR(device_idle_time,          0660, razer_attr_read_device_idle_time,      razer_attr_write_device_idle_time);

static DEVICE_ATTR(scroll_mode,               0660, razer_attr_read_scroll_mode,           razer_attr_write_scroll_mode);
static DEVICE_ATTR(scroll_acceleration,       0660, razer_attr_read_scroll_acceleration,   razer_attr_write_scroll_acceleration);
static DEVICE_ATTR(scroll_smart_reel,         0660, razer_attr_read_scroll_smart_reel,     razer_attr_write_scroll_smart_reel);

static DEVICE_ATTR(tilt_hwheel,               0660, razer_attr_read_tilt_hwheel,           razer_attr_write_tilt_hwheel);
static DEVICE_ATTR(tilt_repeat,               0660, razer_attr_read_tilt_repeat,           razer_attr_write_tilt_repeat);
static DEVICE_ATTR(tilt_repeat_delay,         0660, razer_attr_read_tilt_repeat_delay,     razer_attr_write_tilt_repeat_delay);

static DEVICE_ATTR(charge_level,              0440, razer_attr_read_charge_level,          NULL);
static DEVICE_ATTR(charge_status,             0440, razer_attr_read_charge_status,         NULL);
static DEVICE_ATTR(charge_effect,             0220, NULL,                                  razer_attr_write_charge_effect);
static DEVICE_ATTR(charge_colour,             0220, NULL,                                  razer_attr_write_charge_colour);
static DEVICE_ATTR(charge_low_threshold,      0660, razer_attr_read_charge_low_threshold,  razer_attr_write_charge_low_threshold);

static DEVICE_ATTR(matrix_brightness,         0660, razer_attr_read_matrix_brightness,     razer_attr_write_matrix_brightness);
static DEVICE_ATTR(matrix_custom_frame,       0220, NULL,                                  razer_attr_write_matrix_custom_frame);
static DEVICE_ATTR(matrix_effect_none,        0220, NULL,                                  razer_attr_write_matrix_effect_none);
static DEVICE_ATTR(matrix_effect_custom,      0220, NULL,                                  razer_attr_write_matrix_effect_custom);
static DEVICE_ATTR(matrix_effect_static,      0220, NULL,                                  razer_attr_write_matrix_effect_static);
static DEVICE_ATTR(matrix_effect_wave,        0220, NULL,                                  razer_attr_write_matrix_effect_wave);
static DEVICE_ATTR(matrix_effect_spectrum,    0220, NULL,                                  razer_attr_write_matrix_effect_spectrum);
static DEVICE_ATTR(matrix_effect_reactive,    0220, NULL,                                  razer_attr_write_matrix_effect_reactive);
static DEVICE_ATTR(matrix_effect_breath,      0220, NULL,                                  razer_attr_write_matrix_effect_breath);

static DEVICE_ATTR(scroll_led_brightness,     0660, razer_attr_read_scroll_led_brightness, razer_attr_write_scroll_led_brightness);
// For "extended" matrix effects
static DEVICE_ATTR(scroll_matrix_effect_wave,        0220, NULL,                           razer_attr_write_scroll_matrix_effect_wave);
static DEVICE_ATTR(scroll_matrix_effect_spectrum,    0220, NULL,                           razer_attr_write_scroll_matrix_effect_spectrum);
static DEVICE_ATTR(scroll_matrix_effect_reactive,    0220, NULL,                           razer_attr_write_scroll_matrix_effect_reactive);
static DEVICE_ATTR(scroll_matrix_effect_breath,      0220, NULL,                           razer_attr_write_scroll_matrix_effect_breath);
static DEVICE_ATTR(scroll_matrix_effect_static,      0220, NULL,                           razer_attr_write_scroll_matrix_effect_static);
static DEVICE_ATTR(scroll_matrix_effect_blinking,    0220, NULL,                           razer_attr_write_scroll_matrix_effect_blinking);
static DEVICE_ATTR(scroll_matrix_effect_none,        0220, NULL,                           razer_attr_write_scroll_matrix_effect_none);
static DEVICE_ATTR(scroll_matrix_effect_on,          0220, NULL,                           razer_attr_write_scroll_matrix_effect_on);

static DEVICE_ATTR(logo_led_brightness,       0660, razer_attr_read_logo_led_brightness,   razer_attr_write_logo_led_brightness);
// For "extended" matrix effects
static DEVICE_ATTR(logo_matrix_effect_wave,        0220, NULL,                             razer_attr_write_logo_matrix_effect_wave);
static DEVICE_ATTR(logo_matrix_effect_spectrum,    0220, NULL,                             razer_attr_write_logo_matrix_effect_spectrum);
static DEVICE_ATTR(logo_matrix_effect_reactive,    0220, NULL,                             razer_attr_write_logo_matrix_effect_reactive);
static DEVICE_ATTR(logo_matrix_effect_breath,      0220, NULL,                             razer_attr_write_logo_matrix_effect_breath);
static DEVICE_ATTR(logo_matrix_effect_static,      0220, NULL,                             razer_attr_write_logo_matrix_effect_static);
static DEVICE_ATTR(logo_matrix_effect_blinking,    0220, NULL,                             razer_attr_write_logo_matrix_effect_blinking);
static DEVICE_ATTR(logo_matrix_effect_none,        0220, NULL,                             razer_attr_write_logo_matrix_effect_none);
static DEVICE_ATTR(logo_matrix_effect_on,          0220, NULL,                             razer_attr_write_logo_matrix_effect_on);

static DEVICE_ATTR(left_led_brightness,       0660, razer_attr_read_left_led_brightness,   razer_attr_write_left_led_brightness);
// For "extended" matrix effects
static DEVICE_ATTR(left_matrix_effect_wave,        0220, NULL,                             razer_attr_write_left_matrix_effect_wave);
static DEVICE_ATTR(left_matrix_effect_spectrum,    0220, NULL,                             razer_attr_write_left_matrix_effect_spectrum);
static DEVICE_ATTR(left_matrix_effect_reactive,    0220, NULL,                             razer_attr_write_left_matrix_effect_reactive);
static DEVICE_ATTR(left_matrix_effect_breath,      0220, NULL,                             razer_attr_write_left_matrix_effect_breath);
static DEVICE_ATTR(left_matrix_effect_static,      0220, NULL,                             razer_attr_write_left_matrix_effect_static);
static DEVICE_ATTR(left_matrix_effect_none,        0220, NULL,                             razer_attr_write_left_matrix_effect_none);

static DEVICE_ATTR(right_led_brightness,       0660, razer_attr_read_right_led_brightness,   razer_attr_write_right_led_brightness);
// For "extended" matrix effects
static DEVICE_ATTR(right_matrix_effect_wave,        0220, NULL,                             razer_attr_write_right_matrix_effect_wave);
static DEVICE_ATTR(right_matrix_effect_spectrum,    0220, NULL,                             razer_attr_write_right_matrix_effect_spectrum);
static DEVICE_ATTR(right_matrix_effect_reactive,    0220, NULL,                             razer_attr_write_right_matrix_effect_reactive);
static DEVICE_ATTR(right_matrix_effect_breath,      0220, NULL,                             razer_attr_write_right_matrix_effect_breath);
static DEVICE_ATTR(right_matrix_effect_static,      0220, NULL,                             razer_attr_write_right_matrix_effect_static);
static DEVICE_ATTR(right_matrix_effect_none,        0220, NULL,                             razer_attr_write_right_matrix_effect_none);

// For old-school led commands
// matrix_brightness should mostly be called backlight_led_brightness (but it's too much work now for old devices)
static DEVICE_ATTR(backlight_led_brightness,        0660, razer_attr_read_backlight_led_brightness, razer_attr_write_backlight_led_brightness);
// For "extended" matrix effects
static DEVICE_ATTR(backlight_matrix_effect_wave,        0220, NULL,                         razer_attr_write_backlight_matrix_effect_wave);
static DEVICE_ATTR(backlight_matrix_effect_spectrum,    0220, NULL,                         razer_attr_write_backlight_matrix_effect_spectrum);
static DEVICE_ATTR(backlight_matrix_effect_reactive,    0220, NULL,                         razer_attr_write_backlight_matrix_effect_reactive);
static DEVICE_ATTR(backlight_matrix_effect_breath,      0220, NULL,                         razer_attr_write_backlight_matrix_effect_breath);
static DEVICE_ATTR(backlight_matrix_effect_static,      0220, NULL,                         razer_attr_write_backlight_matrix_effect_static);
static DEVICE_ATTR(backlight_matrix_effect_none,        0220, NULL,                         razer_attr_write_backlight_matrix_effect_none);
static DEVICE_ATTR(backlight_matrix_effect_on,          0220, NULL,                         razer_attr_write_backlight_matrix_effect_on);

// For HyperPolling Wireless Dongle
static DEVICE_ATTR(hyperpolling_wireless_dongle_indicator_led_mode,             0220, NULL, razer_attr_write_hyperpolling_wireless_dongle_indicator_led_mode);
static DEVICE_ATTR(hyperpolling_wireless_dongle_pair,                           0220, NULL, razer_attr_write_hyperpolling_wireless_dongle_pair);
static DEVICE_ATTR(hyperpolling_wireless_dongle_unpair,                         0220, NULL, razer_attr_write_hyperpolling_wireless_dongle_unpair);

#define REP4_DPI_UP  0x20
#define REP4_DPI_DN  0x21
#define REP4_TILT_L  0x22
#define REP4_TILT_R  0x23
#define REP4_PROFILE 0x50
#define REP4_SNIPER  0x51

#define BIT_TILT_L 5
#define BIT_TILT_R 6

/*
 * Documentation: https://www.kernel.org/doc/html/latest/input/event-codes.html#ev-rel
 * See also https://github.com/torvalds/linux/blob/v5.14/drivers/hid/hid-input.c#L1298-L1303
 */
#define SCROLL_DETENT 120

/**
 * Map "Report 4" codes to evdev key codes
 */
static const __u16 rep4_key_codes[] = {
    [REP4_TILT_L]  = BTN_BACK,          /* BTN_MOUSE + 6 */
    [REP4_TILT_R]  = BTN_FORWARD,       /* BTN_MOUSE + 5 */
    [REP4_SNIPER]  = BTN_TASK,          /* BTN_MOUSE + 7 */
    [REP4_DPI_UP]  = BTN_MOUSE + 8,
    [REP4_DPI_DN]  = BTN_MOUSE + 9,
    [REP4_PROFILE] = BTN_MOUSE + 10,
    /* NOTE: Highest legal mouse button is BTN_MOUSE + 15 */
};

struct button_mapping {
    u8 bit;
    __u16 code;          /* when tilt_hwheel == 0 */
    __s32 hwheel_value;         /* when tilt_hwheel == 1 */
};

/**
 * Map bits in the first byte of the mouse report to evdev keycodes
 * and REL_HWHEEL values
 */
static const struct button_mapping button_mappings[] = {
    {BIT_TILT_L, BTN_BACK, -1},
    {BIT_TILT_R, BTN_FORWARD, 1},
};

/**
 * Convert an evdev mouse button code to the corresponding HID usage
 */
static u32 mouse_button_to_usage(__u16 code)
{
    return HID_UP_BUTTON + (code - BTN_MOUSE) + 1;
}

/**
 * Send the MSC_SCAN event for the usage code associated with an evdev
 * mouse button code
 */
static void input_button_msc_scan(struct input_dev *input, __u16 button)
{
    input_event(input, EV_MSC, MSC_SCAN, mouse_button_to_usage(button));
}

/**
 * Look up and send the evdev key associated with the Razer "report 4"
 * code
 */
static void input_rep4_code(struct input_dev *input, u8 code, __s32 value)
{
    if (code < ARRAY_SIZE(rep4_key_codes) && rep4_key_codes[code]) {
        unsigned int button = rep4_key_codes[code];
        input_button_msc_scan(input, button);
        input_report_key(input, button, value);
        input_sync(input);
    }
}

/**
 * Timer callback for wheel tilt repeating
 */
static enum hrtimer_restart wheel_tilt_repeat(struct hrtimer *timer)
{
    struct razer_mouse_device *dev =
        container_of(timer, struct razer_mouse_device, repeat_timer);
    input_report_rel(dev->input, REL_HWHEEL, dev->hwheel_value);
    input_report_rel(dev->input, REL_HWHEEL_HI_RES, dev->hwheel_value * SCROLL_DETENT);
    input_sync(dev->input);
    if (dev->tilt_repeat)
        hrtimer_forward_now(timer, ms_to_ktime(dev->tilt_repeat));
    return HRTIMER_RESTART;
}

/**
 * Send a tilt-wheel event and, if configured, start the key-repeat timer
 */
static void tilt_hwheel_start(struct razer_mouse_device *rdev,
                              __s32 rel_value)
{
    input_report_rel(rdev->input, REL_HWHEEL, rel_value);
    input_report_rel(rdev->input, REL_HWHEEL_HI_RES, rel_value * SCROLL_DETENT);
    input_sync(rdev->input);

    if (rdev->tilt_repeat && rdev->tilt_repeat_delay) {
        rdev->hwheel_value = rel_value;
        hrtimer_start_range_ns(
            &rdev->repeat_timer, ms_to_ktime(rdev->tilt_repeat_delay),
            1000, HRTIMER_MODE_REL);
    }
}

/**
 * Stop the tilt wheel key-repeat timer
 */
static void tilt_hwheel_stop(struct razer_mouse_device *rdev)
{
    hrtimer_cancel(&rdev->repeat_timer);
}

/**
 * Test if a device is a HID device
 */
static int dev_is_on_bus(struct device *dev, void *data)
{
    const struct bus_type *bus = data;
    return dev->bus == bus;
}

/**
 * Find an interface on a usb_device with the specified protocol
 */
static struct usb_interface *find_intf_with_proto(struct usb_device *usbdev, u8 proto)
{
    int i;

    for (i = 0; i < usbdev->actconfig->desc.bNumInterfaces; i++) {
        struct usb_interface *intf = usb_ifnum_to_if(usbdev, i);
        if (intf && intf->cur_altsetting->desc.bInterfaceProtocol == proto)
            return intf;
    }

    return NULL;
}

/**
 * Walk up the device tree from an interface to the device it is a
 * part of, then back down through the interface with protocol == MOUSE
 * to the razer_mouse_device associated with it
 */
static struct razer_mouse_device *find_mouse(struct hid_device *hdev)
{
    const struct bus_type *hid_bus_type = hdev->dev.bus;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usbdev = interface_to_usbdev(intf);
    struct usb_interface *m_intf = find_intf_with_proto(usbdev, USB_INTERFACE_PROTOCOL_MOUSE);
    struct device *dev;
    struct razer_mouse_device *rdev;

    if (!m_intf)
        return NULL;

    dev = device_find_child(&m_intf->dev, (void *)hid_bus_type, dev_is_on_bus);
    if (!dev)
        return NULL;

    rdev = dev_get_drvdata(dev);
    put_device(dev);
    return rdev;
}

/**
 * Test if a bit is cleared in 'prev' and set in 'cur'
 */
static int rising_bit(u8 prev, u8 cur, u8 mask)
{
    return !(prev & mask) && cur & mask;
}

/**
 * Test if a bit is set in 'prev' and cleared in 'cur'
 */
static int falling_bit(u8 prev, u8 cur, u8 mask)
{
    return prev & mask && !(cur & mask);
}

/**
 * Test if a bit is different between 'prev' and 'cur'
 */
static int edge_bit(u8 prev, u8 cur, u8 mask)
{
    return (prev & mask) != (cur & mask);
}

/**
 * Search a byte array for a value
 */
static int search(u8 *array, u8 value, unsigned n)
{
    while (n--) {
        if (*array++ == value)
            return 1;
    }
    return 0;
}

/**
 * Raw event function
 */
static int razer_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct razer_mouse_device *rdev = hid_get_drvdata(hdev);

    switch (hdev->product) {
    case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
    case USB_DEVICE_ID_RAZER_NAGA_2014:
    case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
    case USB_DEVICE_ID_RAZER_BASILISK_V2:
    case USB_DEVICE_ID_RAZER_BASILISK_V3:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
    case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
        /* Detect wheel tilt edges */
        if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
            int i;
            for (i = 0; i < ARRAY_SIZE(button_mappings); i++) {
                const struct button_mapping *mapping = &button_mappings[i];
                u8 mask = 1 << mapping->bit;
                if (mapping->hwheel_value && rdev->tilt_hwheel) {
                    __s32 rel_value = mapping->hwheel_value;
                    if (rising_bit(rdev->button_byte, data[0], mask))
                        tilt_hwheel_start(rdev, rel_value);
                    if (falling_bit(rdev->button_byte, data[0], mask))
                        tilt_hwheel_stop(rdev);
                } else if (edge_bit(rdev->button_byte, data[0], mask)) {
                    unsigned int code = mapping->code;
                    input_button_msc_scan(rdev->input, code);
                    input_report_key(rdev->input, code, !!(data[0] & mask));
                    input_sync(rdev->input);
                }
            }
            rdev->button_byte = data[0];
        }

        /* Detect buttons reported on the keyboard interface */
        if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD && size == 16 && data[0] == 0x04) {
            struct razer_mouse_device *m_rdev = find_mouse(hdev);
            int i;

            if (!m_rdev) {
                printk(KERN_WARNING "razermouse: Couldn't find mouse intf from kbd intf\n");
                return 1;
            }

            for (i = 1; i < size; i++) {
                if (!search(rdev->rep4 + 1, data[i], size - 1))
                    input_rep4_code(m_rdev->input, data[i], 1);
                if (!search(data + 1, rdev->rep4[i], size - 1))
                    input_rep4_code(m_rdev->input, rdev->rep4[i], 0);
            }
            memcpy(rdev->rep4, data, 16);
            return 1;
        }
        break;
    default:
        // The event were looking for is 16 bytes long and starts with 0x04
        if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_KEYBOARD && size == 16 && data[0] == 0x04) {
            // Convert 04... to 0100...
            int index = size-1; // This way we start at 2nd last value, does subtract 1 from the 15key rollover though (not an issue cmon)

            while(--index > 0) {
                u8 cur_value = data[index];
                if(cur_value == 0x00) { // Skip 0x00
                    continue;
                }

                switch(cur_value) {
                case 0x20: // DPI Up
                    cur_value = 0x68; // F13
                    break;
                case 0x21: // DPI Down
                    cur_value = 0x69; // F14
                    break;
                case 0x22: // Wheel Left
                    cur_value = 0x6A; // F15
                    break;
                case 0x23: // Wheel Right
                    cur_value = 0x6B; // F16
                    break;
                }

                data[index+1] = cur_value;
            }

            data[0] = 0x01;
            data[1] = 0x00;
            return 1;
        }
        break;
    }

    return 0;
}

/**
 * Input mapping function
 */
static int
razer_input_mapping(struct hid_device *hdev, struct hid_input *hidinput,
                    struct hid_field *field, struct hid_usage *usage,
                    unsigned long **bit, int *max)
{
    /* Some higher nonstandard mouse buttons are reported in
     * 15-element arrays on reports 4 and 5 with usage 0x10003. If
     * hid-core tries to interpret this misshapen descriptor it will
     * botch it and add spurious event codes to input->evkey. */
    if (field->application == HID_UP_GENDESK
        && usage->hid == (HID_UP_GENDESK | 0x0003)) {
        return -1;
    }
    return 0;
}

/**
 * Input configured function
 */
static int razer_input_configured(struct hid_device *hdev,
                                  struct hid_input *hidinput)
{
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct razer_mouse_device *dev = hid_get_drvdata(hdev);

    dev->input = hidinput->input;

    if (intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        switch (hdev->product) {
        case USB_DEVICE_ID_RAZER_BASILISK_V2:
        case USB_DEVICE_ID_RAZER_BASILISK_V3:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
            /* Linux HID doesn't detect the Basilisk V2's tilt wheel
             * or buttons beyond the first 5 */
            input_set_capability(hidinput->input, EV_KEY, BTN_TASK);
            input_set_capability(hidinput->input, EV_KEY, BTN_MOUSE + 8);
            input_set_capability(hidinput->input, EV_KEY, BTN_MOUSE + 9);
            input_set_capability(hidinput->input, EV_KEY, BTN_MOUSE + 10);
            fallthrough;
        case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
        case USB_DEVICE_ID_RAZER_NAGA_2014:
        case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
            input_set_capability(hidinput->input, EV_REL, REL_HWHEEL);
            input_set_capability(hidinput->input, EV_REL, REL_HWHEEL_HI_RES);
            input_set_capability(hidinput->input, EV_KEY, BTN_FORWARD);
            input_set_capability(hidinput->input, EV_KEY, BTN_BACK);
            break;
        }
    }

    return 0;
}

/**
 * Mouse init function
 */
static void razer_mouse_init(struct razer_mouse_device *dev, struct usb_interface *intf, struct hid_device *hdev)
{
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    unsigned int rand_serial = 0;

    // Initialise mutex
    mutex_init(&dev->lock);
    // Setup values
    dev->usb_dev = usb_dev;
    dev->usb_vid = usb_dev->descriptor.idVendor;
    dev->usb_pid = usb_dev->descriptor.idProduct;
    dev->usb_interface_protocol = intf->cur_altsetting->desc.bInterfaceProtocol;
    dev->usb_interface_subclass = intf->cur_altsetting->desc.bInterfaceSubClass;

    // Get a "random" integer
    get_random_bytes(&rand_serial, sizeof(unsigned int));
    sprintf(&dev->serial[0], "PM%012u", rand_serial);

    // Setup orochi2011
    dev->orochi2011.dpi = 0x4c;
    dev->orochi2011.poll = 500;

    // Setup default values for DeathAdder 3.5G
    dev->da3_5g.leds = 3; // Lights up all lights
    dev->da3_5g.dpi = 1; // 3500 DPI
    dev->da3_5g.profile = 1; // Profile 1
    dev->da3_5g.poll = 1; // Poll rate 1000

    // Setup tilt wheel HWHEEL emulation
    hrtimer_init(&dev->repeat_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    dev->repeat_timer.function = wheel_tilt_repeat;
    dev->tilt_hwheel = 1;
    dev->tilt_repeat_delay = 250;
    dev->tilt_repeat = 33;
}

/**
 * Probe method is ran whenever a device is binded to the driver
 */
static int razer_mouse_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct razer_mouse_device *dev = NULL;
    unsigned char expected_subclass = 0xFF;

    dev = kzalloc(sizeof(struct razer_mouse_device), GFP_KERNEL);

    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        return -ENOMEM;
    }

    // Init data
    razer_mouse_init(dev, intf, hdev);

    switch(dev->usb_pid) {
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
    case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
        expected_subclass = 0x01;
        break;
    }

    if(dev->usb_interface_protocol == USB_INTERFACE_PROTOCOL_MOUSE
       && (expected_subclass == 0xFF || dev->usb_interface_subclass == expected_subclass)) {
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_test);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);

        switch(dev->usb_pid) {
        case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
        case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            fallthrough;
        case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);
            fallthrough;
        case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
        case USB_DEVICE_ID_RAZER_BASILISK:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_mode);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_acceleration);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_smart_reel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_mode);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_acceleration);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_smart_reel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
        case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_2011:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_IMPERATOR:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_OUROBOROS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_2013:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_HEX:
        case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
        case USB_DEVICE_ID_RAZER_TAIPAN:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_2014:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);
            fallthrough;
        case USB_DEVICE_ID_RAZER_NAGA_2012:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_on);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
        case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
        case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            break;

        case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
        case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_2000:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_X:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_left_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_right_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_hwheel);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat_delay);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_tilt_repeat);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_effect);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER:
        case USB_DEVICE_ID_RAZER_VIPER_MINI:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
        case USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED:
        case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
        case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
        case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
        case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_indicator_led_mode);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_indicator_led_mode);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_pair);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_unpair);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_8K:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_COBRA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_poll_rate);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_dpi_stages);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_idle_time);
            break;

        }

    }

    hid_set_drvdata(hdev, dev);
    dev_set_drvdata(&hdev->dev, dev);

    retval = hid_parse(hdev);
    if(retval)    {
        hid_err(hdev, "parse failed\n");
        goto exit_free;
    }
    retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (retval) {
        hid_err(hdev, "hw start failed\n");
        goto exit_free;
    }

    //razer_reset(usb_dev);
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
static void razer_mouse_disconnect(struct hid_device *hdev)
{
    struct razer_mouse_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);

    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {
        device_remove_file(&hdev->dev, &dev_attr_version);
        device_remove_file(&hdev->dev, &dev_attr_test);
        device_remove_file(&hdev->dev, &dev_attr_firmware_version);
        device_remove_file(&hdev->dev, &dev_attr_device_type);
        device_remove_file(&hdev->dev, &dev_attr_device_serial);
        device_remove_file(&hdev->dev, &dev_attr_device_mode);

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION:
        case USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED:
        case USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            fallthrough;
        case USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_left_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_right_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);
            device_remove_file(&hdev->dev, &dev_attr_left_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_right_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V2:
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);
            fallthrough;
        case USB_DEVICE_ID_RAZER_DEATHADDER_ELITE:
        case USB_DEVICE_ID_RAZER_BASILISK:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_scroll_mode);
            device_remove_file(&hdev->dev, &dev_attr_scroll_acceleration);
            device_remove_file(&hdev->dev, &dev_attr_scroll_smart_reel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);

            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_scroll_mode);
            device_remove_file(&hdev->dev, &dev_attr_scroll_acceleration);
            device_remove_file(&hdev->dev, &dev_attr_scroll_smart_reel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);

            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_HEX_V2:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_backlight_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_backlight_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS:
        case USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_MAMBA_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            device_remove_file(&hdev->dev, &dev_attr_backlight_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_2011:
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS:
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_IMPERATOR:
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_OUROBOROS:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_2013:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_2013:
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_OROCHI_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_HEX:
        case USB_DEVICE_ID_RAZER_NAGA_HEX_RED:
        case USB_DEVICE_ID_RAZER_TAIPAN:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_2014:
            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);
            fallthrough;
        case USB_DEVICE_ID_RAZER_NAGA_2012:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_on);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA:
        case USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_V2:
        case USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_3500:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_blinking);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_2000:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            break;

        case USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            device_remove_file(&hdev->dev, &dev_attr_backlight_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_backlight_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_1800:
        case USB_DEVICE_ID_RAZER_DEATHADDER_1800:
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_ABYSSUS_2000:
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_on);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_X:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_left_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_right_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);

            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_ELITE:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_left_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_left_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_right_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_right_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);

            device_remove_file(&hdev->dev, &dev_attr_tilt_hwheel);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat_delay);
            device_remove_file(&hdev->dev, &dev_attr_tilt_repeat);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL:
        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_TRINITY:
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            break;

        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_charge_effect);
            device_remove_file(&hdev->dev, &dev_attr_charge_colour);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER:
        case USB_DEVICE_ID_RAZER_VIPER_MINI:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER:
        case USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED:
        case USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER:
        case USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED:
        case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED:
        case USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS:
        case USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER:
        case USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS:
            device_remove_file(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_indicator_led_mode);
            fallthrough;
        case USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            device_remove_file(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_indicator_led_mode);
            device_remove_file(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_pair);
            device_remove_file(&hdev->dev, &dev_attr_hyperpolling_wireless_dongle_unpair);
            break;

        case USB_DEVICE_ID_RAZER_VIPER_8K:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            break;

        case USB_DEVICE_ID_RAZER_COBRA:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);

            device_remove_file(&hdev->dev, &dev_attr_logo_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_logo_matrix_effect_breath);
            break;

        case USB_DEVICE_ID_RAZER_DEATHADDER_V3:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            break;

        case USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);
            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        case USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED:
            device_remove_file(&hdev->dev, &dev_attr_poll_rate);
            device_remove_file(&hdev->dev, &dev_attr_dpi);
            device_remove_file(&hdev->dev, &dev_attr_dpi_stages);

            device_remove_file(&hdev->dev, &dev_attr_scroll_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_reactive);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_scroll_matrix_effect_none);

            device_remove_file(&hdev->dev, &dev_attr_charge_level);
            device_remove_file(&hdev->dev, &dev_attr_charge_status);
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);
            device_remove_file(&hdev->dev, &dev_attr_device_idle_time);
            break;

        }

    }

    hid_hw_stop(hdev);
    hrtimer_cancel(&dev->repeat_timer);

    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OROCHI_2011) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS_1800) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS_2000) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_3_5G) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_3_5G_BLACK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_HEX_RED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_2012) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_2014) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_HEX) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_2012_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_2012_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_TE_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_TAIPAN) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_IMPERATOR) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OUROBOROS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_2013) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OROCHI_2013) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OROCHI_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_HEX_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_EPIC_CHROMA_DOCK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_ELITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DIAMONDBACK_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_3500) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LANCEHEAD_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LANCEHEAD_TE_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_TRINITY) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_ELITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_2021) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_1800) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LANCEHEAD_WIRELESS_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MAMBA_WIRELESS_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS_ELITE_DVA_EDITION) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ABYSSUS_ESSENTIAL) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_ESSENTIAL_WHITE_EDITION) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_MINI) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_MINI_SE_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_ULTIMATE_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_ESSENTIAL) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_ULTIMATE_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_V3) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2_MINI) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2_X_HYPERSPEED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_2000) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_ATHERIS_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_X_HYPERSPEED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_X) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_LEFT_HANDED_2020) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_8K) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OROCHI_V2_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_OROCHI_V2_BLUETOOTH) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_PRO_CLICK_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_PRO_CLICK_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_V2_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_COBRA_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_COBRA_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V3) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V3_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_HYPERPOLLING_WIRELESS_DONGLE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_V3_PRO_WIRELESS) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_PRO_CLICK_MINI_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_DEATHADDER_V2_LITE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_COBRA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NAGA_V2_HYPERSPEED_RECEIVER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_VIPER_V3_HYPERSPEED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASILISK_V3_X_HYPERSPEED) },
    { 0 }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_mouse_driver = {
    .name      = "razermouse",
    .id_table  = razer_devices,
    .probe     = razer_mouse_probe,
    .remove    = razer_mouse_disconnect,

    .raw_event = razer_raw_event,
    .input_mapping = razer_input_mapping,
    .input_configured = razer_input_configured,
};

module_hid_driver(razer_mouse_driver);
