/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2015 Terri Cain <terri@dolphincorp.co.uk>
 */

#ifndef __HID_RAZER_BARRACUDA_H
#define __HID_RAZER_BARRACUDA_H

// Codename Unknown
#define USB_DEVICE_ID_RAZER_BARRACUDA 0x053C

#define USB_INTERFACE_PROTOCOL_NONE 0

struct razer_barracuda_device {
    struct usb_device *usb_dev;
    struct mutex lock;
    unsigned char usb_interface_protocol;
    unsigned short usb_pid;
    unsigned short usb_vid;

    char serial[23];
    // 3 Bytes, first byte is whether fw version is collected, 2nd byte is major version, 3rd is minor, should be printed out in hex form as are bcd
    unsigned char firmware_version[3];

    u8 data[33];

};

/*
 * Should wait 15ms per write to EEPROM
 *
 * Report ID:
 *   0x04 - Output ID for memory access
 *   0x05 - Input ID for memory access result
 *
 * Destination:
 *   0x20 - Read data from EEPROM
 *   0x40 - Write data to RAM
 *   0x00 - Read data from RAM
 * */

struct razer_barracuda_request_report {
    unsigned char report_id;
    unsigned char destination;
    unsigned char length;
    unsigned char addr_h;
    unsigned char addr_l;
    unsigned char arguments[32];
};

struct razer_barracuda_response_report {
    unsigned char report_id;
    unsigned char arguments[36];
};

#endif
