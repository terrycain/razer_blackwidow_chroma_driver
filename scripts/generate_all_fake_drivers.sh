#!/bin/bash

set -e

if [ "$1" = "-f" ]; then
    echo "Removing existing fake driver files..."
    rm -f $(dirname "$0")/../pylib/openrazer/_fake_driver/*.cfg
fi

drivers=$(ls driver/razer*_driver.c | cut -d'/' -f2 | cut -d'_' -f1)

for driver in $drivers; do
    [ "$driver" = "razercore" ] && continue # razercore is currently broken
    $(dirname "$0")/generate_fake_driver.sh $driver
done
