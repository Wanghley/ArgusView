#include "audio_recorder.h"
#include "esp_log.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"

static const char *TAG = "AudioRecorder";

// I2S and audio configuration
#define I2S_NUM         I2S_NUM_0
#define SAMPLE_RATE     16000
#define SAMPLE_BITS     I2S_BITS_PER_SAMPLE_16BIT
#define CHANNEL_FORMAT  I2S_CHANNEL_FMT_ONLY_LEFT
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     1024
#define BUFFER_SIZE     (SAMPLE_RATE * 2) // 1 second buffer

// ADC channel selection (adjust according to your wiring!)
#define MIC_ADC_CHANNEL ADC2_CHANNEL_5

static uint8_t *audio_buffer = NULL;
static size_t buffer_size = 0;
static size_t total_audio_size = 0;
static TaskHandle_t recording_task_handle = NULL;
static volatile bool recording_active = false;

static void audio_recording_task(void *arg) {
    size_t bytes_read = 0;

    while (recording_active && total_audio_size < buffer_size) {
        esp_err_t result = i2s_read(I2S_NUM, audio_buffer + total_audio_size, buffer_size - total_audio_size, &bytes_read, 100 / portTICK_PERIOD_MS);
        if (result == ESP_OK && bytes_read > 0) {
            total_audio_size += bytes_read;
        } else {
            log_warn(TAG, "i2s_read() failed or no data read");
        }
    }

    log_info(TAG, "Recording task finished, total recorded: %zu bytes", total_audio_size);
    vTaskDelete(NULL); // Task cleans itself up
}

int start_audio_recording() {
    if (recording_active) {
        log_warn(TAG, "Recording already in progress");
        return -1;
    }

    buffer_size = BUFFER_SIZE * 10; // 10 seconds of audio (adjust as needed)
    audio_buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    if (!audio_buffer) {
        log_error(TAG, "Failed to allocate audio buffer");
        return -1;
    }

    // Configure I2S for ADC sampling
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN,
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = SAMPLE_BITS,
        .channel_format = CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = DMA_BUF_COUNT,
        .dma_buf_len = DMA_BUF_LEN,
        .use_apll = true, // For better clock precision
    };

    if (i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL) != ESP_OK) {
        log_error(TAG, "Failed to install I2S driver");
        free(audio_buffer);
        audio_buffer = NULL;
        return -1;
    }

    i2s_set_adc_mode(ADC_UNIT_1, MIC_ADC_CHANNEL);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(MIC_ADC_CHANNEL, ADC_ATTEN_DB_11);
    i2s_adc_enable(I2S_NUM);

    total_audio_size = 0;
    recording_active = true;

    // Start background recording task
    if (xTaskCreatePinnedToCore(audio_recording_task, "audio_recorder_task", 4096, NULL, 5, &recording_task_handle, 1) != pdPASS) {
        log_error(TAG, "Failed to create audio recording task");
        i2s_adc_disable(I2S_NUM);
        i2s_driver_uninstall(I2S_NUM);
        free(audio_buffer);
        audio_buffer = NULL;
        recording_active = false;
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

    // Clean up peripherals
    i2s_adc_disable(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);

    *audio_data = audio_buffer;
    *audio_size = total_audio_size;

    log_info(TAG, "Audio recording stopped, total size: %zu bytes", total_audio_size);
    return 0;
}
