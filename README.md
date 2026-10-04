# Robotics-BLE (DA14580 Hello World)

Simple DA14580 BLE firmware built on SDK 5.0.4 for the UQ Biorobotics Lab. It connects to a smartphone app via Bluetooth and outputs a square wave.

## Quick Links
* **SDK Source:** [Renesas DA14580 SDK 5.0.4](https://www.renesas.com/en/products/connectivity-and-communications/bluetooth-low-energy/da14580-smartbond-bluetooth-low-energy-4-2-soc)

## Where to find my code
Everything I modified for this task is in these two files:
1.`DA1458x_SDK/5.0.4/projects/target_apps/ble_examples/ble_app_peripheral/src/platform/user_periph_setup.c` — Sets up the hardware pins and peripherals.

2. `DA1458x_SDK/5.0.4/projects/target_apps/ble_examples/ble_app_peripheral/src/user_custs1_impl.c` — Handles the Bluetooth connection, GATT writes, and PWM timer logic to generate the wave.

## Features
* Connects to standard BLE apps (like LightBlue or nRF Connect).
* Listens for GATT write commands to control the waveform output.
