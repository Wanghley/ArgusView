#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdint.h>
#include <stddef.h>

int send_image_http(const char *url, const uint8_t *image_data, size_t image_size);
int send_image_with_audio(const char *url, const uint8_t *image_data, size_t image_size, const uint8_t *audio_data, size_t audio_size);

#endif // HTTP_CLIENT_H
