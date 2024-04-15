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
#include <linux/random.h>

#include "razeraccessory_driver.h"
#include "razercommon.h"
#include "razerchromacommon.h"

/*
 * Version Information
 */
#define DRIVER_DESC "Razer Accessory Device Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

/**
 * Send report to the device
 */
static int razer_get_report(struct usb_device *usb_dev, struct razer_report *request, struct razer_report *response)
{
    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        return razer_get_usb_response(usb_dev, 0x00, request, 0x00, response, RAZER_NEW_DEVICE_WAIT_MIN_US, RAZER_NEW_DEVICE_WAIT_MAX_US);
        break;

    default:
        return razer_get_usb_response(usb_dev, 0x00, request, 0x00, response, RAZER_ACCESSORY_WAIT_MIN_US, RAZER_ACCESSORY_WAIT_MAX_US);
    }
}

/**
 * Function to send to device, get response, and actually check the response
 */
static int razer_send_payload(struct razer_accessory_device *device, struct razer_report *request, struct razer_report *response)
{
    int err;

    request->crc = razer_calculate_crc(request);

    mutex_lock(&device->lock);
    err = razer_get_report(device->usb_dev, request, response);
    mutex_unlock(&device->lock);
    if (err) {
        print_erroneous_report(response, "razeraccessory", "Invalid Report Length");
        return err;
    }

    /* Check the packet number, class and command are the same */
    if (response->remaining_packets != request->remaining_packets ||
        response->command_class != request->command_class ||
        response->command_id.id != request->command_id.id) {
        print_erroneous_report(response, "razeraccessory", "Response doesn't match request");
        return -EIO;
    }

    switch (response->status) {
    case RAZER_CMD_BUSY:
        // TODO: Check if this should be an error.
        // print_erroneous_report(&response, "razeraccessory", "Device is busy");
        break;
    case RAZER_CMD_FAILURE:
        print_erroneous_report(response, "razeraccessory", "Command failed");
        return -EIO;
    case RAZER_CMD_NOT_SUPPORTED:
        print_erroneous_report(response, "razeraccessory", "Command not supported");
        return -EIO;
    case RAZER_CMD_TIMEOUT:
        print_erroneous_report(response, "razeraccessory", "Command timed out");
        return -EIO;
    }

    return 0;
}

/**
 * Device mode function
 */
static void razer_set_device_mode(struct razer_accessory_device *device, unsigned char mode, unsigned char param)
{
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_set_device_mode(mode, param);
    request.transaction_id.id = 0x3F;

    razer_send_payload(device, &request, &response);
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
    struct razer_accessory_device *device = dev_get_drvdata(dev);

    char *device_type;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
        device_type = "Razer Firefly\n";
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        device_type = "Razer Firefly V2\n";
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        device_type = "Razer Firefly Hyperflux\n";
        break;

    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        device_type = "Razer Goliathus\n";
        break;

    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        device_type = "Razer Goliathus Extended\n";
        break;

    case USB_DEVICE_ID_RAZER_CORE:
        device_type = "Razer Core\n";
        break;

    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        device_type = "Razer Core X Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        device_type = "Razer Laptop Stand Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        device_type = "Razer Chroma Mug Holder\n";
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        device_type = "Razer Chroma HDK\n";
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        device_type = "Razer Base Station Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        device_type = "Razer Base Station V2 Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        device_type = "Razer Nommo Pro\n";
        break;

    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        device_type = "Razer Nommo Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        device_type = "Razer Kraken Kitty Edition\n";
        break;

    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        device_type = "Razer Mouse Bungee V3 Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        device_type = "Razer Charging Pad Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
        device_type = "Razer Mouse Dock\n";
        break;

    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        device_type = "Razer Thunderbolt 4 Dock Chroma\n";
        break;

    case USB_DEVICE_ID_RAZER_RAPTOR_27:
        device_type = "Razer Raptor 27\n";
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        device_type = "Razer Chroma Addressable RGB Controller\n";
        break;

    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        device_type = "Razer Laptop Stand Chroma V2\n";
        break;

    default:
        device_type = "Unknown Device\n";
        break;
    }

    return sprintf(buf, device_type);
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
    return sprintf(buf, "\n");
}

/**
 * Write device file "mode_spectrum"
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_matrix_effect_spectrum();
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Must be in normal mode for hardware effects
        razer_set_device_mode(device, 0x00, 0x00);
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
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
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char speed;

    if (count != 4) {
        printk(KERN_WARNING "razeraccessory: Reactive only accepts Speed, RGB (4byte)\n");
        return -EINVAL;
    }

    speed = (unsigned char)buf[0];

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, ZERO_LED, speed, (struct razer_rgb *)&buf[1]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        request = razer_chroma_standard_matrix_effect_reactive(speed, (struct razer_rgb*)&buf[1]);
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "matrix_reactive_trigger"
 *
 * It triggers the mouse pad when written to
 */
static ssize_t razer_attr_write_matrix_reactive_trigger(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    struct razer_rgb rgb = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        // TODO: Fix reactive trigger for Goliathus
        request = razer_chroma_extended_matrix_effect_reactive(VARSTORE, ZERO_LED, 0, &rgb);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
        // TODO: Issue zeroed out razer_chroma_standard_matrix_effect_reactive report
        request = razer_chroma_misc_matrix_reactive_trigger();
        request.transaction_id.id = 0xFF;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_none"
 *
 * None effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_matrix_effect_none();
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Must be in normal mode for hardware effects
        razer_set_device_mode(device, 0x00, 0x00);
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, ZERO_LED);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_blinking"
 *
 * Blinking effect mode is activated whenever the file is written to with 3 bytes
 */
static ssize_t razer_attr_write_matrix_effect_blinking(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razeraccessory: Blinking mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    request = razer_chroma_standard_set_led_rgb(VARSTORE, BACKLIGHT_LED, (struct razer_rgb*)&buf[0]);
    request.transaction_id.id = 0x3F;

    razer_send_payload(device, &request, &response);

    msleep(5);

    request = razer_chroma_standard_set_led_effect(VARSTORE, BACKLIGHT_LED, CLASSIC_EFFECT_BLINKING);
    request.transaction_id.id = 0x3F;

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_custom"
 *
 * Sets the device to custom mode whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_matrix_effect_custom_frame(NOSTORE);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_effect_custom_frame();
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_static"
 *
 * Static effect mode is activated whenever the file is written to with 3 bytes
 */
static ssize_t razer_attr_write_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razeraccessory: Static mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_matrix_effect_static((struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Must be in normal mode for hardware effects
        razer_set_device_mode(device, 0x00, 0x00);
        /**
            * Mode switcher required after setting static color effect once and before setting a second time.
            * Similar to Naga Trinity?
            *
            * If the color is not set twice with the mode switch in-between, each subsequent
            * setting of the static effect actually sets the previous color...
            */
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x1F;

        razer_send_payload(device, &request, &response);

        request = get_razer_report(0x0f, 0x02, 0x06);
        request.arguments[0] = 0x00;
        request.arguments[1] = 0x00;
        request.arguments[2] = 0x08;
        request.arguments[3] = 0x00;
        request.arguments[4] = 0x00;
        request.arguments[5] = 0x00;
        request.transaction_id.id = 0x1F;

        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_wave"
 *
 * When 1 is written (as a character, 0x31) the wave effect is displayed moving anti clockwise
 * if 2 is written (0x32) then the wave effect goes clockwise
 */
static ssize_t razer_attr_write_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_matrix_effect_wave(direction);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, ZERO_LED, direction);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, ZERO_LED, direction);
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Must be in normal mode for hardware effects
        razer_set_device_mode(device, 0x00, 0x00);
        fallthrough;
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        // Direction values are flipped compared to other devices
        direction ^= ((1<<0) | (1<<1));
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, ZERO_LED, direction);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "mode_breath"
 *
 * Breathing effect mode is activated whenever the file is written to with 1, 3, or 6 bytes
 */
static ssize_t razer_attr_write_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0], (struct razer_rgb *)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, ZERO_LED);
            break;
        }
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0], (struct razer_rgb *)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, ZERO_LED);
            break;
        }
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Must be in normal mode for hardware effects
        razer_set_device_mode(device, 0x00, 0x00);
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0], (struct razer_rgb *)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, ZERO_LED);
            break;
        }
        request.transaction_id.id = 0x1F;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
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
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

static ssize_t razer_attr_write_matrix_effect_starlight(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char speed = 0;

    if (count != 1 && count != 4 && count != 7) {
        printk(KERN_WARNING "razeraccessory: Starlight accepts only 1, 4 or 7 bytes input (speed, [RGB], [RGB])\n");
        return -EINVAL;
    }
    speed = buf[0];

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        switch(count) {
        case 4: // Single colour mode
            request = razer_chroma_extended_matrix_effect_starlight_single(VARSTORE, ZERO_LED, speed, (struct razer_rgb *)&buf[1]);
            break;

        case 7: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_starlight_dual(VARSTORE, ZERO_LED, speed, (struct razer_rgb *)&buf[1], (struct razer_rgb *)&buf[4]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_starlight_random(VARSTORE, ZERO_LED, speed);
            break;
        }
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "set_key_row"
 *
 * Writes the colour to the LEDs of the device
 *
 * Start is 0x00
 * Stop is 0x0E
 *
 * As you go from 0x00 -> 0x0E the leds light up in a clockwise direction.
 * 0x01,0x03,0x05,0x07,0x09,0x0B,0x0D Are NOT connected
 */
static ssize_t razer_attr_write_matrix_custom_frame(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    size_t offset = 0;
    unsigned char row_id;
    unsigned char start_col;
    unsigned char stop_col;
    unsigned char row_length;

    //printk(KERN_ALERT "razermyg: Total count: %d\n", (unsigned char)count);

    while(offset < count) {
        if(offset + 3 > count) {
            printk(KERN_ALERT "razeraccessory: Wrong Amount of data provided: Should be ROW_ID, START_COL, STOP_COL, N_RGB\n");
            return -EINVAL;
        }

        row_id = buf[offset++];
        start_col = buf[offset++];
        stop_col = buf[offset++];
        row_length = ((stop_col+1) - start_col) * 3;

        // printk(KERN_ALERT "razeraccessory: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_id, start_col, stop_col, row_length);

        if(start_col > stop_col) {
            printk(KERN_ALERT "razeraccessory: Start column is greater than end column\n");
            return -EINVAL;
        }

        if(offset + row_length > count) {
            printk(KERN_ALERT "razeraccessory: Not enough RGB to fill row\n");
            return -EINVAL;
        }

        switch (device->usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CORE:
            request = razer_chroma_standard_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0xFF;
            break;

        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
            request = razer_chroma_misc_one_row_set_custom_frame(start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0xFF;
            break;

        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
            request = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char*)&buf[offset]);
            request.transaction_id.id = 0x3F;
            break;

        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_RAPTOR_27:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
            request = razer_chroma_extended_matrix_set_custom_frame2(row_id, start_col, stop_col, (unsigned char*)&buf[offset], 0);
            request.transaction_id.id = 0x1F;
            break;

        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
            // Must be in driver mode for custom effects
            razer_set_device_mode(device, 0x03, 0x00);
            request = razer_chroma_extended_matrix_set_custom_frame2(row_id, start_col, stop_col, (unsigned char*)&buf[offset], 0);
            request.transaction_id.id = 0x1F;
            break;

        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
            razer_send_argb_msg(device->usb_dev, row_id, (stop_col - start_col) + 1, (unsigned char*)&buf[offset]);
            return count;

        default:
            printk(KERN_WARNING "razeraccessory: Unknown device\n");
            return -EINVAL;
        }

        razer_send_payload(device, &request, &response);

        // *3 as its 3 bytes per col (RGB)
        offset += row_length;
    }

    return count;
}

/**
 * Read device file "serial", doesn't have a proper one so one is generated
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    char serial_string[51];

    request = razer_chroma_standard_get_serial();

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        strncpy(&serial_string[0], &device->serial[0], sizeof(serial_string));
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        strncpy(&serial_string[0], &response.arguments[0], 22);
        serial_string[22] = '\0';
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request.transaction_id.id = 0x1F;
        razer_send_payload(device, &request, &response);
        strncpy(&serial_string[0], &response.arguments[0], 22);
        serial_string[22] = '\0';
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    return sprintf(buf, "%s\n", &serial_string[0]);
}

/**
 * Read device file "get_firmware_version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_firmware_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_firmware_version();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request.transaction_id.id = 0x1F;
        break;

    default:
        request.transaction_id.id = 0x3F;
        break;
    }

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "v%u.%u\n", response.arguments[0], response.arguments[1]);
}

/**
 * Write device file "device_mode"
 */
static ssize_t razer_attr_write_device_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 2) {
        printk(KERN_WARNING "razeraccessory: Device mode only takes 2 bytes.\n");
        return -EINVAL;
    }

    request = razer_chroma_standard_set_device_mode(buf[0], buf[1]);

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request.transaction_id.id = 0x1F;
        break;

    default:
        request.transaction_id.id = 0xFF;
        break;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

static ssize_t razer_attr_read_is_mug_present(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = get_razer_report(0x02, 0x81, 0x02);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%u\n", response.arguments[1]);
}

/**
 * Read device file "device_mode"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_standard_get_device_mode();

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request.transaction_id.id = 0x1F;
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
 * Write device file "set_brightness"
 *
 * Sets the brightness to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_matrix_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    unsigned char brightness = 0;
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count < 1) {
        printk(KERN_WARNING "razeraccessory: Brightness takes an ascii number\n");
        return -EINVAL;
    }

    brightness = (unsigned char)simple_strtoul(buf, NULL, 10);

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x3F;
        device->saved_brightness = brightness;
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x1F;
        device->saved_brightness = brightness;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        request = razer_chroma_standard_set_led_brightness(VARSTORE, BACKLIGHT_LED, brightness);
        request.transaction_id.id = 0xFF;
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        request.transaction_id.id = 0x3F;
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        /* Set the brightness for all channels to the requested value */
        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_1_LED, brightness);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_2_LED, brightness);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_3_LED, brightness);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_4_LED, brightness);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_5_LED, brightness);
        request.transaction_id.id = 0x3F;
        razer_send_payload(device, &request, &response);

        request = razer_chroma_extended_matrix_brightness(VARSTORE, ARGB_CH_6_LED, brightness);
        request.transaction_id.id = 0x3F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "set_brightness"
 *
 * Returns brightness or -1 if the initial brightness is not known
 */
static ssize_t razer_attr_read_matrix_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report response = {0};
    struct razer_report request = {0};
    unsigned char brightness = 0;
    size_t sum = 0;
    size_t i;

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        brightness = device->saved_brightness;
        break;

    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        /* Get the average brightness of all channels */
        for (i = ARGB_CH_1_LED; i <= ARGB_CH_6_LED; i++) {
            request = razer_chroma_extended_matrix_get_brightness(VARSTORE, i);
            request.transaction_id.id = 0x3F;
            razer_send_payload(device, &request, &response);
            sum += response.arguments[2];
        }
        brightness = sum / 6;
        break;

    default:
        request = razer_chroma_standard_get_led_brightness(VARSTORE, BACKLIGHT_LED);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        brightness = response.arguments[2];
        break;
    }

    return sprintf(buf, "%d\n", brightness);
}

/**
 * Write charge brightness device files
 *
 * Sets the brightness to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_set_charge_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    unsigned char brightness = 0;
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count < 1) {
        printk(KERN_WARNING "razeraccessory: Brightness takes an ascii number\n");
        return -EINVAL;
    }

    brightness = (unsigned char)simple_strtoul(buf, NULL, 10);

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        request = razer_chroma_extended_matrix_brightness(VARSTORE, led, brightness);
        request.transaction_id.id = 0x1F;
        device->saved_brightness = brightness;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read charge brightness device files
 *
 * Returns brightness or -1 if the initial brightness is not known
 */
static ssize_t razer_attr_read_set_charge_brightness(struct device *dev, struct device_attribute *attr, char *buf, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char brightness = 0;

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        brightness = device->saved_brightness;
        break;

    default:
        request = razer_chroma_standard_get_led_brightness(VARSTORE, led);
        request.transaction_id.id = 0xFF;
        razer_send_payload(device, &request, &response);
        brightness = response.arguments[2];
        break;
    }

    return sprintf(buf, "%d\n", brightness);
}

/**
 * Read device file "charging_led_brightness"
 */
static ssize_t razer_attr_read_charging_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_set_charge_brightness(dev, attr, buf, CHARGING_LED);
}

/**
 * Write device file "charging_led_brightness"
 */
static ssize_t razer_attr_write_charging_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_set_charge_brightness(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Read device file "fast_charging_led_brightness"
 */
static ssize_t razer_attr_read_fast_charging_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_set_charge_brightness(dev, attr, buf, FAST_CHARGING_LED);
}

/**
 * Write device file "fast_charging_led_brightness"
 */
static ssize_t razer_attr_write_fast_charging_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_set_charge_brightness(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Read device file "fully_charged_led_brightness"
 */
static ssize_t razer_attr_read_fully_charged_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_set_charge_brightness(dev, attr, buf, FULLY_CHARGED_LED);
}

/**
 * Write device file "fully_charged_led_brightness"
 */
static ssize_t razer_attr_write_fully_charged_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_set_charge_brightness(dev, attr, buf, count, FULLY_CHARGED_LED);
}

/**
 * Write device file "mode_spectrum"
 *
 * Spectrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_charge_mode_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        request = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, led);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charging_mode_spectrum"
 */
static ssize_t razer_attr_write_charging_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_charge_mode_spectrum(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Write device file "fast_charging_mode_spectrum"
 */
static ssize_t razer_attr_write_fast_charging_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_charge_mode_spectrum(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Write device file "fully_charged_mode_spectrum"
 */
static ssize_t razer_attr_write_fully_charged_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_charge_mode_spectrum(dev, attr, buf, count, FULLY_CHARGED_LED);
}

static ssize_t razer_attr_write_matrix_effect_none_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        request = razer_chroma_extended_matrix_effect_none(VARSTORE, led);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charging_mode_none"
 */
static ssize_t razer_attr_write_charging_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Write device file "fast_charging_mode_none"
 */
static ssize_t razer_attr_write_fast_charging_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Write device file "fully_charged_mode_none"
 */
static ssize_t razer_attr_write_fully_charged_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_none_common(dev, attr, buf, count, FULLY_CHARGED_LED);
}

static ssize_t razer_attr_write_matrix_effect_static_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count != 3) {
        printk(KERN_WARNING "razeraccessory: Static mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        request = razer_chroma_extended_matrix_effect_static(VARSTORE, led, (struct razer_rgb*) & buf[0]);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charging_mode_static"
 */
static ssize_t razer_attr_write_charging_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Write device file "fast_charging_mode_static"
 */
static ssize_t razer_attr_write_fast_charging_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Write device file "fully_charged_mode_static"
 */
static ssize_t razer_attr_write_fully_charged_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_static_common(dev, attr, buf, count, FULLY_CHARGED_LED);
}

static ssize_t razer_attr_write_matrix_effect_wave_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    unsigned char direction = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        // Direction values are flipped compared to other devices
        direction ^= ((1<<0) | (1<<1));
        request = razer_chroma_extended_matrix_effect_wave(VARSTORE, led, direction);
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charging_mode_wave"
 */
static ssize_t razer_attr_write_charging_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Write device file "fast_charging_mode_wave"
 */
static ssize_t razer_attr_write_fast_charging_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Write device file "fully_charged_mode_wave"
 */
static ssize_t razer_attr_write_fully_charged_matrix_effect_wave(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_wave_common(dev, attr, buf, count, FULLY_CHARGED_LED);
}

static ssize_t razer_attr_write_matrix_effect_breath_common(struct device *dev, struct device_attribute *attr, const char *buf, size_t count, int led)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    switch (device->usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        switch(count) {
        case 3: // Single colour mode
            request = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, led, (struct razer_rgb *)&buf[0]);
            break;

        case 6: // Dual colour mode
            request = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, led, (struct razer_rgb *)&buf[0], (struct razer_rgb *)&buf[3]);
            break;

        default: // "Random" colour mode
            request = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, led);
            break;
        }
        request.transaction_id.id = 0x1F;
        break;

    default:
        printk(KERN_WARNING "razeraccessory: Unknown device\n");
        return -EINVAL;
    }

    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Write device file "charging_mode_breath"
 */
static ssize_t razer_attr_write_charging_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, CHARGING_LED);
}

/**
 * Write device file "fast_charging_mode_breath"
 */
static ssize_t razer_attr_write_fast_charging_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, FAST_CHARGING_LED);
}

/**
 * Write device file "fully_charged_mode_breath"
 */
static ssize_t razer_attr_write_fully_charged_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_matrix_effect_breath_common(dev, attr, buf, count, FULLY_CHARGED_LED);
}

/**
 * Sets the brightness to the ASCII number
 */
static ssize_t razer_attr_write_channel_led_brightness(unsigned char led, struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    unsigned char brightness = 0;
    struct razer_report request = {0};
    struct razer_report response = {0};

    if (count < 1) {
        printk(KERN_WARNING "razeraccessory: Brightness takes an ascii number\n");
        return -EINVAL;
    }

    brightness = (unsigned char)simple_strtoul(buf, NULL, 10);

    request = razer_chroma_extended_matrix_brightness(VARSTORE, led, brightness);
    request.transaction_id.id = 0x3F;

    razer_send_payload(device, &request, &response);

    return count;
}

static ssize_t razer_attr_write_channel1_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_1_LED, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel2_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_2_LED, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel3_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_3_LED, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel4_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_4_LED, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel5_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_5_LED, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel6_led_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_led_brightness(ARGB_CH_6_LED, dev, attr, buf, count);
}

/**
 * Read device file "channelX_size"
 */
static ssize_t razer_attr_read_channel_size(unsigned int channel, struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = get_razer_report(0x0f, 0x88, 0x0d);
    request.transaction_id.id = 0x1F;
    request.arguments[0] = 0x06;

    razer_send_payload(device, &request, &response);

    return sprintf(buf, "%d\n", response.arguments[channel * 2]);
}

static ssize_t razer_attr_read_channel1_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(1, dev, attr, buf);
}

static ssize_t razer_attr_read_channel2_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(2, dev, attr, buf);
}

static ssize_t razer_attr_read_channel3_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(3, dev, attr, buf);
}

static ssize_t razer_attr_read_channel4_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(4, dev, attr, buf);
}

static ssize_t razer_attr_read_channel5_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(5, dev, attr, buf);
}

static ssize_t razer_attr_read_channel6_size(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_size(6, dev, attr, buf);
}

/**
 * Write device file "channelX_size"
 */
static ssize_t razer_attr_write_channel_size(unsigned int channel, struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned char sz;
    struct razer_report request = {0};
    struct razer_report response = {0};
    struct razer_accessory_device *device;

    if (count < 1) {
        printk(KERN_WARNING "razeraccessory: Size takes an ascii number\n");
        return -EINVAL;
    }

    device = dev_get_drvdata(dev);

    /* Get existing sizes */
    request = get_razer_report(0x0f, 0x88, 0x0d);
    request.transaction_id.id = 0x1F;
    request.arguments[0] = 0x06;

    razer_send_payload(device, &request, &response);

    /* Set new sizes */
    sz = (unsigned char)simple_strtoul(buf, NULL, 10);

    request = get_razer_report(0x0f, 0x08, 0x0d);
    request.transaction_id.id = 0xFF;
    request.arguments[0] = 0x06;
    request.arguments[1] = 0x01;
    request.arguments[2] = channel == 1 ? sz : response.arguments[2];
    request.arguments[3] = 0x02;
    request.arguments[4] = channel == 2 ? sz : response.arguments[4];
    request.arguments[5] = 0x03;
    request.arguments[6] = channel == 3 ? sz : response.arguments[6];
    request.arguments[7] = 0x04;
    request.arguments[8] = channel == 4 ? sz : response.arguments[8];
    request.arguments[9] = 0x05;
    request.arguments[10] = channel == 5 ? sz : response.arguments[10];
    request.arguments[11] = 0x06;
    request.arguments[12] = channel == 6 ? sz : response.arguments[12];

    razer_send_payload(device, &request, &response);

    return count;
}

static ssize_t razer_attr_write_channel1_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(1, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel2_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(2, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel3_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(3, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel4_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(4, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel5_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(5, dev, attr, buf, count);
}

static ssize_t razer_attr_write_channel6_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return razer_attr_write_channel_size(6, dev, attr, buf, count);
}

static ssize_t razer_attr_write_reset_channels(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);

    /* Get existing sizes */
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned int i;

    for (i = 0; i < 6; i++) {
        request = get_razer_report(0x0f, 0x04, 0x03);
        request.transaction_id.id = 0x1F;
        request.arguments[0] = 0x01;
        request.arguments[1] = ARGB_CH_1_LED + i;
        request.arguments[2] = 0xff;

        razer_send_payload(device, &request, &response);
    }

    request = get_razer_report(0x00, 0xb7, 0x01);
    request.transaction_id.id = 0x1F;
    request.arguments[0] = 0x00;
    razer_send_payload(device, &request, &response);

    request = get_razer_report(0x00, 0x36, 0x01);
    request.transaction_id.id = 0x1F;
    request.arguments[0] = 0x01;
    razer_send_payload(device, &request, &response);

    return count;
}

/**
 * Read device file "channelX_led_brightness"
 *
 * Returns brightness or -1 if the initial brightness is not known
 */
static ssize_t razer_attr_read_channel_led_brightness(unsigned char led, struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_accessory_device *device = dev_get_drvdata(dev);
    struct razer_report request = {0};
    struct razer_report response = {0};
    unsigned char brightness = 0;

    request = razer_chroma_extended_matrix_get_brightness(VARSTORE, led);
    request.transaction_id.id = 0x3F;

    razer_send_payload(device, &request, &response);
    brightness = response.arguments[2];

    return sprintf(buf, "%d\n", brightness);
}

static ssize_t razer_attr_read_channel1_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_1_LED, dev, attr, buf);
}

static ssize_t razer_attr_read_channel2_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_2_LED, dev, attr, buf);
}

static ssize_t razer_attr_read_channel3_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_3_LED, dev, attr, buf);
}

static ssize_t razer_attr_read_channel4_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_4_LED, dev, attr, buf);
}

static ssize_t razer_attr_read_channel5_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_5_LED, dev, attr, buf);
}

static ssize_t razer_attr_read_channel6_led_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    return razer_attr_read_channel_led_brightness(ARGB_CH_6_LED, dev, attr, buf);
}

/**
 * Set up the device driver files

 *
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */

static DEVICE_ATTR(test,                                    0660, razer_attr_read_test,                           razer_attr_write_test);
static DEVICE_ATTR(version,                                 0440, razer_attr_read_version,                        NULL);
static DEVICE_ATTR(device_type,                             0440, razer_attr_read_device_type,                    NULL);
static DEVICE_ATTR(device_mode,                             0660, razer_attr_read_device_mode,                    razer_attr_write_device_mode);
static DEVICE_ATTR(device_serial,                           0440, razer_attr_read_device_serial,                  NULL);
static DEVICE_ATTR(firmware_version,                        0440, razer_attr_read_firmware_version,               NULL);

static DEVICE_ATTR(matrix_effect_none,                      0220, NULL,                                           razer_attr_write_matrix_effect_none);
static DEVICE_ATTR(matrix_effect_spectrum,                  0220, NULL,                                           razer_attr_write_matrix_effect_spectrum);
static DEVICE_ATTR(matrix_effect_static,                    0220, NULL,                                           razer_attr_write_matrix_effect_static);
static DEVICE_ATTR(matrix_effect_reactive,                  0220, NULL,                                           razer_attr_write_matrix_effect_reactive);
static DEVICE_ATTR(matrix_effect_breath,                    0220, NULL,                                           razer_attr_write_matrix_effect_breath);
static DEVICE_ATTR(matrix_effect_custom,                    0220, NULL,                                           razer_attr_write_matrix_effect_custom);
static DEVICE_ATTR(matrix_effect_wave,                      0220, NULL,                                           razer_attr_write_matrix_effect_wave);
static DEVICE_ATTR(matrix_effect_blinking,                  0220, NULL,                                           razer_attr_write_matrix_effect_blinking);
static DEVICE_ATTR(matrix_effect_starlight,                 0220, NULL,                                           razer_attr_write_matrix_effect_starlight);
static DEVICE_ATTR(matrix_brightness,                       0660, razer_attr_read_matrix_brightness,              razer_attr_write_matrix_brightness);
static DEVICE_ATTR(matrix_custom_frame,                     0220, NULL,                                           razer_attr_write_matrix_custom_frame);
static DEVICE_ATTR(matrix_reactive_trigger,                 0220, NULL,                                           razer_attr_write_matrix_reactive_trigger);

static DEVICE_ATTR(charging_led_brightness,                 0660, razer_attr_read_charging_led_brightness,        razer_attr_write_charging_led_brightness);
static DEVICE_ATTR(charging_matrix_effect_wave,             0220, NULL,                                           razer_attr_write_charging_matrix_effect_wave);
static DEVICE_ATTR(charging_matrix_effect_spectrum,         0220, NULL,                                           razer_attr_write_charging_matrix_effect_spectrum);
static DEVICE_ATTR(charging_matrix_effect_breath,           0220, NULL,                                           razer_attr_write_charging_matrix_effect_breath);
static DEVICE_ATTR(charging_matrix_effect_static,           0220, NULL,                                           razer_attr_write_charging_matrix_effect_static);
static DEVICE_ATTR(charging_matrix_effect_none,             0220, NULL,                                           razer_attr_write_charging_matrix_effect_none);

static DEVICE_ATTR(fast_charging_led_brightness,            0660, razer_attr_read_fast_charging_led_brightness,   razer_attr_write_fast_charging_led_brightness);
static DEVICE_ATTR(fast_charging_matrix_effect_wave,        0220, NULL,                                           razer_attr_write_fast_charging_matrix_effect_wave);
static DEVICE_ATTR(fast_charging_matrix_effect_spectrum,    0220, NULL,                                           razer_attr_write_fast_charging_matrix_effect_spectrum);
static DEVICE_ATTR(fast_charging_matrix_effect_breath,      0220, NULL,                                           razer_attr_write_fast_charging_matrix_effect_breath);
static DEVICE_ATTR(fast_charging_matrix_effect_static,      0220, NULL,                                           razer_attr_write_fast_charging_matrix_effect_static);
static DEVICE_ATTR(fast_charging_matrix_effect_none,        0220, NULL,                                           razer_attr_write_fast_charging_matrix_effect_none);

static DEVICE_ATTR(fully_charged_led_brightness,            0660, razer_attr_read_fully_charged_led_brightness,   razer_attr_write_fully_charged_led_brightness);
static DEVICE_ATTR(fully_charged_matrix_effect_wave,        0220, NULL,                                           razer_attr_write_fully_charged_matrix_effect_wave);
static DEVICE_ATTR(fully_charged_matrix_effect_spectrum,    0220, NULL,                                           razer_attr_write_fully_charged_matrix_effect_spectrum);
static DEVICE_ATTR(fully_charged_matrix_effect_breath,      0220, NULL,                                           razer_attr_write_fully_charged_matrix_effect_breath);
static DEVICE_ATTR(fully_charged_matrix_effect_static,      0220, NULL,                                           razer_attr_write_fully_charged_matrix_effect_static);
static DEVICE_ATTR(fully_charged_matrix_effect_none,        0220, NULL,                                           razer_attr_write_fully_charged_matrix_effect_none);

static DEVICE_ATTR(reset_channels,                          0220, NULL,                                           razer_attr_write_reset_channels);
static DEVICE_ATTR(channel1_size,                           0660, razer_attr_read_channel1_size,                  razer_attr_write_channel1_size);
static DEVICE_ATTR(channel2_size,                           0660, razer_attr_read_channel2_size,                  razer_attr_write_channel2_size);
static DEVICE_ATTR(channel3_size,                           0660, razer_attr_read_channel3_size,                  razer_attr_write_channel3_size);
static DEVICE_ATTR(channel4_size,                           0660, razer_attr_read_channel4_size,                  razer_attr_write_channel4_size);
static DEVICE_ATTR(channel5_size,                           0660, razer_attr_read_channel5_size,                  razer_attr_write_channel5_size);
static DEVICE_ATTR(channel6_size,                           0660, razer_attr_read_channel6_size,                  razer_attr_write_channel6_size);
static DEVICE_ATTR(channel1_led_brightness,                 0660, razer_attr_read_channel1_led_brightness,        razer_attr_write_channel1_led_brightness);
static DEVICE_ATTR(channel2_led_brightness,                 0660, razer_attr_read_channel2_led_brightness,        razer_attr_write_channel2_led_brightness);
static DEVICE_ATTR(channel3_led_brightness,                 0660, razer_attr_read_channel3_led_brightness,        razer_attr_write_channel3_led_brightness);
static DEVICE_ATTR(channel4_led_brightness,                 0660, razer_attr_read_channel4_led_brightness,        razer_attr_write_channel4_led_brightness);
static DEVICE_ATTR(channel5_led_brightness,                 0660, razer_attr_read_channel5_led_brightness,        razer_attr_write_channel5_led_brightness);
static DEVICE_ATTR(channel6_led_brightness,                 0660, razer_attr_read_channel6_led_brightness,        razer_attr_write_channel6_led_brightness);

static DEVICE_ATTR(is_mug_present,                          0440, razer_attr_read_is_mug_present,                 NULL);

static void razer_accessory_init(struct razer_accessory_device *dev, struct usb_interface *intf, struct hid_device *hdev)
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

    // Get a "random" integer
    get_random_bytes(&rand_serial, sizeof(unsigned int));
    sprintf(&dev->serial[0], "MUG%012u", rand_serial);
}

/**
 * Say that we want to allow EV_KEY events and that we want to allow KEY_PROG1 events specifically
 */
static int razer_setup_input(struct input_dev *input, struct hid_device *hdev)
{
    __set_bit(EV_KEY, input->evbit);
    __set_bit(KEY_PROG1, input->keybit);

    return 0;
}

/**
 * Setup the input device now that its been added to our struct
 */
static int razer_input_configured(struct hid_device *hdev, struct hid_input *hi)
{
    struct razer_accessory_device *device = hid_get_drvdata(hdev);
    int ret;

    ret = razer_setup_input(device->input, hdev);
    if (ret) {
        hid_err(hdev, "magicmouse setup input failed (%d)\n", ret);
        /* clean msc->input to notify probe() of the failure */
        device->input = NULL;
        return ret;
    }

    return 0;
}

/**
 * Basically map a hid input to our structure
 */
static int razer_input_mapping(struct hid_device *hdev, struct hid_input *hi, struct hid_field *field,    struct hid_usage *usage, unsigned long **bit, int *max)
{
    struct razer_accessory_device *device = hid_get_drvdata(hdev);

    if (!device->input)
        device->input = hi->input;

    return 0;
}

/**
 * Match method checks whether this driver should be used for a given HID device
 */
static bool razer_accessory_match(struct hid_device *hdev, bool ignore_special_driver)
{
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        if (intf->cur_altsetting->desc.bInterfaceNumber != 0) {
            dev_info(&intf->dev, "skipping secondary interface\n");
            return false;
        }
    }

    return true;
}

/**
 * Probe method is ran whenever a device is binded to the driver
 */
static int razer_accessory_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    unsigned char expected_protocol = USB_INTERFACE_PROTOCOL_MOUSE;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_accessory_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_accessory_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        return -ENOMEM;
    }

    // Init data
    razer_accessory_init(dev, intf, hdev);

    switch(usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        expected_protocol = 0;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        expected_protocol = USB_INTERFACE_PROTOCOL_MOUSE;
        break;

    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        expected_protocol = USB_INTERFACE_PROTOCOL_KEYBOARD;
        break;
    }

    if(dev->usb_interface_protocol == expected_protocol) {
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);                               // Get driver version
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_test);                                  // Test mode
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);                           // Get string of device mode
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version

        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);                   // Custom effect frame
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);                    // No effect
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);                  // Static effect
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);                  // Breathing effect
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);                  // Custom effect
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);                     // Brightness

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
            // Razer has also added a "Fast Wave" effect for at least this device
            // which uses the same effect command but a speed parameter of 0x10.
            // It has not been implemented.
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_led_brightness);           // Charging effects
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charging_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fast_charging_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_matrix_effect_wave);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_fully_charged_matrix_effect_none);
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
            // Device initial brightness is always 100% anyway
            dev->saved_brightness = 0xFF;
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
        case USB_DEVICE_ID_RAZER_RAPTOR_27:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);            // Spectrum effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        case USB_DEVICE_ID_RAZER_RAPTOR_27:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_wave);                // Wave effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_reactive);            // Reactive
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_reactive_trigger);           // Reactive trigger
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_is_mug_present);                    // Is cup present
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_blinking);            // Blinking effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_starlight);
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_reset_channels);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel1_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel2_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel3_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel4_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel5_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel6_size);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel1_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel2_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel3_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel4_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel5_led_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_channel6_led_brightness);
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        // Needs to be in "Normal" mode for idle effects to function properly
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
            break;

        default:
            // Needs to be in "Driver" mode just to function
            razer_set_device_mode(dev, 0x03, 0x00);
            break;
        }
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

    usb_disable_autosuspend(usb_dev);

    return 0;

exit_free:
    kfree(dev);
    return retval;
}

/**
 * Unbind function
 */
static void razer_accessory_disconnect(struct hid_device *hdev)
{
    unsigned char expected_protocol = USB_INTERFACE_PROTOCOL_MOUSE;
    struct razer_accessory_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);

    switch(usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CORE:
    case USB_DEVICE_ID_RAZER_FIREFLY_V2:
    case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
    case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
    case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
    case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
    case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
    case USB_DEVICE_ID_RAZER_RAPTOR_27:
    case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        expected_protocol = 0;
        break;

    case USB_DEVICE_ID_RAZER_FIREFLY:
    case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
    case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
    case USB_DEVICE_ID_RAZER_CHROMA_MUG:
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
    case USB_DEVICE_ID_RAZER_CHROMA_BASE:
    case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
        expected_protocol = USB_INTERFACE_PROTOCOL_MOUSE;
        break;

    case USB_DEVICE_ID_RAZER_NOMMO_PRO:
    case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
    case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
    case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        expected_protocol = USB_INTERFACE_PROTOCOL_KEYBOARD;
        break;
    }

    if(dev->usb_interface_protocol == expected_protocol) {
        device_remove_file(&hdev->dev, &dev_attr_version);                               // Get driver version
        device_remove_file(&hdev->dev, &dev_attr_test);                                  // Test mode
        device_remove_file(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        device_remove_file(&hdev->dev, &dev_attr_device_mode);                           // Get string of device mode
        device_remove_file(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
        device_remove_file(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version

        device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);                   // Custom effect frame
        device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);                    // No effect
        device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);                  // Static effect
        device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);                  // Breathing effect
        device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);                  // Custom effect
        device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);                     // Brightness

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_charging_led_brightness);           // Charging effects
            device_remove_file(&hdev->dev, &dev_attr_charging_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_charging_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_charging_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_charging_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_charging_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_fast_charging_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_matrix_effect_wave);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_fully_charged_matrix_effect_none);
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        case USB_DEVICE_ID_RAZER_MOUSE_DOCK:
        case USB_DEVICE_ID_RAZER_RAPTOR_27:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);            // Spectrum effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        case USB_DEVICE_ID_RAZER_CHROMA_BASE:
        case USB_DEVICE_ID_RAZER_NOMMO_PRO:
        case USB_DEVICE_ID_RAZER_NOMMO_CHROMA:
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
        case USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA:
        case USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA:
        case USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA:
        case USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA:
        case USB_DEVICE_ID_RAZER_RAPTOR_27:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_wave);                // Wave effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX:
        case USB_DEVICE_ID_RAZER_FIREFLY_V2:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA:
        case USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED:
        case USB_DEVICE_ID_RAZER_FIREFLY:
        case USB_DEVICE_ID_RAZER_CORE:
        case USB_DEVICE_ID_RAZER_CORE_X_CHROMA:
        case USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_reactive);            // Reactive
            device_remove_file(&hdev->dev, &dev_attr_matrix_reactive_trigger);           // Reactive trigger
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_MUG:
            device_remove_file(&hdev->dev, &dev_attr_is_mug_present);                    // Is cup present
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_blinking);            // Blinking effect
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_starlight);
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER:
            device_remove_file(&hdev->dev, &dev_attr_reset_channels);
            device_remove_file(&hdev->dev, &dev_attr_channel1_size);
            device_remove_file(&hdev->dev, &dev_attr_channel2_size);
            device_remove_file(&hdev->dev, &dev_attr_channel3_size);
            device_remove_file(&hdev->dev, &dev_attr_channel4_size);
            device_remove_file(&hdev->dev, &dev_attr_channel5_size);
            device_remove_file(&hdev->dev, &dev_attr_channel6_size);
            device_remove_file(&hdev->dev, &dev_attr_channel1_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_channel2_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_channel3_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_channel4_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_channel5_led_brightness);
            device_remove_file(&hdev->dev, &dev_attr_channel6_led_brightness);
            break;
        }
    }

    hid_hw_stop(hdev);

    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

/**
 * Converts interrupt event into PROG1 keypress
 *
 * Checks if we get the event were after.
 * Creates a keypress event of KEY_PROG1
 *
 * input_report_key generates an event
 * input_sync says were finished, all events are complete. Is useful when setting up other events as they might take multiple statements to complete an event like relative events
 *
 * data[1] == 0xa0 if mug is present
 */
static int razer_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
    struct razer_accessory_device *device = hid_get_drvdata(hdev);

    if(size == 16 && data[0] == 0x04) {
        input_report_key(device->input, KEY_PROG1, 0x01);
        input_report_key(device->input, KEY_PROG1, 0x00);
        input_sync(device->input);
        return 1;
    }

    return 0;
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_FIREFLY) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_FIREFLY_HYPERFLUX) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_FIREFLY_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_GOLIATHUS_CHROMA_EXTENDED) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CORE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CORE_X_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHROMA_MUG) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHROMA_HDK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHROMA_BASE) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NOMMO_PRO) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_NOMMO_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN_KITTY_EDITION) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHROMA_ADDRESSABLE_RGB_CONTROLLER) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MOUSE_BUNGEE_V3_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_THUNDERBOLT_4_DOCK_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BASE_STATION_V2_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHARGING_PAD_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_MOUSE_DOCK) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_RAPTOR_27) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_LAPTOP_STAND_CHROMA_V2) },
    { 0 }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_accessory_driver = {
    .name = "razeraccessory",
    .id_table = razer_devices,
    .match = razer_accessory_match,
    .probe = razer_accessory_probe,
    .remove = razer_accessory_disconnect,
    .raw_event = razer_raw_event,
    .input_mapping = razer_input_mapping,
    .input_configured = razer_input_configured
};

module_hid_driver(razer_accessory_driver);
