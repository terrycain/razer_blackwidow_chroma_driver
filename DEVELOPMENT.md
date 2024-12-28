# Development

## Introduction

OpenRazer consists of a Linux kernel module and Python daemon + library. The Linux kernel module exposes an
sysfs interface, the Python daemon (openrazer-daemon) exposes this on it's turn over D-Bus.

It also includes a Python libraries for applications and scripts to use that talks to the daemon over D-Bus.

If you want to add support for a new hardware device it's often required to make changes to both the kernel module
(`driver/`) and the Python sources (`daemon/`).

Some other files that need modifying are:
* `README.md`
* `install_files/udev/99-razer.rules`
* `install_files/appstream/io.github.openrazer.openrazer.metainfo.xml`: generate with `./scripts/generate_appstream_file.sh`
* `pylib/openrazer/_fake_driver/*.cfg`: generate with `./scripts/generate_all_fake_drivers.sh -f`

Generally it's helpful to look at recent commits adding new devices and doing similar changes for your device.

This description assumes you have cloned the openrazer repository to your home directory.

## Setup your development environment

These instructions should help you setup a development environment for both the kernel module and Python
daemon.

### Kernel module

It is possible to do development and test your changes on your local system but you risk kernel panics and therefore
possibly some data loss. Therefore it's recommended to do your development on a separate virtual or physical machine.

When you forward a USB device to the virtual machine, it becomes inaccessible for the host, so if you forward your
keyboard make sure you have a second one attached so you can still control your host.

I had some success with QEMU on Linux with the following.

To add a device to QEMU (change hostbus/hostaddr to the Razer device):
```
device_add usb-host,id=razer,hostbus=5,hostaddr=2
```

and to remove the device from the virtual machine:
```
device_del razer
```

If you're on macOS or Windows I recommend to use VirtualBox and use USB passthough as described here:
[VirtualBox USB Support](https://www.virtualbox.org/manual/ch03.html#settings-usb)

Make sure you're using the same kernel version on both your development as your test machine. Easiest way
to achieve this is to use the same Linux distribution and use the latest available kernel version.

#### Debian/Ubuntu

For Debian/Ubuntu install the following packages for kernel development:

```
apt-get install -y make gcc flex bison bc linux-headers-$(uname -r)
```

#### RedHat/Fedora

For RedHat/Fedora install the following packages for kernel development:

```
dnf install -y binutils make gcc ncurses-devel sed flex bison kernel-headers
```

#### Build the kernel module

Run the following command to build the kernel module:

```
make driver
```

This will build the kernel modules in `driver/*.ko`. If you make changes to the sources
you can rerun the `make driver` command and the modules will be rebuild.


#### Test the kernel module

You can copy the `driver/*.ko` modules to your test system and (re)load for example
the keyboard module using:

```
rmmod razerkbd && insmod razerkbd.ko
```

If you update the kernel module you have to manually reload the module.

Driver messages can be found by running:

```
dmesg
```

### Python daemon

OpenRazer can only use your system site-packages and cannot be (easily) installed in
a virtualenv. Therefore you will have to install the dependencies on your system and
set your PYTHONPATH so you can run from sources.

#### Debian/Ubuntu

For Debian/Ubuntu install the following packages to be able to run the daemon:

```
apt-get install -y libnotify-bin python3 python3-daemonize python3-dbus python3-gi python3-numpy python3-pyudev python3-setproctitle
```

#### RedHat/Fedora

For RedHat/Fedora install the following packages to be able to run the daemon:

```
dnf install -y libnotify python3 python3-daemonize python3-dbus python3-numpy python3-pyudev python3-setproctitle
```

#### Test the daemon

Copy or have another clone of openrazer on your test system.

Run:

```
PYTHONPATH="pylib:daemon" python3 ./daemon/run_openrazer_daemon.py -Fv --config=$PWD/daemon/resources/razer.conf
```

## Contribute back your changes!

### Prerequisites

The OpenRazer code is licensed under the GPL-2.0-or-later license. Make sure you understand what this means. Please
also be sure that if your changes include work done by others they also agree with publishing under this license.

If you haven't already you'll have to sign up for a GitHub account, fork this repository and push your changes to a
branch in this fork.

### Check if there's a issue for what you've fixed

Please search our issues to see if you've fixed something that has been reported by someone already. If you're
adding support for a new device please create a new github issue.

### Create a new pull request

[Create a new pull request](https://github.com/openrazer/openrazer/compare) and please properly describe your changes.

Keep in mind, OpenRazer is an open source project managed by volunteers, we really appreciate your work but
everything is done as a best effort.

Even if your changes work we could decide they're not suitable for OpenRazer or require changes.
