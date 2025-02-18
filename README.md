# Shift Registers 74HC595 ESP-IDF Library

ESP-IDF component of 74HC595 8-Bit Shift Registers With 3-State Output Registers.

## Features

- Support single or multiple cascaded 74HC595 chips
- Provide single-byte and multi-bytes write functions
- Support data write at specified position
- Flexible clock delay configuration
- Complete error handling

## Add component to your project

Please add the following line to your `idf_component.yml`:

```yaml
dependencies:
  esp-idf-74hc595:
    git: https://github.com/davidli218/esp-idf-74hc595.git
    version: "*"
```

## Basic Usage

```c
#include "esp_err.h"
#include "driver/gpio.h"

#include "x4hc595.h"

/* Configuration for 74HC595 */
const x4hc595_config_t config = {
    .ds = GPIO_NUM_2,    // Data Serial Pin
    .shcp = GPIO_NUM_15, // Shift Register Clock Pin
    .stcp = GPIO_NUM_16, // Storage Register Clock Pin
    .mr_ = GPIO_NUM_17,  // Master Reset Pin (optional, set to -1 to disable)
    .oe_ = GPIO_NUM_18,  // Output Enable Pin (optional, set to -1 to disable)
    .num_devices = 1     // Number of cascaded 74HC595 chips
};

void app_main(void) {
    x4hc595_t hc595;

    /* Initialize 74HC595 instance */
    ESP_ERROR_CHECK(x4hc595_init(&hc595, &config));

    /* Write data to 74HC595 */
    ESP_ERROR_CHECK(x4hc595_write(&hc595, 0xFF)); // Write 0xFF to 74HC595
    ESP_ERROR_CHECK(x4hc595_latch(&hc595));       // Latch data to output
}

```
