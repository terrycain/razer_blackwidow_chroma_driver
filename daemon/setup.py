#!/usr/bin/python3
# SPDX-License-Identifier: GPL-2.0-or-later

from setuptools import setup, find_packages

setup(
    name="openrazer_daemon",
    version="3.8.0",
    packages=find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
    install_requires=[
        "daemonize >= 2.4.7",
        "dbus-python >= 1.2.0",
        "PyGObject >= 3.20.0",
        "pyudev >= 0.16.1",
        "setproctitle >= 1.1.8",
    ],
)
