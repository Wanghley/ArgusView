#include "audio_player.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"

// Removed unused TAG variable
// static const char *TAG = "AudioPlayer";

#define PWM_GPIO 12
#define PWM_FREQ 24000 // 24 kHz for audio playback
#define PWM_RES LEDC_TIMER_10_BIT

esp_err_t play_audio(const uint8_t *audio_data, size_t audio_size) {
    // Initialize PWM
    ledc_timer_config_t pwm_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RES,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&pwm_timer));

    ledc_channel_config_t pwm_channel = {
        .gpio_num = PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&pwm_channel));

    // Process PCM data
    for (size_t i = 0; i < audio_size; i += 2) {
        // Convert 16-bit signed PCM to unsigned 10-bit duty cycle
        int16_t sample = (audio_data[i + 1] << 8) | audio_data[i]; // Little-endian
        uint32_t duty = (sample + 32768) * ((1 << PWM_RES) - 1) / 65535;

        // Set PWM duty cycle
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

        // Delay to match the sample rate
        esp_rom_delay_us(1000000 / PWM_FREQ); // Replaced ets_delay_us
    }

    return ESP_OK;
}
