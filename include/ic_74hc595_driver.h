#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @brief Configuration structure for x4HC595 shift register.
 */
typedef struct {
    gpio_num_t mr_;     /*!<         Master Reset         */
    gpio_num_t shcp;    /*!<  Shift Register Clock Pulse  */
    gpio_num_t stcp;    /*!< Storage Register Clock Pulse */
    gpio_num_t oe_;     /*!<        Output Enable         */
    gpio_num_t ds;      /*!<         Data Serial          */
    size_t num_devices; /*!<   Number of cascaded chips   */
} x4hc595_config_t;

/**
 * @brief Circular queue structure for storing x4HC595 device states.
 */
typedef struct {
    uint8_t* data;    /*!< Data array                           (储存数据的数组) */
    size_t data_len;  /*!< Count of data in the array           (数组中的数据数量) */
    size_t head;      /*!< Index of the first data in the array (数组中第一个数据的索引) */
    size_t data_size; /*!< Size of the data array (B)           (数据数组的大小) */
} x4hc595_state_queue_t;

/**
 * @brief Stores all relevant information for a particular x4HC595 device.
 */
typedef struct {
    gpio_num_t mr_;                       /*!< Master Reset                 (主复位) */
    gpio_num_t shcp;                      /*!< Shift Register Clock Pulse   (移位寄存器时钟脉冲) */
    gpio_num_t stcp;                      /*!< Storage Register Clock Pulse (存储寄存器时钟脉冲) */
    gpio_num_t oe_;                       /*!< Output Enable                (输出使能) */
    gpio_num_t ds;                        /*!< Data Serial                  (串行数据输入) */
    uint32_t shcp_clk_delay_us;           /*!< Clock timing delay for SHCP  (SHCP的时钟延迟) */
    uint32_t stcp_clk_delay_us;           /*!< Clock timing delay for STCP  (STCP的时钟延迟) */
    size_t num_devices;                   /*!< Number of cascaded devices   (级联芯片的数量) */
    uint32_t is_output_enabled;           /*!< Output enable status         (输出使能状态) */
    x4hc595_state_queue_t current_states; /*!< Current state of all devices (设备的当前状态) */
} x4hc595_t;

/**
 * @brief Initializes the x4HC595 device with the given configuration.
 *
 * @param[in]  config Pointer to the configuration structure for x4HC595 device.
 * @param[out] device Pointer to the x4hc595_t structure that will be initialized.
 *
 * @return
 * - `ESP_OK`               Successful initialization.
 * - `ESP_ERR_INVALID_ARG`  The configuration is invalid or `NULL`.
 * - `ESP_ERR_NO_MEM`       Memory allocation fails.
 * - Other error codes from the driver initialization.
 */
esp_err_t x4hc595_init(const x4hc595_config_t* config, x4hc595_t* device);

/**
 * @brief Deinitialize the x4HC595 device instance.
 *
 * @param[in] device Pointer to the x4HC595 device instance that needs to be deinitialized.
 *
 * @return
 * - `ESP_OK`               Successful deinitialization.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_deinit(const x4hc595_t* device);

/**
 * @brief Set the clock delay for the x4HC595 device.
 *
 * @param[in] device             Pointer to the x4HC595 device instance.
 * @param[in] shcp_clk_delay_us  Clock timing delay for SHCP.
 * @param[in] stcp_clk_delay_us  Clock timing delay for STCP.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_set_clock_delay(x4hc595_t* device, uint32_t shcp_clk_delay_us, uint32_t stcp_clk_delay_us);

/**
 * @brief Write a single byte of data to the x4HC595
 *
 * @param[in] device Pointer to the x4HC595 device instance.
 * @param[in] data   The 8-bit data to be written.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_write(x4hc595_t* device, uint8_t data);

/**
 * @brief Latch the shifted data into the output register
 *
 * @param[in] device Pointer to the x4HC595 device instance.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_latch(const x4hc595_t* device);

/**
 * @brief Enable the outputs of the x4HC595
 *
 * This function only works if the [oe_] is configured.
 *
 * @param[in] device Pointer to the x4HC595 device instance.
 *
 * @return
 * - `ESP_OK`                 Successful operation.
 * - `ESP_ERR_INVALID_ARG`    The handle is invalid or `NULL`.
 * - `ESP_ERR_NOT_SUPPORTED`  The Output Enable pin is not connected.
 */
esp_err_t x4hc595_enable_output(x4hc595_t* device);

/**
 * @brief Disable the outputs of the x4HC595
 *
 * This function only works if the [oe_] is configured.
 *
 * @param[in] device Pointer to the x4HC595 device instance.
 *
 * @return
 * - `ESP_OK`                 Successful operation.
 * - `ESP_ERR_INVALID_ARG`    The handle is invalid or `NULL`.
 * - `ESP_ERR_NOT_SUPPORTED`  The Output Enable pin is not connected.
 */
esp_err_t x4hc595_disable_output(x4hc595_t* device);

/**
 * @brief Resets the x4HC595
 *
 * This function clearing all the bits in the shift register and storage register.
 * If the [mr_] is connected, operation will be performed by driving the Master Reset pin,
 * otherwise, operation will be performed by sending zeros via the [ds] and toggling the [stcp].
 *
 * @param[in] device Pointer to the x4HC595 device instance.
 *
 * @return
 * - `ESP_OK`               Successful operation.
 * - `ESP_ERR_INVALID_ARG`  The handle is invalid or `NULL`.
 */
esp_err_t x4hc595_reset(x4hc595_t* device);
