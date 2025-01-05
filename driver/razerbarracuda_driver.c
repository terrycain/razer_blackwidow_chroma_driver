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

#include "razerbarracuda_driver.h"
#include "razercommon.h"
#include "razerchromacommon.h"

/*
 * Version Information
 */
#define DRIVER_DESC "Razer Barracuda Device Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

/**
 * Print report to syslog
 */
/*
static void print_erroneous_barracuda_request_report(struct razer_barracuda_request_report* report, char* driver_name, char* message)
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

static int razer_barracuda_send_control_msg(struct usb_device *usb_dev,struct razer_barracuda_request_report* report, unsigned char skip)
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
static struct razer_barracuda_request_report get_barracuda_request_report(unsigned char report_id, unsigned char destination, unsigned char length, unsigned short address)
{
    struct razer_barracuda_request_report report;
    memset(&report, 0, sizeof(struct razer_barracuda_request_report));

    report.report_id = report_id;
    report.destination = destination;
    report.length = length;
    report.addr_h = (address >> 8);
    report.addr_l = (address & 0xFF);

    return report;
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
    struct razer_barracuda_device *device = dev_get_drvdata(dev);

    char *device_type;

    switch (device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BARRACUDA:
        device_type = "Razer Barracuda\n";
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
 * Read device file "serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
    struct razer_barracuda_request_report report = get_barracuda_request_report(0x04, 0x20, 0x16, 0x7f00);

    // Basically some simple caching
    // Also skips going to device if it doesn't contain the serial
    if(device->serial[0] == '\0') {

        mutex_lock(&device->lock);
        device->data[0] = 0x00;
        razer_barracuda_send_control_msg(device->usb_dev, &report, 1);
        msleep(25); // Sleep 20ms

        // Check for actual data
        if(device->data[0] == 0x05) {
            // Serial is present
            memcpy(&device->serial[0], &device->data[1], 22);
            device->serial[22] = '\0';
        } else {
            printk(KERN_CRIT "razerbarracuda: Did not manage to get serial from device, using XX01 instead\n");
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
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
    struct razer_barracuda_request_report report = get_barracuda_request_report(0x04, 0x20, 0x02, 0x0030);

    // Basically some simple caching
    if(device->firmware_version[0] != 1) {

        mutex_lock(&device->lock);
        device->data[0] = 0x00;
        razer_barracuda_send_control_msg(device->usb_dev, &report, 1);
        msleep(25); // Sleep 20ms

        // Check for actual data
        if(device->data[0] == 0x05) {
            // Serial is present
            device->firmware_version[0] = 1;
            device->firmware_version[1] = device->data[1];
            device->firmware_version[2] = device->data[2];
        } else {
            printk(KERN_CRIT "razerbarracuda: Did not manage to get firmware version from device, using v9.99 instead\n");
            device->firmware_version[0] = 1;
            device->firmware_version[1] = 0x09;
            device->firmware_version[2] = 0x99;
        }
        mutex_unlock(&device->lock);
    }

    return sprintf(buf, "v%x.%x\n", device->firmware_version[1], device->firmware_version[2]);
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
 * Get request/response indices and timing parameters for the device
 */
static void razer_get_report_params(struct usb_device *usb_dev, uint *report_index, uint *response_index, ulong *wait_min, ulong *wait_max)
{
    switch (usb_dev->descriptor.idProduct) {
    default:
        *report_index = 0x01;
        *response_index = 0x01;
        *wait_min = 600;
        *wait_max = 800;
        break;
    }
}

/**
 * Send report to the headset
 */
static int razer_get_report(struct usb_device *usb_dev, struct razer_report *request, struct razer_report *response)
{
    uint report_index, response_index;
    ulong wait_min, wait_max;
    razer_get_report_params(usb_dev, &report_index, &response_index, &wait_min, &wait_max);
    return razer_get_usb_response(usb_dev, report_index, request, response_index, response, wait_min, wait_max);
}

/**
 * Function to send to device, get response, and actually check the response
 */
static int razer_send_payload(struct razer_barracuda_device *device, struct razer_report *request, struct razer_report *response)
{
    int err;

    request->crc = razer_calculate_crc(request);

    mutex_lock(&device->lock);
    err = razer_get_report(device->usb_dev, request, response);
    mutex_unlock(&device->lock);
    if (err) {
        print_erroneous_report(response, "razerbarracuda", "Invalid Report Length");
        return err;
    }

    /* Check the packet number, class and command are the same */
    if (response->remaining_packets != request->remaining_packets ||
        response->command_class != request->command_class ||
        response->command_id.id != request->command_id.id) {
        print_erroneous_report(response, "razerbarracuda", "Response doesn't match request");
        return -EIO;
    }

    switch (response->status) {
    case RAZER_CMD_BUSY:
        // TODO: Check if this should be an error.
        // print_erroneous_report(&response, "razerbarracuda", "Device is busy");
        break;
    case RAZER_CMD_FAILURE:
        print_erroneous_report(response, "razerbarracuda", "Command failed");
        return -EIO;
    case RAZER_CMD_NOT_SUPPORTED:
        print_erroneous_report(response, "razerbarracuda", "Command not supported");
        return -EIO;
    case RAZER_CMD_TIMEOUT:
        print_erroneous_report(response, "razerbarracuda", "Command timed out");
        return -EIO;
    }

    return 0;
}

/**
 * Read device file "charge_level"
 *
 * Returns an integer which needs to be scaled from 0-255 -> 0-100
 */
static ssize_t razer_attr_read_charge_level(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
    struct razer_report request = razer_chroma_misc_get_battery_level();
    struct razer_report response = {0};

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BARRACUDA:
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
 * Read device file "charge_status"
 *
 * Returns 0 when not charging, 1 when charging
 */
static ssize_t razer_attr_read_charge_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
    struct razer_report request = razer_chroma_misc_get_charging_status();
    struct razer_report response = {0};

    switch(device->usb_pid) {
    case USB_DEVICE_ID_RAZER_BARRACUDA:
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
 * Read device file "charge_low_threshold"
 */
static ssize_t razer_attr_read_charge_low_threshold(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
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
    struct razer_barracuda_device *device = dev_get_drvdata(dev);
    unsigned char threshold = (unsigned char)simple_strtoul(buf, NULL, 10);
    struct razer_report request = {0};
    struct razer_report response = {0};

    request = razer_chroma_misc_set_low_battery_threshold(threshold);
    request.transaction_id.id = 0xFF;

    razer_send_payload(device, &request, &response);
    return count;
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

static DEVICE_ATTR(charge_level,            0440, razer_attr_read_charge_level,               NULL);
static DEVICE_ATTR(charge_status,           0440, razer_attr_read_charge_status,              NULL);
static DEVICE_ATTR(charge_low_threshold,    0660, razer_attr_read_charge_low_threshold,       razer_attr_write_charge_low_threshold);


static void razer_barracuda_init(struct razer_barracuda_device *dev, struct usb_interface *intf)
{
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    // Initialise mutex
    mutex_init(&dev->lock);
    // Setup values
    dev->usb_dev = usb_dev;
    dev->usb_interface_protocol = intf->cur_altsetting->desc.bInterfaceProtocol;
    dev->usb_vid = usb_dev->descriptor.idVendor;
    dev->usb_pid = usb_dev->descriptor.idProduct;
}

/**
 * Probe method is ran whenever a device is binded to the driver
 */
static int razer_barracuda_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_barracuda_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_barracuda_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        return -ENOMEM;
    }

    // Init data
    razer_barracuda_init(dev, intf);

    if(dev->usb_interface_protocol == USB_INTERFACE_PROTOCOL_NONE) {
        switch(dev->usb_pid) {
        case USB_DEVICE_ID_RAZER_BARRACUDA:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);                               // Get driver version
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_test);                                  // Test mode
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);                           // Get device mode
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_level);                          // Charge level
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_status);                         // Charge status
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_charge_low_threshold);                  // Charge low threshold
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
static void razer_barracuda_disconnect(struct hid_device *hdev)
{
    struct razer_barracuda_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);

    dev = hid_get_drvdata(hdev);

    if(dev->usb_interface_protocol == USB_INTERFACE_PROTOCOL_NONE) {
        switch(dev->usb_pid) {
        case USB_DEVICE_ID_RAZER_BARRACUDA:
            device_remove_file(&hdev->dev, &dev_attr_version);                               // Get driver version
            device_remove_file(&hdev->dev, &dev_attr_test);                                  // Test mode
            device_remove_file(&hdev->dev, &dev_attr_device_type);                           // Get string of device type
            device_remove_file(&hdev->dev, &dev_attr_device_serial);                         // Get string of device serial
            device_remove_file(&hdev->dev, &dev_attr_firmware_version);                      // Get string of device fw version
            device_remove_file(&hdev->dev, &dev_attr_device_mode);                           // Get device mode
            device_remove_file(&hdev->dev, &dev_attr_charge_level);                          // Charge level
            device_remove_file(&hdev->dev, &dev_attr_charge_status);                         // Charge status
            device_remove_file(&hdev->dev, &dev_attr_charge_low_threshold);                  // Charge low threshold
            break;
        }
    }

    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

static int razer_raw_event(struct hid_device *hdev, struct hid_report *report, u8 *data, int size)
{
    struct razer_barracuda_device *device = dev_get_drvdata(&hdev->dev);

    //printk(KERN_WARNING "razerbarracuda: Got raw message %d\n", size);

    if(size == 33) { // Should be a response to a Control packet
        memcpy(&device->data[0], &data[0], size);

    } else {
        printk(KERN_WARNING "razerbarracuda: Got raw message, length: %d\n", size);
    }

    return 0;
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_BARRACUDA) },
    { 0 }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_barracuda_driver = {
    .name = "razerbarracuda",
    .id_table = razer_devices,
    .probe = razer_barracuda_probe,
    .remove = razer_barracuda_disconnect,
    .raw_event = razer_raw_event
};

module_hid_driver(razer_barracuda_driver);
