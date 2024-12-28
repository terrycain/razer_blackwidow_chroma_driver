#!/bin/bash

declare -A files_metadata=(
    ["backlight_led_brightness"]="rw;0"
    ["backlight_led_state"]="rw;0"
    ["backlight_matrix_effect_breath"]="w;"
    ["backlight_matrix_effect_none"]="w;"
    ["backlight_matrix_effect_on"]="w;"
    ["backlight_matrix_effect_reactive"]="w;"
    ["backlight_matrix_effect_spectrum"]="w;"
    ["backlight_matrix_effect_static"]="w;"
    ["backlight_matrix_effect_wave"]="w;"
    ["charge_colour"]="w;"
    ["charge_effect"]="w;"
    ["charge_level"]="r;255"
    ["charge_low_threshold"]="rw;38"
    ["charge_status"]="r;1"
    ["charging_led_brightness"]="rw;0"
    ["charging_matrix_effect_breath"]="w;"
    ["charging_matrix_effect_none"]="w;"
    ["charging_matrix_effect_spectrum"]="w;"
    ["charging_matrix_effect_static"]="w;"
    ["charging_matrix_effect_wave"]="w;"
    ["device_idle_time"]="rw;600"
    ["device_mode"]="rw;0x0000"
    ["device_serial"]="r;XX0000000000" # default value will get overwritten
    ["device_type"]="r;%(name)s"
    ["dpi"]="rw;800:800"
    ["dpi_stages"]="rw;0x010320032005dc05dc"
    ["fast_charging_led_brightness"]="rw;0"
    ["fast_charging_matrix_effect_breath"]="w;"
    ["fast_charging_matrix_effect_none"]="w;"
    ["fast_charging_matrix_effect_spectrum"]="w;"
    ["fast_charging_matrix_effect_static"]="w;"
    ["fast_charging_matrix_effect_wave"]="w;"
    ["firmware_version"]="r;v1.0"
    ["fully_charged_led_brightness"]="rw;0"
    ["fully_charged_matrix_effect_breath"]="w;"
    ["fully_charged_matrix_effect_none"]="w;"
    ["fully_charged_matrix_effect_spectrum"]="w;"
    ["fully_charged_matrix_effect_static"]="w;"
    ["fully_charged_matrix_effect_wave"]="w;"
    ["game_led_state"]="rw;0"
    ["hyperpolling_wireless_dongle_indicator_led_mode"]="w;"
    ["hyperpolling_wireless_dongle_pair"]="w;"
    ["hyperpolling_wireless_dongle_unpair"]="w;"
    ["keyswitch_optimization"]="rw;0"
    ["is_mug_present"]="r;0"
    ["kbd_layout"]="r;01"
    ["key_alt_f4"]="rw;0x00"
    ["key_alt_tab"]="rw;0x00"
    ["key_super"]="rw;0x00"
    ["left_led_brightness"]="rw;0"
    ["left_matrix_effect_breath"]="w;"
    ["left_matrix_effect_none"]="w;"
    ["left_matrix_effect_reactive"]="w;"
    ["left_matrix_effect_spectrum"]="w;"
    ["left_matrix_effect_static"]="w;"
    ["left_matrix_effect_wave"]="w;"
    ["logo_led_brightness"]="rw;0"
    ["logo_led_effect"]="rw;0"
    ["logo_led_state"]="rw;0"
    ["logo_matrix_effect_blinking"]="w;"
    ["logo_matrix_effect_breath"]="w;"
    ["logo_matrix_effect_none"]="w;"
    ["logo_matrix_effect_on"]="w;"
    ["logo_matrix_effect_reactive"]="w;"
    ["logo_matrix_effect_spectrum"]="w;"
    ["logo_matrix_effect_static"]="w;"
    ["logo_matrix_effect_wave"]="w;"
    ["macro_led_effect"]="rw;0"
    ["macro_led_state"]="rw;0"
    ["matrix_brightness"]="rw;0"
    ["matrix_current_effect"]="w;05"
    ["matrix_custom_frame"]="w;"
    ["matrix_effect_blinking"]="w;"
    ["matrix_effect_breath"]="w;"
    ["matrix_effect_custom"]="w;"
    ["matrix_effect_none"]="w;"
    ["matrix_effect_pulsate"]="w;"
    ["matrix_effect_reactive"]="w;"
    ["matrix_effect_spectrum"]="w;"
    ["matrix_effect_starlight"]="w;"
    ["matrix_effect_static"]="w;"
    ["matrix_effect_wave"]="w;"
    ["matrix_effect_wheel"]="w;"
    ["matrix_reactive_trigger"]="w;"
    ["poll_rate"]="rw;500"
    ["profile_led_blue"]="rw;0"
    ["profile_led_green"]="rw;0"
    ["profile_led_red"]="rw;0"
    ["right_led_brightness"]="rw;0"
    ["right_matrix_effect_breath"]="w;"
    ["right_matrix_effect_none"]="w;"
    ["right_matrix_effect_reactive"]="w;"
    ["right_matrix_effect_spectrum"]="w;"
    ["right_matrix_effect_static"]="w;"
    ["right_matrix_effect_wave"]="w;"
    ["scroll_led_brightness"]="rw;0"
    ["scroll_led_effect"]="rw;0"
    ["scroll_led_state"]="rw;0"
    ["scroll_matrix_effect_blinking"]="w;"
    ["scroll_matrix_effect_breath"]="w;"
    ["scroll_matrix_effect_none"]="w;"
    ["scroll_matrix_effect_on"]="w;"
    ["scroll_matrix_effect_reactive"]="w;"
    ["scroll_matrix_effect_spectrum"]="w;"
    ["scroll_matrix_effect_static"]="w;"
    ["scroll_matrix_effect_wave"]="w;"
    ["scroll_mode"]="rw;0"
    ["scroll_acceleration"]="rw;0"
    ["scroll_smart_reel"]="rw;0"
    ["tilt_hwheel"]="rw;0"
    ["tilt_repeat"]="rw;0"
    ["tilt_repeat_delay"]="rw;0"
    ["reset_channels"]="w;1"
    ["channel1_size"]="rw;8"
    ["channel2_size"]="rw;8"
    ["channel3_size"]="rw;8"
    ["channel4_size"]="rw;8"
    ["channel5_size"]="rw;8"
    ["channel6_size"]="rw;8"
    ["channel1_led_brightness"]="rw;0"
    ["channel2_led_brightness"]="rw;0"
    ["channel3_led_brightness"]="rw;0"
    ["channel4_led_brightness"]="rw;0"
    ["channel5_led_brightness"]="rw;0"
    ["channel6_led_brightness"]="rw;0"
    ["version"]="r;1.0.0"
)

# USB PID of devices that use single-byte values for DPI
byte_dpi_devices=(
    '0013' # Razer Orochi 2011
    '0020' # Razer Abyssus 1800
    '002E' # Razer Naga 2012
    '0036' # Razer Naga Hex Red
    '0037' # Razer DeathAdder 2013
    '0038' # Razer DeathAdder 1800
    '0041' # Razer Naga Hex
)


get_attr_from_create_device_file() {
    sed -n 's/[[:space:]]\+CREATE_DEVICE_FILE(&hdev->dev, &dev_attr_\([[:lower:][:digit:]_]\+\));.*/\1/p'
}

driver=$1

if [ -z "$driver" ]; then
    echo "Usage: $0 razerkbd/razermouse"
    exit 1
fi

driver_short=$(echo "$driver" | sed 's/razer//g')

devices=$(git grep -h "#define USB_DEVICE_ID_RAZER_" driver/${driver}_driver.h | cut -d' ' -f2-3)

# https://askubuntu.com/a/849016
probe_func=$(sed -n '/^static int razer_'${driver_short}'_probe/,/^}$/p' driver/${driver}_driver.c)
devicetype_func=$(sed -n '/^static ssize_t razer_attr_read_device_type/,/^}$/p' driver/${driver}_driver.c)

probe_lines=$(echo "$probe_func" | grep 'case \|CREATE_DEVICE_FILE\|break;')
devicetype_lines=$(echo "$devicetype_func" | grep 'case \|device_type = \|break;')

# https://unix.stackexchange.com/a/11323
common_attrs_lines=$(echo "$probe_lines" | sed -ne '/CREATE_DEVICE_FILE/,$ p' | sed '/case USB_DEVICE_ID_RAZER_/Q')
common_attrs=$(echo "$common_attrs_lines" | get_attr_from_create_device_file)

# Iterate through all devices
while IFS= read -r device_raw; do
    device=$(echo "$device_raw" | cut -d' ' -f1)
    device_pid=$(echo "$device_raw" | cut -d' ' -f2 | sed 's/0x//')

    devicetype_line=$(echo "$devicetype_lines" | sed -n '/case '$device':/,/break;/p' | grep "device_type = ")
    device_name=$(echo "$devicetype_line" | sed 's/[[:space:]]\+device_type = "\(.\+\)\\n";.*/\1/' | sed 's/%/%%/g')

    #echo "-----------------------------" >&2
    echo "Generating for $device_name (1532:$device_pid) ..." >&2

    device_attrs_lines=$(echo "$probe_lines" | sed -n '/case '$device':/,/break;/p')
    device_attrs=$(echo "$device_attrs_lines" | get_attr_from_create_device_file)

    all_attrs=$(echo "$device_attrs"; echo "$common_attrs")
    all_attrs=$(echo "$all_attrs" | sort)

    if [ -z "$device_attrs_lines" ]; then
        echo "Error: couldn't find _probe lines for device $device"
        exit 1
    fi

    device_name_simple=$(echo "$device_name" | tr -d ' ' | tr -d '(' | tr -d ')' | sed 's/%%/pct/g' | tr '[:upper:]' '[:lower:]')
    filename="./pylib/openrazer/_fake_driver/$device_name_simple.cfg"

    # There are some devices that have multiple PID for the same codename.
    # Why exactly I'm not sure, but we need to add a suffix to the fake driver file then.
    if [ "$device_pid" = "0506" ] || # Razer Kraken 7.1
        [ "$device_pid" = "028F" ] || # Razer Ornata V3
        [ "$device_pid" = "02A2" ] || # Razer Ornata V3 X
        [ "$device_pid" = "00C2" ] || # Razer DeathAdder V3 Pro (Wired)
        [ "$device_pid" = "00C3" ]; then # Razer DeathAdder V3 Pro (Wireless)
        filename=${filename%.cfg}_2.cfg
    fi

    if [ -f "$filename" ]; then
        echo "Error: File $filename already exists!"
        exit 1
    fi

    cat << EOF > "$filename"
# DO NOT EDIT THIS FILE!
# You can regenerate all fake driver files with ./scripts/generate_all_fake_drivers.sh -f
[device]
dir_name = 0003:1532:$device_pid.0001
name = $device_name
EOF
    # echo "event = TODO" >> "$filename"
    echo -n "files = " >> "$filename"

    first_attr=true
    # Iterate through the attributes of the device
    while IFS= read -r attr; do
        # Ignore some attributes that we don't need in the fake driver
        # "": sometimes we get empty line in here
        # "test": not used by daemon
        # "fn_toggle": not used by daemon
        [[ "$attr" = "" || "$attr" = "test" || "$attr" = "fn_toggle" ]] && continue

        metadata="${files_metadata[$attr]}"
        if [ -z "$metadata" ]; then
            echo "Error: Missing metadata for attr: $attr"
            exit 1
        fi

        filemode=${metadata%;*} # part before the semicolon
        default=${metadata#*;} # part after the semicolon

        # Handle a few special cases
        if [[ "$attr" = "dpi" && " ${byte_dpi_devices[@]} " =~ " ${device_pid} " ]]; then
            default="30:30"
        elif [ "$attr" = "device_serial" ]; then
            default="XX000000$device_pid"
        fi

        if [ "$first_attr" = false ]; then
            # Indentation
            echo -n "        " >> "$filename"
        fi
        echo -n "$filemode,$attr" >> "$filename"
        if [ ! -z "$default" ]; then
            echo -n ",$default" >> "$filename"
        fi
        echo >> "$filename"

        first_attr=false
    done <<< "$all_attrs"
done <<< "$devices"
