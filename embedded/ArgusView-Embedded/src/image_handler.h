#ifndef IMAGE_HANDLER_H
#define IMAGE_HANDLER_H

#include <stdint.h>
#include <stddef.h>

int capture_and_send_image(const char *url);
int capture_and_send_image_with_audio(const char *url, const uint8_t *audio_data, size_t audio_size);

#endif // IMAGE_HANDLER_H
