#include <string.h>

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"

#include "x4hc595.h"

#define X4HC595_CHECK_RETURN(a, str, ret_val)                     \
    if (!(a)) {                                                   \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

#define X4HC595_CHECK_GOTO(a, str, label)                          \
    if (!(a)) {                                                    \
        ESP_LOGE(TAG, "%s:(%d): %s", __FUNCTION__, __LINE__, str); \
        goto label;                                                \
    }

#define X4HC595_CHECK_GOTO_WITH_OP(a, str, label, op)              \
    if (!(a)) {                                                    \
        ESP_LOGE(TAG, "%s:(%d): %s", __FUNCTION__, __LINE__, str); \
        op;                                                        \
        goto label;                                                \
    }

static const char* TAG = "x4hc595";

static esp_err_t x4hc595_oe_func_base(x4hc595_t* device, const uint32_t level) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    X4HC595_CHECK_RETURN(device->oe_ >= 0, "Output Enable Pin is offline", ESP_ERR_INVALID_ARG);
    gpio_set_level(device->oe_, level);
    device->is_output_enabled = !level;

    return ESP_OK;
}

esp_err_t x4hc595_init(x4hc595_t* device, const x4hc595_config_t* config) {
    X4HC595_CHECK_RETURN(config != NULL, "The pointer of config is NULL", ESP_ERR_INVALID_ARG);
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);
    X4HC595_CHECK_RETURN(config->num_devices > 0, "Number of devices must > 0", ESP_ERR_INVALID_ARG);

    esp_err_t ret = ESP_OK;

    device->sr_state.head = 0;
    device->sr_state.data_len = config->num_devices;
    device->sr_state.data = malloc(device->sr_state.data_len * sizeof(uint8_t));
    X4HC595_CHECK_RETURN(
        device->sr_state.data != NULL, "Memory allocation failed", ESP_ERR_NO_MEM
    );
    memset(device->sr_state.data, 0, device->sr_state.data_len * sizeof(uint8_t));

    device->lr_states = malloc(config->num_devices * sizeof(uint8_t));
    X4HC595_CHECK_GOTO_WITH_OP(
        device->lr_states != NULL, "Memory allocation failed", clean_up,
        ret = ESP_ERR_NO_MEM
    );
    memset(device->lr_states, 0, config->num_devices * sizeof(uint8_t));

    device->num_devices = config->num_devices;
    device->ds = config->ds;
    device->shcp = config->shcp;
    device->stcp = config->stcp;
    device->mr_ = config->mr_ >= 0 ? config->mr_ : -1;
    device->oe_ = config->oe_ >= 0 ? config->oe_ : -1;

    device->shcp_clk_delay_us = 0;
    device->stcp_clk_delay_us = 0;
    device->is_output_enabled = 1;

    gpio_config_t io_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (1) {
        io_config.pin_bit_mask = (1ULL << device->shcp) | (1ULL << device->stcp) | (1ULL << device->ds);
        ret = gpio_config(&io_config);
        X4HC595_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (device->oe_ >= 0) {
        io_config.pin_bit_mask = (1ULL << device->oe_);
        ret = gpio_config(&io_config);
        X4HC595_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }
    if (device->mr_ >= 0) {
        io_config.pin_bit_mask = (1ULL << device->mr_);
        ret = gpio_config(&io_config);
        X4HC595_CHECK_GOTO(ret == ESP_OK, "GPIO configuration failed", clean_up);
    }

    /*
     * Set the initial state of the 74HC595 output to 0 / High-Z.
     * Clear the 74HC595 shift register & output register, setting all stored bits to 0.
     */
    if (device->oe_ >= 0) { x4hc595_disable_output(device); }
    x4hc595_reset(device);

    return ESP_OK;

clean_up:
    free(device->sr_state.data);
    if (device->lr_states) { free(device->lr_states); }

    gpio_reset_pin(device->shcp);
    gpio_reset_pin(device->stcp);
    gpio_reset_pin(device->ds);
    if (device->oe_ >= 0) { gpio_reset_pin(device->oe_); }
    if (device->mr_ >= 0) { gpio_reset_pin(device->mr_); }

    return ret;
}

esp_err_t x4hc595_deinit(x4hc595_t* device) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    if (device->sr_state.data) {
        free(device->sr_state.data);
        device->sr_state.data = NULL;
    }
    if (device->lr_states) {
        free(device->lr_states);
        device->lr_states = NULL;
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
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    device->shcp_clk_delay_us = shcp_clk_delay_us;
    device->stcp_clk_delay_us = stcp_clk_delay_us;

    return ESP_OK;
}

esp_err_t x4hc595_write(x4hc595_t* device, const uint8_t data) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    /* LSB first */
    for (int i = 0; i < 8; i++) {
        gpio_set_level(device->ds, (data >> i) & 0x01);
        if (device->shcp_clk_delay_us > 0) { esp_rom_delay_us(device->shcp_clk_delay_us); }
        gpio_set_level(device->shcp, 1);
        if (device->shcp_clk_delay_us > 0) { esp_rom_delay_us(device->shcp_clk_delay_us); }
        gpio_set_level(device->shcp, 0);
    }

    device->sr_state.head = (device->sr_state.head + device->sr_state.data_len - 1) % device->sr_state.data_len;
    device->sr_state.data[device->sr_state.head] = data;

    return ESP_OK;
}

esp_err_t x4hc595_write_multiple(x4hc595_t* device, const uint8_t* data, const size_t len) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);
    X4HC595_CHECK_RETURN(data != NULL, "The pointer of data is NULL", ESP_ERR_INVALID_ARG);

    for (size_t i = 0; i < len; i++) {
        x4hc595_write(device, data[i]);
    }

    return ESP_OK;
}

esp_err_t x4hc595_write_to_index(x4hc595_t* device, const uint8_t data, const size_t index) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);
    X4HC595_CHECK_RETURN(index < device->num_devices, "The index is out of range", ESP_ERR_INVALID_ARG);

    uint8_t tmp[device->num_devices];

    /* Copy the current state of the shift register */
    for (int i = 0; i < device->num_devices; i++) {
        tmp[i] = device->sr_state.data[(device->sr_state.head + i) % device->sr_state.data_len];
    }

    /* Update the data at the specified index */
    tmp[index] = data;

    /* Write the updated data back to the shift register */
    for (int i = (int)device->num_devices - 1; i >= 0; i--) {
        x4hc595_write(device, tmp[i]);
    }

    return ESP_OK;
}

esp_err_t x4hc595_latch(const x4hc595_t* device) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    gpio_set_level(device->stcp, 1);
    if (device->stcp_clk_delay_us > 0) { esp_rom_delay_us(device->stcp_clk_delay_us); }
    gpio_set_level(device->stcp, 0);

    for (int i = 0; i < device->num_devices; i++) {
        device->lr_states[i] = device->sr_state.data[(device->sr_state.head + i) % device->sr_state.data_len];
    }

    return ESP_OK;
}

esp_err_t x4hc595_enable_output(x4hc595_t* device) {
    return x4hc595_oe_func_base(device, 0);
}

esp_err_t x4hc595_disable_output(x4hc595_t* device) {
    return x4hc595_oe_func_base(device, 1);
}

esp_err_t x4hc595_reset(x4hc595_t* device) {
    X4HC595_CHECK_RETURN(device != NULL, "The pointer of device is NULL", ESP_ERR_INVALID_ARG);

    if (device->mr_ >= 0) {
        gpio_set_level(device->mr_, 0);
        gpio_set_level(device->mr_, 1);
    } else {
        ESP_LOGI(TAG, "IC Master Reset Pin is offline, using software reset instead");
        x4hc595_write(device, 0x00);
        x4hc595_latch(device);
    }

    memset(device->sr_state.data, 0, device->sr_state.data_len * sizeof(uint8_t));
    memset(device->lr_states, 0, device->num_devices * sizeof(uint8_t));

    return ESP_OK;
}
