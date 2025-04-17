#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stddef.h>
#include "esp_err.h"

esp_err_t init_audio_player();
esp_err_t play_audio(const uint8_t *audio_data, size_t audio_size);

#endif // AUDIO_PLAYER_H
