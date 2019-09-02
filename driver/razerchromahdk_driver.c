/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Should you need to contact me, the author, you can do so by
 * e-mail - mail your message to Terry Cain <terry@terrys-home.co.uk>
 */


#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#include "razercommon.h"
#include "razerchromahdk_driver.h"
#include "razerchromacommon.h"


/*
 * Version Information
 */
#define DRIVER_DESC "Razer Chroma HDK Device Driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE(DRIVER_LICENSE);

static unsigned char saved_brightness = 0;

/**
 * Send report to the chromahdk
 */
int razer_get_report(struct usb_device *usb_dev, struct razer_report *request_report, struct razer_report *response_report)
{
    return razer_get_usb_response(usb_dev, 0x00, request_report, 0x00, response_report, RAZER_CHROMA_HDK_WAIT_MIN_US, RAZER_CHROMA_HDK_WAIT_MAX_US);
}

/**
 * Function to send to device, get response, and actually check the response
 */
struct razer_report razer_send_payload(struct usb_device *usb_dev, struct razer_report *request_report)
{
    int retval = -1;
    struct razer_report response_report = {0};

    request_report->crc = razer_calculate_crc(request_report);

    retval = razer_get_report(usb_dev, request_report, &response_report);

    if(retval == 0) {
        // Check the packet number, class and command are the same
        if(response_report.remaining_packets != request_report->remaining_packets ||
           response_report.command_class != request_report->command_class ||
           response_report.command_id.id != request_report->command_id.id) {
            print_erroneous_report(&response_report, "razerchromahdk", "Response doesn't match request");
//		} else if (response_report.status == RAZER_CMD_BUSY) {
//			print_erroneous_report(&response_report, "razerchromahdk", "Device is busy");
        } else if (response_report.status == RAZER_CMD_FAILURE) {
            print_erroneous_report(&response_report, "razerchromahdk", "Command failed");
        } else if (response_report.status == RAZER_CMD_NOT_SUPPORTED) {
            print_erroneous_report(&response_report, "razerchromahdk", "Command not supported");
        } else if (response_report.status == RAZER_CMD_TIMEOUT) {
            print_erroneous_report(&response_report, "razerchromahdk", "Command timed out");
        }
    } else {
        print_erroneous_report(&response_report, "razerchromahdk", "Invalid Report Length");
    }

    return response_report;
}

/**
 * Read device file "set_brightness"
 *
 * Returns brightness or -1 if the initial brightness is not known
 */
static ssize_t razer_attr_read_set_brightness(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report response = {0};
    struct razer_report report = {0};
    unsigned char brightness = 0;

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        brightness = saved_brightness;
        break;

    default:
        break;
    }

    return sprintf(buf, "%d\n", brightness);
}

/**
 *
 * Write device file "set_brightness"
 *
 * Sets the brightness to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_set_brightness(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};
    unsigned char brightness = (unsigned char)simple_strtoul(buf, NULL, 10);

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        report = razer_chroma_extended_matrix_brightness(VARSTORE, ZERO_LED, brightness);
        saved_brightness = brightness;
        break;

    default:
        break;
    }

    razer_send_payload(usb_dev, &report);

    return count;
}

/**
 * Read device file "get_firmware_version"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_get_firmware_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = razer_chroma_standard_get_firmware_version();
    struct razer_report response_report = razer_send_payload(usb_dev, &report);

    return sprintf(buf, "v%d.%d\n", response_report.arguments[0], response_report.arguments[1]);
}

/**
 * Read device file "device_type"
 *
 * Returns friendly string of device type
 */
static ssize_t razer_attr_read_device_type(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    char *device_type;

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        device_type = "Razer Chroma HDK\n";
        break;

    default:
        device_type = "Unknown Device\n";
        break;
    }

    return sprintf(buf, device_type);
}

/**
 * Read device file "get_serial"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_get_serial(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    char serial_string[23];
    struct razer_report report = razer_chroma_standard_get_serial();
    struct razer_report response_report = razer_send_payload(usb_dev, &report);

    strncpy(&serial_string[0], &response_report.arguments[0], 22);
    serial_string[22] = '\0';

    return sprintf(buf, "%s\n", &serial_string[0]);
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
 * Write device file "mode_none"
 *
 * No effect is activated whenever this file is written to
 */
static ssize_t razer_attr_write_mode_none(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        report = razer_chroma_extended_matrix_effect_none(VARSTORE, ZERO_LED);
        break;

    default:
        break;
    }

    razer_send_payload(usb_dev, &report);

    return count;
}

/**
 * Write device file "mode_spectrum"
 *
 * Specrum effect mode is activated whenever the file is written to
 */
static ssize_t razer_attr_write_mode_spectrum(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        report = razer_chroma_extended_matrix_effect_spectrum(VARSTORE, ZERO_LED);
        break;

    default:
        break;
    }

    razer_send_payload(usb_dev, &report);

    return count;
}

/**
 * Write device file "mode_breath"
 */
static ssize_t razer_attr_write_mode_breath(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        switch(count) {
        case 3: // Single colour mode
            report = razer_chroma_extended_matrix_effect_breathing_single(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0]);
            break;

        case 6: // Dual colour mode
            report = razer_chroma_extended_matrix_effect_breathing_dual(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0], (struct razer_rgb *)&buf[3]);
            break;

        default: // "Random" colour mode
            report = razer_chroma_extended_matrix_effect_breathing_random(VARSTORE, ZERO_LED);
            break;
        }
        break;

    default:
        break;
    }

    razer_send_payload(usb_dev, &report);

    return count;
}

/**
 * Write device file "mode_custom"
 *
 * Sets the chromahdk to custom mode whenever the file is written to
 */
static ssize_t razer_attr_write_mode_custom(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    switch (usb_dev->descriptor.idProduct) {
    case USB_DEVICE_ID_RAZER_CHROMA_HDK:
        report = razer_chroma_extended_matrix_effect_custom_frame();
        break;

    default:
        break;
    }

    razer_send_payload(usb_dev, &report);

    return count;
}

/**
 * Write device file "mode_static"
 *
 * Set the chromahdk to static mode when 3 RGB bytes are written
 */
static ssize_t razer_attr_write_mode_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    if (count == 3) {
        switch (usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
            report = razer_chroma_extended_matrix_effect_static(VARSTORE, ZERO_LED, (struct razer_rgb *)&buf[0]);
            break;

        default:
            break;
        }

        razer_send_payload(usb_dev, &report);
    } else {
        printk(KERN_WARNING "razerchromahdk: Static mode only accepts RGB (3byte)");
    }

    return count;
}

/**
 * Write device file "set_key_row"
 *
 * Writes the colour to the LEDs of the chromahdk
 *
 * Chroma HDK:
 * Start is 0x00
 * Stop is 0x0F
 * Has 4 rows 0x00 through 0x03
 */
static ssize_t razer_attr_write_set_key_row(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};
    size_t offset = 0;
    unsigned char row_id;
    unsigned char start_col;
    unsigned char stop_col;
    unsigned char row_length;

    //printk(KERN_ALERT "razerchromahdk: Total count: %d\n", (unsigned char)count);

    while(offset < count) {
        if(offset + 3 > count) {
            printk(KERN_ALERT "razerchromahdk: Wrong Amount of data provided: Should be ROW_ID, START_COL, STOP_COL, N_RGB\n");
            break;
        }

        row_id = buf[offset++];
        start_col = buf[offset++];
        stop_col = buf[offset++];
        row_length = ((stop_col+1) - start_col) * 3;

        //printk(KERN_ALERT "razerchromahdk: Row ID: %d, Start: %d, Stop: %d, row length: %d\n", row_id, start_col, stop_col, row_length);

        if(start_col > stop_col) {
            printk(KERN_ALERT "razerchromahdk: Start column is greater than end column\n");
            break;
        }

        if(offset + row_length > count) {
            printk(KERN_ALERT "razerchromahdk: Not enough RGB to fill row\n");
            break;
        }

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
            report = razer_chroma_extended_matrix_set_custom_frame(row_id, start_col, stop_col, (unsigned char *)&buf[offset]);
            report.transaction_id.id = 0x1F;
            break;

        default:
            break;
        }

        razer_send_payload(usb_dev, &report);

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
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = {0};

    if(count == 2) {
        report = razer_chroma_standard_set_device_mode(buf[0], buf[1]);
        razer_send_payload(usb_dev, &report);
    } else {
        printk(KERN_WARNING "razerkbd: Device mode only takes 2 bytes.");
    }

    return count;
}

/**
 * Read device file "device_mode"
 *
 * Returns a string
 */
static ssize_t razer_attr_read_device_mode(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_report report = razer_chroma_standard_get_device_mode();
    struct razer_report response = razer_send_payload(usb_dev, &report);

    return sprintf(buf, "%d:%d\n", response.arguments[0], response.arguments[1]);
}




/**
 * Set up the device driver files
 *
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */
static DEVICE_ATTR(firmware_version,        0440, razer_attr_read_get_firmware_version, NULL);
static DEVICE_ATTR(device_type,             0440, razer_attr_read_device_type,          NULL);
static DEVICE_ATTR(device_serial,           0440, razer_attr_read_get_serial,           NULL);
static DEVICE_ATTR(device_mode,             0660, razer_attr_read_device_mode,          razer_attr_write_device_mode);
static DEVICE_ATTR(version,                 0440, razer_attr_read_version,              NULL);
static DEVICE_ATTR(matrix_brightness,       0664, razer_attr_read_set_brightness,       razer_attr_write_set_brightness);
static DEVICE_ATTR(matrix_effect_none,      0220, NULL,                                 razer_attr_write_mode_none);
static DEVICE_ATTR(matrix_effect_spectrum,  0220, NULL,                                 razer_attr_write_mode_spectrum);
static DEVICE_ATTR(matrix_effect_breath,    0220, NULL,                                 razer_attr_write_mode_breath);
static DEVICE_ATTR(matrix_effect_custom,    0220, NULL,                                 razer_attr_write_mode_custom);
static DEVICE_ATTR(matrix_effect_static,    0220, NULL,                                 razer_attr_write_mode_static);
static DEVICE_ATTR(matrix_custom_frame,     0220, NULL,                                 razer_attr_write_set_key_row);




/**
 * Probe method is ran whenever a device is binded to the driver
 *
 * TODO remove goto's
 */
static int razer_chromahdk_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
    int retval = 0;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_chromahdk_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_chromahdk_device), GFP_KERNEL);
    if(dev == NULL) {
        dev_err(&intf->dev, "out of memory\n");
        retval = -ENOMEM;
        goto exit;
    }

    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_version);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_custom_frame);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_none);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_breath);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_custom);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_serial);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_firmware_version);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_type);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_effect_static);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_matrix_brightness);
            CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_device_mode);

            // Device initial brightness is always 100% anyway
            saved_brightness = 0xFF;
            break;

        default:
            break;
        }

    }

    hid_set_drvdata(hdev, dev);


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


    usb_disable_autosuspend(usb_dev);
    return 0;
exit:
    return retval;
exit_free:
    kfree(dev);
    return retval;
}

/**
 * Unbind function
 */
static void razer_chromahdk_disconnect(struct hid_device *hdev)
{
    struct razer_kbd_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);

    if(intf->cur_altsetting->desc.bInterfaceProtocol == USB_INTERFACE_PROTOCOL_MOUSE) {

        switch(usb_dev->descriptor.idProduct) {
        case USB_DEVICE_ID_RAZER_CHROMA_HDK:
            device_remove_file(&hdev->dev, &dev_attr_version);
            device_remove_file(&hdev->dev, &dev_attr_matrix_custom_frame);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_spectrum);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_none);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_breath);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_custom);
            device_remove_file(&hdev->dev, &dev_attr_device_serial);
            device_remove_file(&hdev->dev, &dev_attr_firmware_version);
            device_remove_file(&hdev->dev, &dev_attr_device_type);
            device_remove_file(&hdev->dev, &dev_attr_matrix_effect_static);
            device_remove_file(&hdev->dev, &dev_attr_matrix_brightness);
            device_remove_file(&hdev->dev, &dev_attr_device_mode);
            break;

        default:
            break;
        }

    }

    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}

/**
 * Device ID mapping table
 */
static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER,USB_DEVICE_ID_RAZER_CHROMA_HDK) },
    { }
};

MODULE_DEVICE_TABLE(hid, razer_devices);

/**
 * Describes the contents of the driver
 */
static struct hid_driver razer_chromahdk_driver = {
    .name =        "razerchromahdk",
    .id_table =    razer_devices,
    .probe =    razer_chromahdk_probe,
    .remove =    razer_chromahdk_disconnect,
};

module_hid_driver(razer_chromahdk_driver);