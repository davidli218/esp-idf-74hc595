#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @brief Configuration structure for x4HC595 driver
 */
typedef struct {
    gpio_num_t mr_;  /*!<         Master Reset         */
    gpio_num_t shcp; /*!<  Shift Register Clock Pulse  */
    gpio_num_t stcp; /*!< Storage Register Clock Pulse */
    gpio_num_t oe_;  /*!<        Output Enable         */
    gpio_num_t ds;   /*!<         Data Serial          */
} x4hc595_config_t;

/**
 * @brief Handle type for x4HC595 driver
 */
typedef void* x4hc595_handle_t;

/**
 * @brief Initialize the x4HC595 driver
 *
 * @param[in]  config Pointer to the configuration structure for x4HC595 driver.
 * @param[out] handle Pointer to a handle that will be initialized.
 *
 * @return
 * - `ESP_OK`               Successful initialization.
 * - `ESP_ERR_INVALID_ARG`  The configuration is invalid or `NULL`.
 * - `ESP_ERR_NO_MEM`       Memory allocation fails.
 * - Other error codes from the driver initialization.
 */
esp_err_t x4hc595_init(const x4hc595_config_t* config, x4hc595_handle_t* handle);

/**
 * @brief Deinitialize the x4HC595 driver
 *
 * @param[in,out] handle Pointer to the handle that needs to be deinitialized.
 *
 * @return
 * - `ESP_OK`               Successful deinitialization.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_deinit(x4hc595_handle_t* handle);

/**
 * @brief Set the clock delay for the x4HC595
 *
 * @param[in] handle          Handle to the x4HC595 driver instance.
 * @param[in] shcp_clk_delay_us  Clock timing delay for SHCP.
 * @param[in] stcp_clk_delay_us  Clock timing delay for STCP.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_set_clock_delay(x4hc595_handle_t handle, uint32_t shcp_clk_delay_us, uint32_t stcp_clk_delay_us);

/**
 * @brief Write a single byte of data to the x4HC595
 *
 * @param[in] handle Handle to the x4HC595 driver instance.
 * @param[in] data   The 8-bit data to be written.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_write(x4hc595_handle_t handle, uint8_t data);

/**
 * @brief Latch the shifted data into the output register
 *
 * @param[in] handle Handle to the x4HC595 driver instance.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_latch(x4hc595_handle_t handle);

/**
 * @brief Enable the outputs of the x4HC595
 *
 * This function only works if the [oe_] is configured.
 *
 * @param[in] handle Handle to the x4HC595 driver instance.
 *
 * @return
 * - `ESP_OK`                 Successful operation.
 * - `ESP_ERR_INVALID_ARG`    The handle is invalid or `NULL`.
 * - `ESP_ERR_NOT_SUPPORTED`  The Output Enable pin is not connected.
 */
esp_err_t x4hc595_enable_output(x4hc595_handle_t handle);

/**
 * @brief Disable the outputs of the x4HC595
 *
 * This function only works if the [oe_] is configured.
 *
 * @param[in] handle Handle to the x4HC595 driver instance.
 *
 * @return
 * - `ESP_OK`                 Successful operation.
 * - `ESP_ERR_INVALID_ARG`    The handle is invalid or `NULL`.
 * - `ESP_ERR_NOT_SUPPORTED`  The Output Enable pin is not connected.
 */
esp_err_t x4hc595_disable_output(x4hc595_handle_t handle);

/**
 * @brief Resets the x4HC595
 *
 * This function clearing all the bits in the shift register and storage register.
 * If the [mr_] is connected, operation will be performed by driving the Master Reset pin,
 * otherwise, operation will be performed by sending zeros via the [ds] and toggling the [stcp].
 *
 * @param[in] handle Handle to the x4HC595 driver instance.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_reset(x4hc595_handle_t handle);
