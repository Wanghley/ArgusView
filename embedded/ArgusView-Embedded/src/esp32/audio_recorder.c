#include "audio_recorder.h"
#include "esp_log.h"
#include "driver/adc.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"
#include "wifi.h"

static const char *TAG = "AudioRecorder";

// I2S and audio configuration
#define I2S_NUM         I2S_NUM_0
#define SAMPLE_RATE     16000
#define SAMPLE_BITS     I2S_BITS_PER_SAMPLE_16BIT
#define CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     1024
#define BUFFER_SIZE     (SAMPLE_RATE * 2) // 1 second buffer
#define SAMPLE_INTERVAL_US (1000000 / SAMPLE_RATE) // Sampling interval in microseconds

// ADC channel selection (adjust according to your wiring!)
#define MIC_ADC_CHANNEL ADC2_CHANNEL_4

static uint8_t *audio_buffer = NULL;
static size_t buffer_size = 0;
static size_t total_audio_size = 0;
static TaskHandle_t recording_task_handle = NULL;
static volatile bool recording_active = false;

static void audio_recording_task(void *arg) {
    while (recording_active && total_audio_size < buffer_size) {
        int raw_sample = 0;
        esp_err_t result = adc2_get_raw(MIC_ADC_CHANNEL, ADC_WIDTH_BIT_12, &raw_sample);
        if (result == ESP_OK) {
            // Store the sample in the buffer
            audio_buffer[total_audio_size++] = raw_sample & 0xFF; // Low byte
            audio_buffer[total_audio_size++] = (raw_sample >> 8) & 0xFF; // High byte
        } else {
            log_warn(TAG, "adc2_get_raw() failed");
        }
        esp_rom_delay_us(SAMPLE_INTERVAL_US); // Delay to match the sampling rate
    }

    log_info(TAG, "Recording task finished, total recorded: %zu bytes", total_audio_size);
    vTaskDelete(NULL); // Task cleans itself up
}

int start_audio_recording() {
    if (recording_active) {
        log_warn(TAG, "Recording already in progress");
        return -1;
    }

    // Disable Wi-Fi to avoid conflicts with ADC2
    log_info(TAG, "Disabling Wi-Fi for audio recording.");
    pause_wifi();

    buffer_size = BUFFER_SIZE * 10; // 10 seconds of audio (adjust as needed)
    audio_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!audio_buffer) {
        log_error(TAG, "Failed to allocate audio buffer");
        resume_wifi(); // Re-enable Wi-Fi if allocation fails
        return -1;
    }

    // Configure ADC2 channel
    adc2_config_channel_atten(MIC_ADC_CHANNEL, ADC_ATTEN_DB_11);

    total_audio_size = 0;
    recording_active = true;

    // Start background recording task
    if (xTaskCreate(audio_recording_task, "audio_recorder_task", 4096, NULL, 5, &recording_task_handle) != pdPASS) {
        log_error(TAG, "Failed to create audio recording task");
        free(audio_buffer);
        audio_buffer = NULL;
        recording_active = false;
        resume_wifi(); // Re-enable Wi-Fi if task creation fails
        return -1;
    }

    log_info(TAG, "Audio recording started");
    return 0;
}

int stop_audio_recording(uint8_t **audio_data, size_t *audio_size) {
    if (!recording_active) {
        log_error(TAG, "Recording not active");
        return -1;
    }

    recording_active = false;

    // Wait for the background task to finish (should exit by itself)
    if (recording_task_handle != NULL) {
        while (eTaskGetState(recording_task_handle) != eDeleted) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        recording_task_handle = NULL;
    }

    *audio_data = audio_buffer;
    *audio_size = total_audio_size;

    log_info(TAG, "Audio recording stopped, total size: %zu bytes", total_audio_size);

    // Re-enable Wi-Fi after recording
    resume_wifi();
    log_info(TAG, "Wi-Fi re-enabled after audio recording.");

    return 0;
}
