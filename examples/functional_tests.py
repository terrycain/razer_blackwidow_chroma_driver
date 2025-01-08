import sys
from openrazer.client import DeviceManager
from openrazer.client import constants as razer_constants

def main():
    # Create a DeviceManager. This is used to get specific devices
    device_manager = DeviceManager()


    print("Found {} Razer devices".format(len(device_manager.devices)))
    print()


    prompt = 'noprompt' not in sys.argv

    # Iterate over each device and pretty out some standard information about each
    for device in device_manager.devices:
        for capability, supported in device.capabilities.items():
            if not supported:
                continue

            if capability == "name":
                print(f"device.name: {device.name}")
            elif capability == "type":
                print(f"device.type: {device.type}")
            elif capability == "firmware_version":
                print(f"device.firmware_version: {device.firmware_version}")
            elif capability == "serial":
                print(f"device.serial: {device.serial}")
            elif capability == "brightness":
                print(f"device.brightness: {device.brightness}")
                device.brightness = 100
                print("brightness = 100")
                if prompt: input()
                device.brightness = 50
                print("brightness = 50")
                if prompt: input()
            elif capability == "keyboard_layout":
                print(f"device.keyboard_layout: {device.keyboard_layout}")
            elif capability == "lighting":
                print(f"lighting is supported")
            elif capability == "lighting_breath_single":
                for red, green, blue in ((256, 0, 0), (0, 256, 0), (0, 0, 256)):
                    device.fx.breath_single(red, green, blue)
                    print(f"breath_single red: {red} green: {green} blue: {blue}")
                    if prompt: input()
            elif capability == "lighting_breath_dual":
                for r1, g1, b1, r2, g2, b2 in ((256, 0, 0, 0, 0, 0), (0, 256, 0, 0, 0, 0), (0, 0, 256, 0, 0, 0),
                                               (0, 0, 0, 256, 0, 0), (0, 0, 0, 0, 256, 0), (0, 0, 0, 0, 0, 256)):
                    device.fx.breath_dual(r1, g1, b1, r2, g2, b2)
                    print(f"lighting_breath_dual red1: {r1} green1: {g1} blue1: {b1} red2: {r2} green2: {g2} blue2: {b2}")
                    if prompt: input()
            elif capability == "lighting_breath_random":
                device.fx.breath_random()
                print("breath_random")
                if prompt: input()
            elif capability == "lighting_wave":
                device.fx.wave(razer_constants.WAVE_LEFT)
                print("lighting_wave WAVE_LEFT")
                if prompt: input()
                device.fx.wave(razer_constants.WAVE_RIGHT)
                print("lighting_wave WAVE_RIGHT")
                if prompt: input()
            elif capability == "lighting_reactive":
                time = razer_constants.REACTIVE_500MS
                device.fx.reactive(255, 0, 255, time)
                print("lighting_reactive")
                if prompt: input()
            elif capability == "lighting_none":
                print("lighting_none")
                device.fx.none()
                if prompt: input()
            elif capability == "lighting_spectrum":
                device.fx.spectrum()
                print("lighting_spectrum")
                if prompt: input()
            elif capability == "lighting_static":
                device.fx.static(255, 0, 255)
                print("lighting_static")
                if prompt: input()
            elif capability == "lighting_led_matrix":
                device.fx.none()
                max_x, max_y = device._matrix_dimensions
                for x in range(max_x):
                    for y in range(max_y):
                        device.fx.advanced.matrix.set(x, y, (255, 0, 255))
                        device.fx.advanced.draw()
                        print("lighting_led_matrix {x} {y} 255, 0, 255")
                        if prompt: input()
            else:
                print(f"unknown capability: {capability}")

if __name__ == "__main__":
    main()
