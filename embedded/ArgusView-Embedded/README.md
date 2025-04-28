# ArgusView Embedded

This directory contains the embedded code for the ArgusView device, which runs on an ESP32-CAM module.

## Overview

The embedded component is responsible for:
- Capturing images using the OV2640 camera
- Processing button inputs for user interaction
- Connecting to Wi-Fi and communicating with the backend server
- Managing power and system resources

## Hardware

- **Board**: HiLetgo ESP32-CAM Camera Module
- **Camera**: OV2640 (2MP camera with JPEG output)
- **Interface Buttons**: For user input and control
- **Power**: Battery-powered with power management

## Code Structure

- `main/`: Contains the application entry point and initialization code
- `src/esp32/`: Contains the ESP32-specific implementation of the common API
- `components/`: External components and libraries (ESP32-Camera, Button, etc.)

## Building and Flashing

This project uses the ESP-IDF framework. To build and flash:

1. Set up ESP-IDF environment variables:
   ```
   . $IDF_PATH/export.sh
   ```

2. Configure the project (first time only):
   ```
   idf.py menuconfig
   ```
   - Set WiFi credentials
   - Configure camera pins
   - Set backend server details

3. Build and flash:
   ```
   idf.py build
   idf.py -p (PORT) flash
   ```

4. Monitor logs:
   ```
   idf.py -p (PORT) monitor
   ```

## Architecture

The code follows a clean architecture pattern with abstracted interfaces to ensure portability. Direct ESP-IDF calls are isolated to the `src/esp32` directory to allow for potential migration to other platforms in the future.
