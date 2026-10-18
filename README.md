# Robotics-BLE (DA14580 Bio-Robotics Controller)

DA14580 BLE firmware built on SDK 5.0.4 for the UQ Biorobotics Lab. This firmware generates dynamic PWM signals (1Hz to 255Hz) to control directional stimulation on a bio-robotic beetle. It communicates with a custom Web Bluetooth HTML/JS interface that supports physical gamepads.

**Quick Links**
*   **SDK Source:** Renesas DA14580 SDK 5.0.4

## Features
*   **Hybrid PWM System:** Uses hardware `timer0` for high frequencies (≥ 31 Hz) and an `app_easy_timer` software state machine for ultra-low frequencies (< 31 Hz) to bypass the 16-bit hardware limit.
*   **Custom BLE Payload:** Accepts a 3-byte data packet (`[pin, freq, duty]`) over a custom GATT characteristic.
*   **Target Routing:** Supports specific pad routing (Right Antenna on P04, Left Antenna on P05) and a Cerci Dual-Fire case (P04 & P06 simultaneously).
*   **Web GUI Integration:** Controlled via a browser-based dashboard that handles pulse-width-to-duty-cycle calculations and standard USB/Bluetooth controller inputs.

## Where to Find the Code
Everything modified for this specific application is contained in these files:
*   `DA1458x_SDK\5.0.4\projects\target_apps\ble_examples\ble_app_peripheral\src\platform\user_periph_setup.c`
    *   Sets up the hardware pins, handles the hybrid PWM logic, and intercepts 0% duty cycles for strict 500ms duration control.
*   `DA1458x_SDK\5.0.4\projects\target_apps\ble_examples\ble_app_peripheral\src\user_custs1_impl.c`
    *   Handles the Bluetooth connection and parses incoming 3-byte GATT write commands.
*   `DA1458x_SDK\5.0.4\projects\target_apps\ble_examples\ble_app_peripheral\src\user_custs1_def.h`
    *   Expands the receiving characteristic length to accept the custom 3-byte payload.

## How to Build and Run

**1. Open the Project**
Open the Keil uVision project file located here:
`DA1458x_SDK\5.0.4\projects\target_apps\ble_examples\ble_app_peripheral\Keil_5\ble_app_peripheral.uvprojx`

**2. Add PWM Dependency**
Before compiling, you must manually add the hardware PWM driver to the project tree.
*   In Keil, expand the project tree and find the **`Sdk_driver_api`** group.
*   Right-click the group, select "Add Existing Files...", and add:
    `DA1458x_SDK\5.0.4\sdk\platform\driver\pwm\pwm.c`

**3. Compile and Flash**
*   Compile the project. 
*   *Note: Because the firmware is not yet permanently burned to the chip's OTP/Flash memory, you must run it via RAM.* Click the **Start/Stop Debug Session** button (or press `Ctrl+F5`) in Keil to load and execute the code on the connected DA14580 development board.

**4. Execute**
*   Once the debug session is running, open the custom HTML GUI in Microsoft Edge (or any Web Bluetooth-enabled browser).
*   Click "Connect to DIALOG-PRPH", pair your physical controller (or use the arrow keys), and begin transmitting stimulation payloads.
