#include <string.h>

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

#include "ic_74hc595_driver.h"

#define IC_DRIVER_CHECK_RETURN(a, str, ret_val)                   \
    if (!(a)) {                                                   \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

#define IC_DRIVER_CHECK_GOTO(a, str, label)                        \
    if (!(a)) {                                                    \
        ESP_LOGE(TAG, "%s:(%d): %s", __FUNCTION__, __LINE__, str); \
        goto label;                                                \
    }

#define IC_DRIVER_CHECK_GOTO_WITH_OP(a, str, label, op)            \
    if (!(a)) {                                                    \
        ESP_LOGE(TAG, "%s:(%d): %s", __FUNCTION__, __LINE__, str); \
        op;                                                        \
        goto label;                                                \
    }

static const char* TAG = "ic_74hc595_driver";

static esp_err_t x4hc595_oe_func_base(x4hc595_t* device, const uint32_t level) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    IC_DRIVER_CHECK_RETURN(device->oe_ >= 0, "Output Enable Pin is offline", ESP_ERR_INVALID_ARG);
    gpio_set_level(device->oe_, level);

    return ESP_OK;
}

esp_err_t x4hc595_init(const x4hc595_config_t* config, x4hc595_t* device) {
    IC_DRIVER_CHECK_RETURN(config != NULL, "The pointer of config is NULL", ESP_ERR_INVALID_ARG);
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);
    IC_DRIVER_CHECK_RETURN(config->num_devices > 0, "Number of devices must >= 0", ESP_ERR_INVALID_ARG);

    device->current_states.head = 0;
    device->current_states.data_len = config->num_devices * 8;
    device->current_states.data_size = config->num_devices * sizeof(uint8_t);
    device->current_states.data = malloc(device->current_states.data_size);
    IC_DRIVER_CHECK_RETURN(
        device->current_states.data != NULL, "Memory allocation failed", ESP_ERR_NO_MEM
    );
    memset(device->current_states.data, 0, device->current_states.data_size);

    device->num_devices = config->num_devices;
    device->ds = config->ds;
    device->shcp = config->shcp;
    device->stcp = config->stcp;
    device->mr_ = config->mr_ >= 0 ? config->mr_ : -1;
    device->oe_ = config->oe_ >= 0 ? config->oe_ : -1;

    device->shcp_clk_delay_us = 0;
    device->stcp_clk_delay_us = 0;

    gpio_config_t io_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = ESP_OK;
    if (1) {
        io_config.pin_bit_mask = (1ULL << device->shcp) | (1ULL << device->stcp) | (1ULL << device->ds);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (device->oe_ >= 0) {
        io_config.pin_bit_mask = (1ULL << device->oe_);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (device->mr_ >= 0) {
        io_config.pin_bit_mask = (1ULL << device->mr_);
        ret = gpio_config(&io_config);
        IC_DRIVER_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }

    /*
     * Set the initial state of the 74HC595 output to 0 / High-Z.
     * Clear the 74HC595 shift register & output register, setting all stored bits to 0.
     */
    if (device->oe_ >= 0) { x4hc595_disable_output(device); }
    x4hc595_reset(device);

    return ESP_OK;

clean_up:
    if (device->current_states.data) {
        free(device->current_states.data);
    }
    return ret;
}

esp_err_t x4hc595_deinit(const x4hc595_t* device) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    if (device->current_states.data) {
        free(device->current_states.data);
    }

    gpio_reset_pin(device->shcp);
    gpio_reset_pin(device->stcp);
    gpio_reset_pin(device->ds);
    if (device->oe_ >= 0) { gpio_reset_pin(device->oe_); }
    if (device->mr_ >= 0) { gpio_reset_pin(device->mr_); }

    return ESP_OK;
}

esp_err_t x4hc595_set_clock_delay(
    x4hc595_t* device, const uint32_t shcp_clk_delay_us, const uint32_t stcp_clk_delay_us
) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    device->shcp_clk_delay_us = shcp_clk_delay_us;
    device->stcp_clk_delay_us = stcp_clk_delay_us;

    return ESP_OK;
}

esp_err_t x4hc595_write(x4hc595_t* device, const uint8_t data) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    // TODO: Update `current_states`

    /* LSB first */
    for (int i = 0; i < 8; i++) {
        gpio_set_level(device->ds, data >> i & 0x01);
        if (device->shcp_clk_delay_us > 0) { esp_rom_delay_us(device->shcp_clk_delay_us); }
        gpio_set_level(device->shcp, 1);
        if (device->shcp_clk_delay_us > 0) { esp_rom_delay_us(device->shcp_clk_delay_us); }
        gpio_set_level(device->shcp, 0);
    }

    return ESP_OK;
}

esp_err_t x4hc595_latch(const x4hc595_t* device) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    gpio_set_level(device->stcp, 1);
    if (device->stcp_clk_delay_us > 0) { esp_rom_delay_us(device->stcp_clk_delay_us); }
    gpio_set_level(device->stcp, 0);

    return ESP_OK;
}

esp_err_t x4hc595_enable_output(x4hc595_t* device) {
    return x4hc595_oe_func_base(device, 0);
}

esp_err_t x4hc595_disable_output(x4hc595_t* device) {
    return x4hc595_oe_func_base(device, 1);
}

esp_err_t x4hc595_reset(x4hc595_t* device) {
    IC_DRIVER_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    if (device->mr_ >= 0) {
        gpio_set_level(device->mr_, 0);
        gpio_set_level(device->mr_, 1);
    } else {
        ESP_LOGI(TAG, "IC Master Reset Pin is offline, using software reset instead");
        x4hc595_write(device, 0x00);
        x4hc595_latch(device);
    }

    return ESP_OK;
}
