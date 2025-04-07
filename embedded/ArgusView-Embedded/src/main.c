#include "esp32/wifi.h"
#include "image_handler.h"
#include "esp32/logger.h" // Include the new logging library

void app_main() {
    log_info("Starting application..."); // Log application start

    wifi_init_sta("DukeVisitor", ""); // Connect to Wi-Fi
    log_info("Wi-Fi initialized.");

    // // Capture and send the image
    // if (capture_and_send_image("http://BACKEND_URL:3000/describe?wordCount=20") != 0) {
    //     log_error("Failed to capture and send image."); // Log error
    // }
}