#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"

void camera_init();
camera_fb_t* capture_image();
void release_image(camera_fb_t *fb);

#endif // CAMERA_H
