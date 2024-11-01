## About
Avnet IoTConnect AT Command Interface project example for the Renesas EK-RA6M4.

This code is based on the [CK-RA6M4 Sample Code]() project and uses an [IoTConnect AT Command-enabled PMOD module (e.g. DA16K based)](https://github.com/avnet-iotconnect/iotc-dialog-da16k-sdk) as a gateway.

`quickstart_ek_ra6m4_ep` Copyright 2023 Renesas Electronics Corporation and/or its affiliates.

## Dependencies
The project uses the following dependent projects:
* [iotc-freertos-da16k-atcmd-lib](https://github.com/avnet-iotconnect/iotc-freertos-da16k-atcmd-lib)

A fully set up DA16600 PMOD Module is required to be set up and connected to IoTConnect.

Follow the [DA16K AT Interface Quick Start guide.](https://github.com/avnet-iotconnect/iotc-dialog-da16k-sdk/blob/main/doc/QUICKSTART.md)


## Supported Boards

[Renesas EK-RA6M4](https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra6m4-evaluation-kit-ra6m4-mcu-group) with [DA16600 Wi-Fi-BLE combo module](https://www.renesas.com/us/en/products/wireless-connectivity/wi-fi/low-power-wi-fi/da16600mod-ultra-low-power-wi-fi-bluetooth-low-energy-combo-modules-battery-powered-iot-devices).


## Getting Started

Follow the [Renesas EK-RA6M4 Quick Start Guide](https://www.renesas.com/us/en/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ek-ra6m4-evaluation-kit-ra6m4-mcu-group#documents) to set up your EK-RA6M4 board for use.

**Make sure the DA16x00 device is connected to the PMOD1 connector.**

**Follow the [Quickstart Guide](./QUICKSTART.md) to flash the included binary image and use it.**

## Supported IoTC Device Commands

You can add these to your device template in the IoTConnect Dashboard.

* `set_red_led <state>`

    Controls state of red LED on the board

    * `on` - Turns red LED on
    * `off` - Truns red LED off

* `set_led_frequency <freq>`

    Controls the frequency of the blue LED flashing on the board

    * `0` - Slow blinking
    * `1` - Medium blinking
    * `2` - Fast blinking
    * `3` - Very fast blinking

## Building & Development

Install the [Flexible Software Package with e² Studio IDE](https://www.renesas.com/us/en/software-tool/flexible-software-package-fsp).

The project is tested and built with FSP version 5.0.0, but it may work with later 5.x.x versions.

You can now open / import, build and debug the project as per the Renesas Quick Start guide.