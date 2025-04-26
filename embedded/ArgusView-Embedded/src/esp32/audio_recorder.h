#ifndef AUDIO_RECORDER_H
#define AUDIO_RECORDER_H

#include <stdint.h>
#include <stddef.h>

int start_audio_recording();
int stop_audio_recording(uint8_t **audio_data, size_t *audio_size);

#endif // AUDIO_RECORDER_H
