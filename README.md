# 🚗 Smart Parking Assistance System – ATmega2560

An embedded-based Smart Parking Assistance System developed using the **ATmega2560 (Arduino Mega 2560)**. The system monitors parking conditions using IR and ultrasonic sensors and provides real-time feedback through a two-digit seven-segment display, a 16×2 LCD, PWM-controlled LEDs, and a buzzer.

The project uses modular driver files for peripherals such as GPIO, timers, ultrasonic sensing, PWM, keypad, LCD, and seven-segment display control.

## ✨ Features

- **Ultrasonic Distance Measurement:** Measures the distance to nearby objects and displays readings from 1–99 cm.
- **IR Object Detection:** Uses two IR sensors to detect objects and determine parking slot status.
- **Two-Digit Seven-Segment Display:** Displays the measured distance and shows `00` when IR1 detects an object.
- **16×2 LCD Display:** Shows the IR sensor states and distance-based safety messages.
- **PWM Status LEDs:** Uses red, yellow, and green LEDs to indicate parking availability.
- **Distance-Based Buzzer:** Changes the buzzer's tone and alert pattern according to the measured distance.
- **Keypad Control:** Supports buzzer muting, LED mode selection, red alert control, and LED testing.
- **Serial Monitor:** Prints sensor readings, parking availability, and system status.
- **IR Debouncing:** Uses a confirmation interval to reduce false state changes caused by sensor fluctuations.
- **Modular Driver Architecture:** Separates peripheral functionality into reusable header and source files.

## 🛠️ Hardware Components

| Component | Purpose |
|---|---|
| Arduino Mega 2560 / ATmega2560 | Main controller |
| HC-SR04 ultrasonic sensor | Distance measurement |
| 2 IR sensors | Object detection and parking status |
| Two-digit seven-segment display | Distance indication |
| 16×2 LCD | System status and warning messages |
| 4×4 keypad (if matching the configured driver) | User input and system control |
| Red LED | Parking unavailable / object alert |
| Yellow LED | One parking slot available |
| Green LED | Both parking slots available |
| PWM-compatible buzzer | Audible distance alerts |
| Resistors and jumper wires | Circuit connections |
| Breadboard or PCB | Circuit assembly |

## ⚙️ Software Requirements

- Arduino IDE
- ATmega2560 / Arduino Mega 2560 board support
- C/C++ compiler support through the Arduino toolchain
- Custom peripheral driver files used by the project

## 🧠 System Working Principle

### 1. Distance Measurement

The ultrasonic sensor measures the distance between the sensor and a nearby object. The result is updated periodically and displayed on the two-digit seven-segment display.

| Distance | System response |
|---|---|
| 1–10 cm | Continuous emergency buzzer tone |
| 11–15 cm | Very fast siren pattern |
| 16–30 cm | Fast siren pattern |
| 31–49 cm | Warning siren pattern |
| 50 cm and above | Buzzer off |
| No echo | Seven-segment display shows `99`; LCD reports `NO ECHO` |

The code limits displayed distance values to 99 cm. A zero-distance reading is treated as a missing echo.

### 2. IR Sensor and Parking Availability

The two IR sensors determine the parking status.

| IR1 | IR2 | Indication |
|---|---|---|
| Not detected | Not detected | 2 slots available — Green |
| Detected | Not detected | 1 slot available — Yellow |
| Not detected | Detected | 1 slot available — Yellow |
| Detected | Detected | 0 slots available — Red |

When IR1 detects an object, the seven-segment display shows `00`. The red LED performs three PWM breathing cycles when the automatic LED mode and red alert are enabled.

### 3. LCD Messages

The LCD displays the two IR sensor states on the first line and a distance-based status message on the second line.

Possible messages include:

- `IR1 OBJECT ALERT`
- `NO ECHO`
- `STOP! TOO CLOSE`
- `VERY CLOSE!`
- `CAUTION: SLOW`
- `SLOW DOWN`
- `SAFE DISTANCE`

### 4. LED Indication

The system uses PWM-controlled status LEDs.

- **Red:** No parking slots available.
- **Yellow:** One parking slot available.
- **Green:** Both parking slots available.

The code implements brightness transitions for the status LEDs and a three-cycle red LED breathing alert when IR1 detects an object.

### 5. Buzzer Alert

The buzzer provides distance-dependent audio feedback. The code uses different tone frequencies and timing patterns to indicate increasing proximity to an object.

The keypad can mute or unmute the buzzer.

### 6. Keypad Controls

| Key | Function |
|---|---|
| `1` | Enable red alert |
| `2` | Disable red alert |
| `3` | Mute buzzer |
| `B` | Unmute buzzer |
| `7` | Enable automatic LED mode |
| `C` | Toggle automatic/manual LED mode |
| `8` | Start red LED test |
| `9` or `#` | Stop red LED test |
| `*` | Reset settings to defaults |
| `0` | Print system status |
| `D` | Restore default automatic LED mode |

## 🏗️ Software Architecture

The project is organized around a main application and separate peripheral drivers.

| Driver / Module | Responsibility |
|---|---|
| `gpio.h` / `gpio.c` | GPIO configuration and control |
| `timer.h` / `timer.c` | Timer-related functions |
| `ultra.h` / `ultra.c` | Ultrasonic distance measurement |
| `two_segment.h` / corresponding source file | Two-digit seven-segment display |
| `pwm.h` / `pwm.c` | PWM buzzer control |
| `keypad.h` / corresponding source file | Keypad scanning and key detection |
| `lcd.h` / `lcd.c` | LCD initialization, cursor control, and text output |
| Main application `.ino` | Integrates sensors, display, alerts, and user controls |

*Note: Adjust the file names and extensions in this table to match the actual files in your repository.*

## 🔌 Connections

The following connections are explicitly configured in the main code:

| Device / Signal | Arduino Mega 2560 Pin |
|---|---|
| IR sensor 1 | D2 |
| IR sensor 2 | D3 |
| Red status LED PWM | D6 / PH3 / OC4A |
| Yellow status LED PWM | D7 / PH4 / OC4B |
| Green status LED PWM | D8 / PH5 / OC4C |
| Seven-segment display | Configured through `PORT_A` and `PORT_C` |
| Ultrasonic sensor | Configured in the ultrasonic driver |
| Buzzer | Configured in the PWM driver |
| Keypad | Configured in the keypad driver |
| LCD | Configured in the LCD driver |

**Important:** Confirm the remaining signal-to-pin mappings in your driver files before wiring the complete circuit. Use suitable current-limiting resistors for the LEDs and a suitable driver circuit if the buzzer exceeds the microcontroller pin's current limits.

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/Pravinkumarvs/ATMEGA_2560.git
```

### 2. Open the project

Open the project folder in Arduino IDE and locate the main `.ino` file.

### 3. Add the driver files

Ensure all required `.h` and source files are present in the project directory or in a location included by the Arduino build system.

Check that the file names used in the `#include` statements exactly match the names in your repository.

### 4. Select the board

In Arduino IDE, select:

- Board: **Arduino Mega or Mega 2560**
- Processor: **ATmega2560**
- Port: The COM port corresponding to your connected board

### 5. Upload the code

Compile the project, resolve any missing-driver or linker errors, and upload it to the Arduino Mega 2560.

### 6. Monitor the output

Open the Serial Monitor at **9600 baud** to observe IR detection states, available parking slots, and distance readings.

## 🧪 Testing

Test the system in stages:

1. Verify both IR sensors independently.
2. Test ultrasonic distance readings at known distances.
3. Verify the two-digit seven-segment display.
4. Check LCD messages for each distance range.
5. Test red, yellow, and green LED indications.
6. Verify the buzzer patterns at different distances.
7. Test every keypad command.
8. Test combined sensor conditions and check the reported parking availability.

## 🎯 Applications

- Smart parking assistance
- Small parking facility monitoring
- Embedded systems learning
- Sensor integration and peripheral driver development
- Microcontroller-based automation projects

## 🔮 Future Improvements

- Add additional parking slots and sensor modules.
- Introduce wireless monitoring using Wi-Fi or LoRa.
- Develop a mobile or web dashboard for parking availability.
- Add data logging and parking usage analytics.
- Improve fault detection for disconnected or malfunctioning sensors.

## 👨‍💻 Author

**Pravinkumar V S**

GitHub: [Pravinkumarvs](https://github.com/Pravinkumarvs)

## 📜 License

Choose a suitable open-source license before distributing or reusing this project. If no license is added, the repository's default copyright restrictions generally remain in effect.

---

*Built as an embedded systems project focused on microcontroller programming, peripheral drivers, sensor integration, and real-time parking assistance.*
