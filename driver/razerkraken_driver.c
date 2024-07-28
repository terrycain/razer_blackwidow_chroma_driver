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
#include <linux/random.h>

#include "razerkraken_driver.h"
#include "razercommon.h"

/*
 * Version Information
 */
#define DRIVER_DESC "Razer Keyboard Device Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

/**
 * Print report to syslog
 */
/*
static void print_erroneous_kraken_request_report(struct razer_kraken_request_report* report, char* driver_name, char* message)
{
    printk(KERN_WARNING "%s: %s. Report ID: %02x dest: %02x length: %02x ADDR: %02x%02x Args: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x .\n",
           driver_name,
           message,
           report->report_id,
           report->destination,
           report->length,
           report->addr_h,
           report->addr_l,
           report->arguments[0], report->arguments[1], report->arguments[2], report->arguments[3], report->arguments[4], report->arguments[5],
           report->arguments[6], report->arguments[7], report->arguments[8], report->arguments[9], report->arguments[10], report->arguments[11],
           report->arguments[12], report->arguments[13], report->arguments[14], report->arguments[15]);
}
*/

static int razer_kraken_send_control_msg(struct usb_device *usb_dev,struct razer_kraken_request_report* report, unsigned char skip)
{
    uint request = HID_REQ_SET_REPORT; // 0x09
    uint request_type = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT; // 0x21
    uint value = 0x0204;
    uint index = 0x0003;
    uint size = 37;
    char *buf;
    int len;

    buf = kmemdup(report, size, GFP_KERNEL);
    if (buf == NULL)
        return -ENOMEM;

    // Send usb control message
    len = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
                          request,      // Request      U8
                          request_type, // RequestType  U8
                          value,        // Value        U16
                          index,        // Index        U16
                          buf,          // Data         void* data
                          size,         // Length       U16
                          USB_CTRL_SET_TIMEOUT); //     Int

    // Wait
    if(skip != 1) {
        msleep(report->length * 15);
    }

    kfree(buf);
    if(len!=size)
        printk(KERN_WARNING "razer driver: Device data transfer failed.\n");

    return ((len < 0) ? len : ((len != size) ? -EIO : 0));
}

/**
 * Get a request report
 *
 * report_id - The type of report
 * destination - where data is going (like ram)
 * length - amount of data
 * address - where to write data to
 */
static struct razer_kraken_request_report get_kraken_request_report(unsigned char report_id, unsigned char destination, unsigned char length, unsigned short address)
{
    struct razer_kraken_request_report report;
    memset(&report, 0, sizeof(struct razer_kraken_request_report));

    report.report_id = report_id;
    report.destination = destination;
    report.length = length;
    report.addr_h = (address >> 8);
    report.addr_l = (address & 0xFF);

    return report;
}

/**
 * Get a union containing the effect bitfield
 */
static union razer_kraken_effect_byte get_kraken_effect_byte(void)
{
    union razer_kraken_effect_byte effect_byte;
    memset(&effect_byte, 0, sizeof(union razer_kraken_effect_byte));

    return effect_byte;
}

/**
 * Get the current effect
 */
static unsigned char get_current_effect(struct device *dev)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x00, 0x01, device->led_mode_address);
    int is_mutex_locked = mutex_is_locked(&device->lock);
    unsigned char result = 0;

    // Lock if there isn't already a lock, otherwise skip, essentially emulate a rentrant lock
    if(is_mutex_locked == 0) {
        mutex_lock(&device->lock);
    }

    device->data[0] = 0x00;
    razer_kraken_send_control_msg(device->usb_dev, &report, 1);
    msleep(25); // Sleep 20ms

    // Check for actual data
    if(device->data[0] == 0x05) {
        result = device->data[1];
    } else {
        printk(KERN_CRIT "razerkraken: Did not manage to get report\n");
    }

    // Unlock if there isn't already a lock (as there would be by now), otherwise skip as reusing existing lock
    if(is_mutex_locked == 0) {
        mutex_unlock(&device->lock);
    }

    return result;
}

static unsigned int get_rgb_from_addr(struct device *dev, unsigned short address, unsigned char len, char* buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x00, len, address);
    int is_mutex_locked = mutex_is_locked(&device->lock);
    unsigned char written = 0;

    // Lock if there isn't already a lock, otherwise skip, essentially emulate a rentrant lock
    if(is_mutex_locked == 0) {
        mutex_lock(&device->lock);
    }

    device->data[0] = 0x00;
    razer_kraken_send_control_msg(device->usb_dev, &report, 1);
    msleep(25); // Sleep 20ms

    // Check for actual data
    if(device->data[0] == 0x05) {
        //printk(KERN_CRIT "razerkraken: Got %02x%02x%02x %02x\n", device->data[1], device->data[2], device->data[3], device->data[4]);
        memcpy(&buf[0], &device->data[1], len);
        written = len;
    } else {
        printk(KERN_CRIT "razerkraken: Did not manage to get report\n");
    }

    // Unlock if there isn't already a lock (as there would be by now), otherwise skip as reusing existing lock
    if(is_mutex_locked == 0) {
        mutex_unlock(&device->lock);
    }

    return written;
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
    struct razer_kraken_device *device = dev_get_drvdata(dev);

    char *device_type;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC:
    case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC_ALT:
        device_type = "Razer Kraken 7.1\n";
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN:
        device_type = "Razer Kraken 7.1 Chroma\n"; // Rainie
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_V2:
        device_type = "Razer Kraken 7.1 V2\n"; // Kylie
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
        device_type = "Razer Kraken Ultimate\n";
        break;

    default:
        device_type = "Unknown Device\n";
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
 * Specrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x40, 0x01, device->led_mode_address);
    union razer_kraken_effect_byte effect_byte = get_kraken_effect_byte();

    // Spectrum Cycling | ON
    effect_byte.bits.on_off_static = 1;
    effect_byte.bits.spectrum_cycling = 1;

    report.arguments[0] = effect_byte.value;

    // Lock access to sending USB as adhering to the razer len*15ms delay
    mutex_lock(&device->lock);
    razer_kraken_send_control_msg(device->usb_dev, &report, 0);
    mutex_unlock(&device->lock);

    return count;
}

/**
 * Write device file "mode_none"
 *
 * None effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_matrix_effect_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x40, 0x01, device->led_mode_address);
    union razer_kraken_effect_byte effect_byte = get_kraken_effect_byte();

    // Spectrum Cycling | OFF
    effect_byte.bits.on_off_static = 0;
    effect_byte.bits.spectrum_cycling = 0;

    report.arguments[0] = effect_byte.value;

    // Lock access to sending USB as adhering to the razer len*15ms delay
    mutex_lock(&device->lock);
    razer_kraken_send_control_msg(device->usb_dev, &report, 0);
    mutex_unlock(&device->lock);

    return count;
}

/**
 * Write device file "mode_static"
 *
 * Static effect mode is activated whenever the file is written to with 3 bytes
 */
static ssize_t razer_attr_write_matrix_effect_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report rgb_report = get_kraken_request_report(0x04, 0x40, count, device->breathing_address[0]);
    struct razer_kraken_request_report effect_report = get_kraken_request_report(0x04, 0x40, 0x01, device->led_mode_address);
    union razer_kraken_effect_byte effect_byte = get_kraken_effect_byte();

    if (count != 3 && count != 4) {
        printk(KERN_WARNING "razerkraken: Static mode only accepts RGB (3byte) or RGB with intensity (4byte)\n");
        return -EINVAL;
    }

    rgb_report.arguments[0] = buf[0];
    rgb_report.arguments[1] = buf[1];
    rgb_report.arguments[2] = buf[2];

    if(count == 4) {
        rgb_report.arguments[3] = buf[3];
    }

    // ON/Static
    effect_byte.bits.on_off_static = 1;
    effect_report.arguments[0] = effect_byte.value;

    // Lock sending of the 2 commands
    mutex_lock(&device->lock);

    // Basically Kraken Classic doesn't take RGB arguments so only do it for the KrakenV1,V2,Ultimate
    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN:
    case USB_DEVICE_ID_RAZER_KRAKEN_V2:
    case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
        razer_kraken_send_control_msg(device->usb_dev, &rgb_report, 0);
        break;
    }

    // Send Set static command
    razer_kraken_send_control_msg(device->usb_dev, &effect_report, 0);
    mutex_unlock(&device->lock);

    return count;
}

/**
 * Write device file "mode_custom"
 *
 * Custom effect mode is activated whenever the file is written to with 3 bytes
 */
static ssize_t razer_attr_write_matrix_effect_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report rgb_report = get_kraken_request_report(0x04, 0x40, count, device->custom_address);
    struct razer_kraken_request_report effect_report = get_kraken_request_report(0x04, 0x40, 0x01, device->led_mode_address);
    union razer_kraken_effect_byte effect_byte = get_kraken_effect_byte();

    if(count != 3 && count != 4) {
        printk(KERN_WARNING "razerkraken: Custom mode only accepts RGB (3byte) or RGB with intensity (4byte)\n");
        return -EINVAL;
    }

    rgb_report.arguments[0] = buf[0];
    rgb_report.arguments[1] = buf[1];
    rgb_report.arguments[2] = buf[2];

    if(count == 4) {
        rgb_report.arguments[3] = buf[3];
    }

    // ON/Static
    effect_byte.bits.on_off_static = 1;
    effect_report.arguments[0] = 1; //effect_byte.value;

    // Lock sending of the 2 commands
    mutex_lock(&device->lock);
    razer_kraken_send_control_msg(device->usb_dev, &rgb_report, 1);

    razer_kraken_send_control_msg(device->usb_dev, &effect_report, 1);
    mutex_unlock(&device->lock);

    return count;
}

/**
 * Read device file "mode_static"
 *
 * Returns 4 bytes for config
 */
static ssize_t razer_attr_read_matrix_effect_static(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    return get_rgb_from_addr(dev, device->breathing_address[0], 0x04, buf);
}

/**
 * Read device file "mode_custom"
 *
 * Returns 4 bytes for config
 */
static ssize_t razer_attr_read_matrix_effect_custom(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    return get_rgb_from_addr(dev, device->custom_address, 0x04, buf);
}

/**
 * Write device file "mode_breath"
 *
 * Breathing effect mode is activated whenever the file is written to with 3,6 or 9 bytes
 */
static ssize_t razer_attr_write_matrix_effect_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report effect_report = get_kraken_request_report(0x04, 0x40, 0x01, device->led_mode_address);
    union razer_kraken_effect_byte effect_byte = get_kraken_effect_byte();

    // Short circuit here as rainie only does breathing1
    if(device->usb_pid == USB_DEVICE_ID_RAZER_KRAKEN && count != 3) {
        printk(KERN_WARNING "razerkraken: Breathing mode only accepts RGB (3byte)\n");
        return -EINVAL;
    }

    if(count == 3) {
        struct razer_kraken_request_report rgb_report = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[0]);

        rgb_report.arguments[0] = buf[0];
        rgb_report.arguments[1] = buf[1];
        rgb_report.arguments[2] = buf[2];

        // ON/Static
        effect_byte.bits.on_off_static = 1;
        effect_byte.bits.single_colour_breathing = 1;
        effect_byte.bits.sync = 1;
        effect_report.arguments[0] = effect_byte.value;

        // Lock sending of the 2 commands
        mutex_lock(&device->lock);
        razer_kraken_send_control_msg(device->usb_dev, &rgb_report, 0);

        razer_kraken_send_control_msg(device->usb_dev, &effect_report, 0);
        mutex_unlock(&device->lock);
    } else if(count == 6) {
        struct razer_kraken_request_report rgb_report  = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[1]);
        struct razer_kraken_request_report rgb_report2 = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[1]+4); // Address the 2nd set of colours

        rgb_report.arguments[0] = buf[0];
        rgb_report.arguments[1] = buf[1];
        rgb_report.arguments[2] = buf[2];
        rgb_report2.arguments[0] = buf[3];
        rgb_report2.arguments[1] = buf[4];
        rgb_report2.arguments[2] = buf[5];

        // ON/Static
        effect_byte.bits.on_off_static = 1;
        effect_byte.bits.two_colour_breathing = 1;
        effect_byte.bits.sync = 1;
        effect_report.arguments[0] = effect_byte.value;

        // Lock sending of the 2 commands
        mutex_lock(&device->lock);
        razer_kraken_send_control_msg(device->usb_dev, &rgb_report, 0);

        razer_kraken_send_control_msg(device->usb_dev, &rgb_report2, 0);

        razer_kraken_send_control_msg(device->usb_dev, &effect_report, 0);
        mutex_unlock(&device->lock);

    } else if(count == 9) {
        struct razer_kraken_request_report rgb_report  = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[2]);
        struct razer_kraken_request_report rgb_report2 = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[2]+4); // Address the 2nd set of colours
        struct razer_kraken_request_report rgb_report3 = get_kraken_request_report(0x04, 0x40, 0x03, device->breathing_address[2]+8); // Address the 3rd set of colours

        rgb_report.arguments[0] = buf[0];
        rgb_report.arguments[1] = buf[1];
        rgb_report.arguments[2] = buf[2];
        rgb_report2.arguments[0] = buf[3];
        rgb_report2.arguments[1] = buf[4];
        rgb_report2.arguments[2] = buf[5];
        rgb_report3.arguments[0] = buf[6];
        rgb_report3.arguments[1] = buf[7];
        rgb_report3.arguments[2] = buf[8];

        // ON/Static
        effect_byte.bits.on_off_static = 1;
        effect_byte.bits.three_colour_breathing = 1;
        effect_byte.bits.sync = 1;
        effect_report.arguments[0] = effect_byte.value;

        // Lock sending of the 2 commands
        mutex_lock(&device->lock);
        razer_kraken_send_control_msg(device->usb_dev, &rgb_report, 0);

        razer_kraken_send_control_msg(device->usb_dev, &rgb_report2, 0);

        razer_kraken_send_control_msg(device->usb_dev, &rgb_report3, 0);

        razer_kraken_send_control_msg(device->usb_dev, &effect_report, 0);
        mutex_unlock(&device->lock);

    } else {
        printk(KERN_WARNING "razerkraken: Breathing mode only accepts RGB (3byte), RGB RGB (6byte) or RGB RGB RGB (9byte)\n");
        return -EINVAL;
    }

    return count;
}

/**
 * Read device file "mode_breath"
 *
 * Returns 4, 8, 12 bytes for config
 */
static ssize_t razer_attr_read_matrix_effect_breath(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    union razer_kraken_effect_byte effect_byte;
    unsigned char num_colours = 1;

    effect_byte.value = get_current_effect(dev);

    if(effect_byte.bits.two_colour_breathing == 1) {
        num_colours = 2;
    } else if(effect_byte.bits.three_colour_breathing == 1) {
        num_colours = 3;
    }

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_V2:
    case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
        switch(num_colours) {
        case 3:
            return get_rgb_from_addr(dev, device->breathing_address[2], 0x0C, buf);
            break;
        case 2:
            return get_rgb_from_addr(dev, device->breathing_address[1], 0x08, buf);
            break;
        default:
            return get_rgb_from_addr(dev, device->breathing_address[0], 0x04, buf);
            break;
        }
        break;

    case USB_DEVICE_ID_RAZER_KRAKEN:
    default:
        return get_rgb_from_addr(dev, device->breathing_address[0], 0x04, buf);
        break;
    }
}

/**
 * Read device file "serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x20, 0x16, 0x7f00);

    // Basically some simple caching
    // Also skips going to device if it doesn't contain the serial
    if(device->serial[0] == '\0') {

        mutex_lock(&device->lock);
        device->data[0] = 0x00;
        razer_kraken_send_control_msg(device->usb_dev, &report, 1);
        msleep(25); // Sleep 20ms

        // Check for actual data
        if(device->data[0] == 0x05) {
            // Serial is present
            memcpy(&device->serial[0], &device->data[1], 22);
            device->serial[22] = '\0';
        } else {
            printk(KERN_CRIT "razerkraken: Did not manage to get serial from device, using XX01 instead\n");
            device->serial[0] = 'X';
            device->serial[1] = 'X';
            device->serial[2] = '0';
            device->serial[3] = '1';
            device->serial[4] = '\0';
        }
        mutex_unlock(&device->lock);

    }

    return sprintf(buf, "%s\n", &device->serial[0]);
}

/**
 * Read device file "get_firmware_version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_firmware_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_kraken_device *device = dev_get_drvdata(dev);
    struct razer_kraken_request_report report = get_kraken_request_report(0x04, 0x20, 0x02, 0x0030);

    // Basically some simple caching
    if(device->firmware_version[0] != 1) {

        mutex_lock(&device->lock);
        device->data[0] = 0x00;
        razer_kraken_send_control_msg(device->usb_dev, &report, 1);
        msleep(25); // Sleep 20ms

        // Check for actual data
        if(device->data[0] == 0x05) {
            // Serial is present
            device->firmware_version[0] = 1;
            device->firmware_version[1] = device->data[1];
            device->firmware_version[2] = device->data[2];
        } else {
            printk(KERN_CRIT "razerkraken: Did not manage to get firmware version from device, using v9.99 instead\n");
            device->firmware_version[0] = 1;
            device->firmware_version[1] = 0x09;
            device->firmware_version[2] = 0x99;
        }
        mutex_unlock(&device->lock);
    }

    return sprintf(buf, "v%x.%x\n", device->firmware_version[1], device->firmware_version[2]);
}

/**
 * Read device file "matrix_current_effect"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_matrix_current_effect(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned char current_effect = get_current_effect(dev);

    return sprintf(buf, "%02x\n", current_effect);
}

/**
 * Write device file "device_mode"
 */
static ssize_t razer_attr_write_device_mode(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return count;
}

/**
 * Read device file "device_mode"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    buf[0] = 0x00;
    buf[1] = 0x00;

    return 2;
}

/**
 * Set up the device driver files

 *
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */

static DEVICE_ATTR(test,                    0660, razer_attr_read_test,                       razer_attr_write_test);
static DEVICE_ATTR(version,                 0440, razer_attr_read_version,                    NULL);
static DEVICE_ATTR(device_type,             0440, razer_attr_read_device_type,                NULL);
static DEVICE_ATTR(device_serial,           0440, razer_attr_read_device_serial,              NULL);
static DEVICE_ATTR(device_mode,             0660, razer_attr_read_device_mode,                razer_attr_write_device_mode);
static DEVICE_ATTR(firmware_version,        0440, razer_attr_read_firmware_version,           NULL);

static DEVICE_ATTR(matrix_current_effect,   0440, razer_attr_read_matrix_current_effect,      NULL);
static DEVICE_ATTR(matrix_effect_none,      0220, NULL,                                       razer_attr_write_matrix_effect_none);
static DEVICE_ATTR(matrix_effect_spectrum,  0220, NULL,                                       razer_attr_write_matrix_effect_spectrum);
static DEVICE_ATTR(matrix_effect_static,    0660, razer_attr_read_matrix_effect_static,       razer_attr_write_matrix_effect_static);
static DEVICE_ATTR(matrix_effect_custom,    0660, razer_attr_read_matrix_effect_custom,       razer_attr_write_matrix_effect_custom);
static DEVICE_ATTR(matrix_effect_breath,    0660, razer_attr_read_matrix_effect_breath,       razer_attr_write_matrix_effect_breath);

static void razer_kraken_init(struct razer_kraken_device *dev, struct usb_interface *intf)
{
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    unsigned int rand_serial = 0;

    // Initialise mutex
    mutex_init(&dev->lock);
    // Setup values
    dev->usb_dev = usb_dev;
    dev->usb_interface_protocol = intf->cur_altsetting->desc.bInterfaceProtocol;
    dev->usb_vid = usb_dev->descriptor.idVendor;
    dev->usb_pid = usb_dev->descriptor.idProduct;

    switch(dev->usb_pid) {
    case USB_DEVICE_ID_RAZER_KRAKEN_V2:
    case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
        dev->led_mode_address = KYLIE_SET_LED_ADDRESS;
        dev->custom_address = KYLIE_CUSTOM_ADDRESS_START;
        dev->breathing_address[0] = KYLIE_BREATHING1_ADDRESS_START;
        dev->breathing_address[1] = KYLIE_BREATHING2_ADDRESS_START;
        dev->breathing_address[2] = KYLIE_BREATHING3_ADDRESS_START;
        break;
    case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC:
    case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC_ALT:
    case USB_DEVICE_ID_RAZER_KRAKEN:
        dev->led_mode_address = RAINIE_SET_LED_ADDRESS;
        dev->custom_address = RAINIE_CUSTOM_ADDRESS_START;
        dev->breathing_address[0] = RAINIE_BREATHING1_ADDRESS_START;

        // Get a "random" integer
        get_random_bytes(&rand_serial, sizeof(unsigned int));
        sprintf(&dev->serial[0], "HN%015u", rand_serial);
        break;
    }
}

/**
 * Probe method is ran whenever a device is binded to the driver
 */
static int razer_kraken_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_kraken_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_kraken_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        return -ENOMEM;
    }

    // Init data
    razer_kraken_init(dev, intf);

    if(dev->usb_interface_protocol == USB_INTERFACE_PROTOCOL_NONE) {
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);                               // Get driver version
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_test);                                  // Test mode
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version
        CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);                           // Get device mode

        switch(dev->usb_pid) {
        case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC:
        case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC_ALT:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_current_effect);         // Get current effect
            break;
        case USB_DEVICE_ID_RAZER_KRAKEN:
        case USB_DEVICE_ID_RAZER_KRAKEN_V2:
        case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_current_effect);         // Get current effect
            break;
        }
    }

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
static void razer_kraken_disconnect(struct hid_device *hdev)
{
    struct razer_kraken_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);

    dev = hid_get_drvdata(hdev);

    if(dev->usb_interface_protocol == USB_INTERFACE_PROTOCOL_NONE) {
        device_remove_file(&hdev->dev, &dev_attr_version);                               // Get driver version
        device_remove_file(&hdev->dev, &dev_attr_test);                                  // Test mode
        device_remove_file(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
        device_remove_file(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
        device_remove_file(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version
        device_remove_file(&hdev->dev, &dev_attr_device_mode);                           // Get device mode

        switch(dev->usb_pid) {
        case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC:
        case USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC_ALT:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_current_effect);         // Get current effect
            break;

        case USB_DEVICE_ID_RAZER_KRAKEN:
        case USB_DEVICE_ID_RAZER_KRAKEN_V2:
        case USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE:
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);            // No effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);        // Spectrum effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);          // Static effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);          // Custom effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);          // Breathing effect
            device_remove_file(&hdev->dev, &dev_attr_matrix_current_effect);         // Get current effect
            break;
        }
    }

    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

static int razer_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
    struct razer_kraken_device *device = dev_get_drvdata(&hdev->dev);

    //printk(KERN_WARNING "razerkraken: Got raw message %d\n", size);

    if(size == 33) { // Should be a response to a Control packet
        memcpy(&device->data[0], &data[0], size);

    } else {
        printk(KERN_WARNING "razerkraken: Got raw message, length: %d\n", size);
    }

    return 0;
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN_CLASSIC_ALT) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN_V2) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_KRAKEN_ULTIMATE) },
    { 0 }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_kraken_driver = {
    .name = "razerkraken",
    .id_table = razer_devices,
    .probe = razer_kraken_probe,
    .remove = razer_kraken_disconnect,
    .raw_event = razer_raw_event
};

module_hid_driver(razer_kraken_driver);
