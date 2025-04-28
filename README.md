# ArgusView

ArgusView is an accessibility device designed to assist visually impaired individuals by providing concise audio descriptions of their surroundings.

## Project Structure

The project is organized into three main components:

### `/embedded`
Contains the ESP32-CAM based firmware that captures images and sends them to the backend for processing.

* The code is written in C using the ESP-IDF framework.
* Key components include camera interfacing, connectivity, and button handling
* The implementation for the high-level functions is found in `src/esp32`

### `/backend`
Houses the TypeScript/Express server responsible for:
- Receiving images from the ESP32-CAM
- Processing images for description generation
- Converting text descriptions to audio
- Sending audio back to the device

### `/frontend`
A quick web interface for intercepting the requests from the ESP32-CAM and displaying the results.
