# ArgusView Copilot Instructions  

## Purpose  
ArgusView is an accessibility device designed to assist visually impaired individuals by providing concise and clear audio descriptions of their surroundings.

* The backend is written in Typescript and uses the Express framework. Code for that is under the `backend` directory.
* The embedded code is written in C and uses the esp-idf (not Arduino) framework. Code for that is under the `embedded/ArgusView-Embedded` directory.
* If you need any extra information, please ask. If you need to add additional libraries that are not available on platformio (like those ones that are only available on espressif), let me know and I will add them to the project.
* Try to write most of the code comprising business logic in the backend. The embedded code should only be used for interfacing with the hardware and sending data to the backend.

# Embedded code specifics

* We're using a HiLetgo ESP32-CAM Camera Module board. The camera is a OV2640. Write code accordingly.
* [REALLY IMPORTANT]: For the embedded code, try to write business logic that calls an abstracted C API that we define locally. Then, write the implementation for that API under a src/esp32 folder (there's a small risk we might need to rewrite some of that code to RaspberryPI, but right now we're doing everything on esp32).
* [REALLY IMPORTANT]: Avoid at all costs calling espressif functions directly from the main.c file. The main.c file should only be used to initialize the hardware and call the abstracted API. Create additional files as needed to implement the API.
* Set up links and header files as needed.
* Overall, be a good programmer and write clean code.