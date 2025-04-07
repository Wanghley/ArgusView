#include "esp32/wifi.h"
#include "image_handler.h"

void app_main() {
    wifi_init_sta("DukeVisitor", ""); // Connect to Wi-Fi

    // // Capture and send the image
    // if (capture_and_send_image("http://BACKEND_URL:3000/describe?wordCount=20") != 0) {
    //     // Handle error
    // }
}