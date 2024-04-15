# ********************************* WARNING *********************************
# This Makefile is not supposed to be used outside of certain usecases like
# compiling the driver ("make driver") in this repository or installing the
# files as part of distribution packaging.
#
# Please never run the install targets manually (unless you really know what
# you're doing) as they're not intended to be used like that!
#
# You're in nearly all cases better off following the build instructions
# that you can find in the wiki.
# ***************************************************************************

# PREFIX is used to prefix where the files will be installed under DESTDIR
PREFIX?=/usr
# UDEV_PREFIX is specifically a prefix for udev files
UDEV_PREFIX?=$(PREFIX)
# DESTDIR is used to install into a different root directory
DESTDIR?=/
# Specify the kernel directory to use
KERNELDIR?=/lib/modules/$(shell uname -r)/build
# Need the absolute directory do the driver directory to build kernel modules
DRIVERDIR?=$(shell pwd)/driver

# Where kernel drivers are going to be installed
MODULEDIR?=/lib/modules/$(shell uname -r)/kernel/drivers/hid

DKMS_NAME?=openrazer-driver
DKMS_VER?=3.8.0


# Build all target
all: driver

# Driver compilation
driver:
	@echo -e "\n::\033[32m Compiling OpenRazer kernel modules\033[0m"
	@echo "========================================"
	$(MAKE) -C $(KERNELDIR) M=$(DRIVERDIR) modules

driver_clean:
	@echo -e "\n::\033[32m Cleaning OpenRazer kernel modules\033[0m"
	@echo "========================================"
	$(MAKE) -C "$(KERNELDIR)" M="$(DRIVERDIR)" clean

# Install kernel modules and then update module dependencies
driver_install:
	@echo -e "\n::\033[34m Installing OpenRazer kernel modules\033[0m"
	@echo "====================================================="
	@cp -v $(DRIVERDIR)/*.ko $(DESTDIR)/$(MODULEDIR)
	@chown -v root:root $(DESTDIR)/$(MODULEDIR)/*.ko
	depmod

# Just use for packaging openrazer, not for installing manually
driver_install_packaging:
	@echo -e "\n::\033[34m Installing OpenRazer kernel modules\033[0m"
	@echo "====================================================="
	@cp -v $(DRIVERDIR)/*.ko $(DESTDIR)/$(MODULEDIR)

# Razer Daemon
daemon_install:
	@echo -e "\n::\033[34m Installing OpenRazer Daemon\033[0m"
	@echo "====================================================="
	make --no-print-directory -C daemon install

ubuntu_daemon_install:
	@echo -e "\n::\033[34m Installing OpenRazer Daemon\033[0m"
	@echo "====================================================="
	make --no-print-directory -C daemon ubuntu_install

# Python Library
python_library_install:
	@echo -e "\n::\033[34m Installing OpenRazer python library\033[0m"
	@echo "====================================================="
	@make --no-print-directory -C pylib install

ubuntu_python_library_install:
	@echo -e "\n::\033[34m Installing OpenRazer python library\033[0m"
	@echo "====================================================="
	@make --no-print-directory -C pylib ubuntu_install

# Legacy XDG autostart
xdg_install:
	@mkdir -p $(DESTDIR)/etc/xdg/autostart
	@cp -v ./install_files/desktop/openrazer-daemon.desktop $(DESTDIR)/etc/xdg/autostart/openrazer-daemon.desktop

install-systemd:
	@make --no-print-directory -C daemon install-systemd

# Clean target
clean: driver_clean

setup_dkms:
	@echo -e "\n::\033[34m Installing DKMS files\033[0m"
	@echo "====================================================="
	install -m 644 -v -D Makefile $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/Makefile
	install -m 644 -v -D install_files/dkms/dkms.conf $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/dkms.conf
	install -m 755 -v -d driver $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/driver
	install -m 644 -v -D driver/Makefile $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/driver/Makefile
	install -m 644 -v driver/*.c $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	install -m 644 -v driver/*.h $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/driver/
	rm -fv $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)/driver/*.mod.c

remove_dkms:
	@echo -e "\n::\033[34m Removing DKMS files\033[0m"
	@echo "====================================================="
	rm -rf $(DESTDIR)$(PREFIX)/src/$(DKMS_NAME)-$(DKMS_VER)

udev_install:
	@echo -e "\n::\033[34m Installing OpenRazer udev rules\033[0m"
	@echo "====================================================="
	install -m 644 -v -D install_files/udev/99-razer.rules $(DESTDIR)$(UDEV_PREFIX)/lib/udev/rules.d/99-razer.rules
	install -m 755 -v -D install_files/udev/razer_mount $(DESTDIR)$(UDEV_PREFIX)/lib/udev/razer_mount

appstream_install:
	@echo -e "\n::\033[34m Installing OpenRazer AppStream metadata\033[0m"
	@echo "====================================================="
	install -m 644 -v -D install_files/appstream/io.github.openrazer.openrazer.metainfo.xml $(DESTDIR)$(PREFIX)/share/metainfo/io.github.openrazer.openrazer.metainfo.xml

# Install for Ubuntu
# WARNING: do not use this target manually, it is just meant for Debian packaging! Read the warning on top of the file!
ubuntu_install: setup_dkms udev_install ubuntu_daemon_install ubuntu_python_library_install appstream_install
	@echo -e "\n::\033[34m Installing for Ubuntu\033[0m"
	@echo "====================================================="

install_i_know_what_i_am_doing: all driver_install udev_install python_library_install
	@make --no-print-directory -C daemon install DESTDIR=$(DESTDIR)

install: manual_install_msg ;

manual_install_msg:
	@echo "Please do not install the driver using this method. Use a distribution package as it tracks the files installed and can remove them afterwards. If you are 100% sure, you want to do this, find the correct target in the Makefile."
	@echo "Exiting."

.PHONY: driver
