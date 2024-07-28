#!/usr/bin/python3
import openrazer.client
import glob

daemon_test_dir = "/tmp/daemon_test"
devmgr = openrazer.client.DeviceManager()
passed = True


def _test_failed(name, msg):
    global passed
    passed = False
    print(f"\nFAILED: {name}\n       ", msg)


def test_sanity_check_matrix_capabilities(d):
    if d.has("lighting_led_matrix"):
        d.fx.advanced.matrix[0, 0] = [0, 255, 0]
        try:
            d.fx.advanced.draw()
        except Exception as e:
            _test_failed(d.name, e)


def test_wired_wireless_naming():
    # Enforce the (Wired) or (Wireless) suffix naming convention when devices have multiple PIDs
    device_names = list(map(lambda device: device.name, devmgr.devices))

    for name in device_names:
        if name.find("(Wired)") != -1:
            striped_name = name.replace("(Wired)", "").strip()
            if striped_name in device_names:
                _test_failed(striped_name, "Naming pattern is inconsistent. Append \"(Wireless)\" for the other USB PID.")

        elif name.find("(Wireless)") != -1:
            striped_name = name.replace("(Wireless)", "").strip()
            if striped_name in device_names:
                _test_failed(striped_name, "Naming pattern is inconsistent. Append \"(Wired)\" for the other USB PID.")


def test_sysfs_consistency(d):
    # Check sysfs files exist for capabilities
    vid = str(hex(d._vid))[2:].upper().rjust(4, '0')
    pid = str(hex(d._pid))[2:].upper().rjust(4, '0')

    def check_sysfs(capability: str, sysfs_name: str):
        """
        Check the device has either the given pylib capability for the
        given sysfs name, and vice versa.
        """
        try:
            expected_path = glob.glob(f"{daemon_test_dir}/*:{vid}:{pid}*/{sysfs_name}", recursive=True)[0]
        except IndexError:
            expected_path = ""

        if d.has(capability) and not glob.glob(expected_path, recursive=True):
            _test_failed(d.name, str(hex(d._pid)) + " Has capability '{}' but no sysfs file: '{}'".format(capability, sysfs_name))

        if not d.has(capability) and glob.glob(expected_path, recursive=True):
            _test_failed(d.name, str(hex(d._pid)) + " Has sysfs '{}' but no capability set: '{}'".format(sysfs_name, capability))

    def check_any_sysfs(capabilities: list[str], sysfs_names: list[str]):
        """
        Check the device has one of these sysfs names for one of these pylib capabilities.
        """
        found_sysfs = []
        found_capability = []

        for sysfs_name in sysfs_names:
            if glob.glob(f"{daemon_test_dir}/*:{vid}:{pid}*/{sysfs_name}", recursive=True):
                found_sysfs.append(sysfs_name)

        for capability in capabilities:
            if d.has(capability):
                found_capability.append(capability)

        # Skip Razer devices that have non-RGB effects
        if not found_capability and ("logo_led_effect" in sysfs_names or "scroll_led_effect" in sysfs_names):
            return

        if not found_capability and found_sysfs:
            _test_failed(d.name, str(hex(d._pid)) + " Has one of these sysfs {} but none of these capabilities: {}".format(sysfs_names, capabilities))

        if found_capability and not found_sysfs:
            _test_failed(d.name, str(hex(d._pid)) + " Has one of these capabilities {} but none of these sysfs files: {}".format(capabilities, sysfs_names))

    # check_sysfs("battery", "charge_effect") # FIXME this is not correct, as per PR comments
    check_sysfs("set_low_battery_threshold", "charge_low_threshold")  # deprecated
    check_sysfs("low_battery_threshold", "charge_low_threshold")
    check_sysfs("set_idle_time", "device_idle_time")  # deprecated
    check_sysfs("idle_time", "device_idle_time")

    check_sysfs("battery", "charge_level")
    check_sysfs("battery", "charge_status")
    check_sysfs("brightness", "matrix_brightness")
    check_sysfs("dpi", "dpi")
    check_sysfs("dpi_stages", "dpi_stages")
    check_sysfs("firmware_version", "firmware_version")
    check_sysfs("game_mode_led", "game_led_state")
    check_sysfs("macro_mode_led", "macro_led_state")
    check_sysfs("macro_mode_led_effect", "macro_led_effect")
    check_sysfs("poll_rate", "poll_rate")
    check_sysfs("serial", "device_serial")
    check_sysfs("type", "device_type")

    if d.type == "keyboard":
        check_sysfs("keyboard_layout", "kbd_layout")

    # Devices with a matrix defined should have a suitable "custom" sysfs. Krakens are a special case.
    if d._matrix_dimensions != (-1, -1) and d.name.find("Kraken") == -1:
        check_sysfs("lighting_led_matrix", "matrix_custom_frame")
        check_sysfs("lighting_led_matrix", "matrix_effect_custom")

    check_sysfs("lighting_breath_single", "matrix_effect_breath")
    check_sysfs("lighting_none", "matrix_effect_none")
    check_sysfs("lighting_reactive", "matrix_effect_reactive")
    check_sysfs("reactive_trigger", "matrix_reactive_trigger")
    check_sysfs("lighting_spectrum", "matrix_effect_spectrum")
    check_any_sysfs(["lighting_starlight_random", "lighting_starlight_single", "lighting_starlight_dual"], ["matrix_effect_starlight"])
    # Ignore devices that have mono-color razer.device.lighting.bw2013.setStatic()
    if d._pid not in [0x010d, 0x010e, 0x0113, 0x0118, 0x011a, 0x011b, 0x011c, 0x0202]:
        check_sysfs("lighting_static", "matrix_effect_static")
    check_sysfs("lighting_wave", "matrix_effect_wave")
    check_sysfs("lighting_wheel", "matrix_effect_wheel")
    check_sysfs("lighting_pulsate", "matrix_effect_pulsate")
    check_sysfs("lighting_blinking", "matrix_effect_blinking")

    check_sysfs("lighting_backlight_active", "backlight_led_state")

    for prefix in ["logo", "scroll", "left", "right", "charging", "fast_charging", "fully_charged"]:
        check_sysfs(f"lighting_{prefix}_brightness", f"{prefix}_led_brightness")
        check_sysfs(f"lighting_{prefix}_active", f"{prefix}_led_state")

        for effect in ["none", "reactive", "spectrum", "static", "wave"]:
            # There are two implementations with different sysfs files
            check_any_sysfs([f"lighting_{prefix}_{effect}"], [f"{prefix}_matrix_effect_{effect}", f"{prefix}_led_effect"])

        check_any_sysfs([f"lighting_{prefix}_breath_single", f"lighting_{prefix}_breath_mono"], [f"{prefix}_matrix_effect_breath"])


def test_ripple_capable(d):
    # Check that the device has a matrix for a software ripple effect
    if d.has("lighting_ripple") or d.has("lighting_ripple_random"):
        if d._matrix_dimensions == (-1, -1):
            _test_failed(d.name, "Cannot have a ripple capability without a matrix")


for d in devmgr.devices:
    test_sanity_check_matrix_capabilities(d)
    test_sysfs_consistency(d)
    test_ripple_capable(d)
test_wired_wireless_naming()

if not passed:
    exit(1)
