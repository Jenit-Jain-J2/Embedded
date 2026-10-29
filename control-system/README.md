
---

# HV 6kV Dual-Channel Control System

## 1. Project Overview
This firmware and hardware system controls a dual-channel High Voltage (HV) power supply capable of outputting 0 to ±6000V DC. It is designed to provide precise bias voltages for RPC detectors in the XRID Lab. The system supports local manual control via a TFT display and hardware switches, as well as remote control via USB Serial or Ethernet (TCP).

---

## 2. Hardware Components & Connections

### Major Components
*   **Microcontroller**: WIZnet W5100S-EVB-Pico (RP2040 MCU + W5100S Ethernet).
*   **High Voltage Modules**: 2x iseg `bp0606r4p12` (6kV, 6W, Max 670µA).
*   **DAC (Digital-to-Analog)**: DFRobot Gravity GP8413 (15-bit, 0-5V output, I2C).
*   **ADC (Analog-to-Digital)**: 2x ADS1115 (16-bit, 4-Channel, I2C).
*   **Display**: 2.4" TFT SPI 240x320 (ST7789 driver, no touch).
*   **Power Regulation**: LM7805 (Provides 5V logic power from 12V isolated DC-DC supply).

### Connections
*   **I2C Bus**: DAC (`0x59`), ADC Positive (`0x49`), ADC Negative (`0x48`). Connected to RP2040 GPIO 2 (SDA) and GPIO 3 (SCL).
*   **SPI Bus**: TFT Display (CS: GPIO 13, DC: GPIO 14, RST: GPIO 15) and W5100S Ethernet.
*   **ADC Inputs**: Reads `Vmon` (Voltage Monitor) and `Imon` (Current Monitor) from the iseg modules.
*   **DAC Outputs**: Outputs `0-5V` analog signals to the `VSET` pins of the iseg modules.
*   **Hardware Mode Switches**:
    *   Toggle Switch 1: Selects between DAC (Remote/Software) and Potentiometer (Local/Manual) for `VSET`.
    *   Toggle Switch 2: Selects between USB and Ethernet transport.
*   **Safety Interlock**: The `/ON` pins of both iseg modules must be wired to a physical Emergency Stop (E-Stop) button and door interlock switches.

---

## 3. Safety & Protection Protocols (CRITICAL)

**⚠️ WARNING: This system generates lethal voltages. Always follow these software and hardware safeguards.**

### Hardware Safety
*   **Emergency Stop (E-Stop)**: Physically connected to the `/ON` pin of the iseg modules. Pressing this instantly cuts HV output, bypassing the software.
*   **Bleeder Resistors**: Permanently connected across the HV outputs to safely discharge capacitors when power is removed.
*   **Isolation**: The 12V supply is isolated. Chassis ground is common for all low-voltage electronics.
*   **Transient Protection**: TVS diodes and MOVs are used on the AC input and 12V DC bus to protect the RP2040 from HV switching spikes.

### Software Safety (Firmware)
*   **Soft Ramping**: The DAC output will **never** jump instantly. It ramps up and down at a configurable rate (default `10V/s`).
*   **Hardcoded Current Limit (`Imax`)**: A maximum current limit is hardcoded in `config.h`. The remote user can set a lower limit (`Iset`), but can **never** exceed `Imax`.
*   **Soft Shutdown**: If a fault occurs (Over-voltage, Over-current, Comms loss), the system does not instantly kill power (which causes dangerous inductive spikes). Instead, it enters a `FAULT` state and slowly ramps down to 0V.
*   **Watchdog Timer**: If the RP2040 firmware crashes or hangs, the hardware watchdog resets the MCU. Upon reboot, the default state is `0V`.
*   **Plausibility Checks**: The code constantly checks if `Vmon` and `Imon` are within realistic physical bounds. If a sensor fails or reads a stuck value, a fault is triggered.

---

## 4. Code Architecture

The project is modularized for safety and readability:
*   `control-system.ino`: Main orchestrator. Sets up hardware and runs the main loop.
*   `config.h`: Pin mappings, timing constants, voltage/current scaling factors, and hardcoded safety limits (`Imax`).
*   `state.h`: Defines the `SystemState` struct (holds voltages, currents, mode, and fault flags) and the State Machine (`IDLE`, `RAMPING`, `STEADY`, `FAULT`).
*   `hardware_i2c.cpp`: Handles all I2C communication. Reads ADCs and writes to the DAC.
*   `display_ui.cpp`: Draws the TFT screens (Startup, Live Telemetry, Trend) and refreshes at a high rate.
*   `serial_communication.cpp`: Handles USB Serial debugging and command parsing.
*   `ethernet_communication.cpp`: Handles the TCP server (Port 5025) and DHCP.

---

## 5. Local Operation & Display

When running in **Local Mode**, the hardware toggle switches determine the behavior.
The TFT display shows the following data continuously (refreshed every 1 second):
1.  `+Vset`: Positive target voltage (0-6000V).
2.  `-Vset`: Negative target voltage (0-6000V).
3.  `+Iset`: Maximum allowed positive current.
4.  `-Iset`: Maximum allowed negative current.
5.  `+Vmon`: Actual positive voltage output.
6.  `-Vmon`: Actual negative voltage output.
7.  `+Imon`: Actual positive current output.
8.  `-Imon`: Actual negative current output.
9.  `Mode`: Displays `LOCAL` (Potentiometer) or `REMOTE` (DAC).

*Note: If a spark is heard, immediately turn off the mains and rotate the potentiometer fully counter-clockwise to zero.*

---

## 6. Firmware Setup & Arduino IDE

### Prerequisites
1.  Install the **Arduino IDE**.
2.  Add the RP2040 board URL in Preferences:
    `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
3.  Install the following libraries via Library Manager:
    *   `Adafruit ADS1X15`
    *   `DFRobot_GP8XXX`
    *   `Adafruit GFX Library`
    *   `Adafruit ST7789`

### Compiling and Uploading
1.  Open `control-system.ino`.
2.  Select the board: **WIZnet W5100S-EVB-Pico**.
3.  Select the correct COM/USB port.
4.  Click **Upload**.

---

## 7. Remote Operation & Command Reference

When the hardware switch is set to **Remote**, the system accepts commands via **USB Serial (9600 baud)** or **Ethernet (TCP Port 5025)**.

*Note: The system operates on a fail-safe state machine. You cannot set voltages until the system is in the `IDLE` or `STEADY` state.*

### Command List
| Command | Description | Example |
| :--- | :--- | :--- |
| `CONF:TRAN?` | Query active transport. | Returns `TRANSPORT=ETHERNET` |
| `SOUR:VOLT<ch>,<val>` | Set target voltage (ramps up/down automatically). | `SOUR:VOLT1,2500` (Sets +2500V) |
| `SOUR:CURR<ch>,<val>` | Set current limit (Cannot exceed hardcoded `Imax`). | `SOUR:CURR2,100` (Sets -100uA limit) |
| `MEAS:VOLT?` | Read all measured voltages. | Returns `+Vmon=2499.5V, -Vmon=...` |
| `MEAS:CURR?` | Read all measured currents. | Returns `+Imon=12.4uA, -Imon=...` |
| `DAC?` | Read raw DAC values (Remote only). | Returns `+Vdac=2.08V, -Vdac=...` |
| `SYST:FAULT:CLEAR` | Clears the fault state and allows ramping to resume. | Returns `FAULT STATE CLEARED` |
| `DEBUG ON/OFF` | Enables/disables verbose serial telemetry. | `DEBUG ON` |

### Remote Control Rules
*   **No Instant Jumps**: If you send `SOUR:VOLT1,6000` while it is at 0V, the display will show `+Vset=6000`, but `+Vmon` will slowly climb at 10V/s until it reaches 6000V.
*   **Hardware Limit Protection**: If you send `SOUR:CURR1,9999`, but the hardcoded `Imax` is 470uA, the system will reject the command and return an `ERROR: CURRENT LIMIT EXCEEDED` message.
*   **Communication Loss**: If the Ethernet connection drops while voltage is active, a timer expires (e.g., 10 seconds), and the system automatically ramps down to 0V.

---

## 8. Debugging & Troubleshooting
*   **I2C Address Not Found**: Check the jumper wires. Long wires cause capacitance issues. Ensure SDA/SCL are not swapped.
*   **Screen Blank**: Check SPI wiring and `TFT_RST` pin.
*   **Voltage Not Ramping**: Check if the system is in a `FAULT` state. Send `SYST:FAULT:CLEAR` to reset.
*   **Spark Sounds**: Turn off immediately. Ensure the load is not drawing more current than the iseg module can supply. Reset the potentiometer to zero.

---
*Document Version: 1.0 | Target MCU: RP2040 (W5100S-EVB-Pico)*