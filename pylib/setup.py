#!/usr/bin/python3
# SPDX-License-Identifier: GPL-2.0-or-later

from setuptools import setup, find_packages

setup(
    name="openrazer",
    version="3.8.0",
    packages=find_packages(exclude=["tests", "openrazer._fake_driver"]),
    install_requires=[
        "dbus-python >= 1.2.0",
        "numpy >= 1.11.0",
        "openrazer_daemon == 3.8.0",
    ],
    author="OpenRazer contributors",
    description="Library for interacting with the OpenRazer daemon.",
    license="GPLv2+",
    url="https://openrazer.github.io/",
    project_urls={
        "Bug Tracker": "https://github.com/openrazer/openrazer/issues",
        "Source Code": "https://github.com/openrazer/openrazer/tree/master/pylib",
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)",
        "Operating System :: POSIX :: Linux",
    ],
)
