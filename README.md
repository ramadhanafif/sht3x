# SHT3x Sensor Library for ESP

This is a ESP-IDF component for interfacing with SHT3x series temperature and humidity sensors using an ESP microcontroller.

This project was forked from [espressif/esp-iot-solution for sht3x](https://github.com/espressif/esp-iot-solution/tree/350117fb02941f9d0f3a2ccde53f8da108d2ee8a/components/sensors/humiture/sht3x).

Tested on ESP32-C3 with ESP-IDF v5.4.1. It should work on all ESP platforms.

The library assumes a 100KHz I2C device frequency.

## Example Usage

An example application is provided in the `example` directory, demonstrating how to initialize the I2C bus, create a sensor instance, and read temperature and humidity data in a loop.

See [`example/main/sht3x-example.c`](example/main/sht3x-example.c) for the example code.

```c
// Include necessary ESP-IDF headers and standard libraries
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>

// Include the SHT3x driver header
#include "freertos/idf_additions.h"
#include "sht3x.h"

void app_main(void) {
  // Configure the I2C master bus
  i2c_master_bus_config_t bus_cfg = {
      .clk_source = I2C_CLK_SRC_DEFAULT, // Use default clock source
      .i2c_port = I2C_NUM_0,             // Use I2C port 0
      .scl_io_num = GPIO_NUM_20,         // Set SCL GPIO pin
      .sda_io_num = GPIO_NUM_21,         // Set SDA GPIO pin
      .glitch_ignore_cnt = 7,          // Configure glitch filter
      /* .flags.enable_internal_pullup = true, // if you don’t have external
         pull-ups */
  };
  i2c_master_bus_handle_t bus_handle;
  // Initialize the I2C master bus
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
  // Small delay to allow the bus to stabilize
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Create an instance of the SHT3x sensor driver
  // The second parameter is the I2C address of the SHT3x sensor (0x44 or 0x45)
  sht3x_handle_t sht = sht3x_create(bus_handle, 0x44);

  float temp, hum;
  // Main loop to continuously read sensor data
  for (;;) {
    // Delay between readings
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Read temperature and humidity from the sensor
    ESP_ERROR_CHECK(sht3x_get_humiture(sht, &temp, &hum));
    // Log the temperature and humidity values
    ESP_LOGI("main", "temp: %f, hum: %f", temp, hum);
  }
}
```

**Note:** The example uses GPIO_NUM_20 for SCL and GPIO_NUM_21 for SDA. You may need to adjust these based on your specific ESP board.

Example Output:

```
I (12345) main: temp: 25.500000, hum: 50.000000
I (13345) main: temp: 25.550000, hum: 50.100000
I (14345) main: temp: 25.600000, hum: 50.200000
```

## License

This project is licensed under the Apache 2.0 License, following the espressif library noted above.