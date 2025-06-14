/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sht3x.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"

typedef struct {
  i2c_master_dev_handle_t i2c_dev;
  uint8_t dev_addr;
} sht3x_sensor_t;

sht3x_handle_t sht3x_create(i2c_master_bus_handle_t bus, uint8_t dev_addr) {
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = dev_addr,
      .scl_speed_hz = 100000,
  };

  sht3x_sensor_t *sens = (sht3x_sensor_t *)calloc(1, sizeof(sht3x_sensor_t));
  i2c_master_bus_add_device(bus, &dev_cfg, &sens->i2c_dev);
  if (sens->i2c_dev == NULL) {
    free(sens);
    return NULL;
  }
  sens->dev_addr = dev_addr;
  return (sht3x_handle_t)sens;
}

esp_err_t sht3x_delete(sht3x_handle_t *sensor) {
  if (*sensor == NULL) {
    return ESP_OK;
  }

  sht3x_sensor_t *sens = (sht3x_sensor_t *)(*sensor);
  i2c_master_bus_rm_device(sens->i2c_dev);
  free(sens);
  *sensor = NULL;
  return ESP_OK;
}

static esp_err_t sht3x_write_cmd(sht3x_handle_t sensor,
                                 sht3x_cmd_measure_t sht3x_cmd) {
  sht3x_sensor_t *sens = (sht3x_sensor_t *)sensor;
  uint8_t cmd_buffer[2];
  cmd_buffer[0] = sht3x_cmd >> 8;
  cmd_buffer[1] = sht3x_cmd;
  esp_err_t ret = i2c_master_transmit(sens->i2c_dev, cmd_buffer, 2, -1);
  return ret;
}

static esp_err_t sht3x_get_data(sht3x_handle_t sensor, uint8_t data_len,
                                uint8_t *data_arr) {
  sht3x_sensor_t *sens = (sht3x_sensor_t *)sensor;
  esp_err_t ret = i2c_master_receive(sens->i2c_dev, data_arr, data_len, -1);
  return ret;
}

static uint8_t CheckCrc8(uint8_t *const message, uint8_t initial_value) {
  uint8_t crc;
  int i = 0, j = 0;
  crc = initial_value;

  for (j = 0; j < 2; j++) {
    crc ^= message[j];
    for (i = 0; i < 8; i++) {
      if (crc & 0x80) {
        crc =
            (crc << 1) ^ 0x31; /*!< 0x31 is Polynomial for 8-bit CRC checksum*/
      } else {
        crc = (crc << 1);
      }
    }
  }

  return crc;
}

esp_err_t sht3x_measure_period(bool set, uint16_t *min_delay) {
  static uint16_t s_min_delay = 250;

  // get value
  if (!set) {
    *min_delay = s_min_delay;
    return ESP_OK;
  }

  // set value
  uint8_t delay_mode = *min_delay >> 8;
  switch (delay_mode) {
  case 0x20:
    s_min_delay = 2000;
    break;
  case 0x21:
    s_min_delay = 1000;
    break;
  case 0x22:
    s_min_delay = 500;
    break;
  case 0x24:
    s_min_delay = 250;
    break;
  case 0x27:
    s_min_delay = 100;
    break;
  default:
    s_min_delay = 250;
    break;
  }
  return ESP_OK;
}

esp_err_t sht3x_get_humiture(sht3x_handle_t sensor, float *Tem_val,
                             float *Hum_val) {
  uint8_t buff[6];
  uint16_t tem, hum;
  float Temperature = 0;
  float Humidity = 0;

  sht3x_write_cmd(sensor,
                  READOUT_FOR_PERIODIC_MODE); /*!< if you want to read data just
                                                 onetime, Comment this code*/
  sht3x_get_data(sensor, 6, buff);

  /* check crc */
  if (CheckCrc8(buff, 0xFF) != buff[2] ||
      CheckCrc8(&buff[3], 0xFF) != buff[5]) {
    return ESP_FAIL;
  }

  tem = (((uint16_t)buff[0] << 8) | buff[1]);
  Temperature = (175.0 * (float)tem / 65535.0 -
                 45.0); /*!< T = -45 + 175 * tem / (2^16-1), this temperature
                           conversion formula is for Celsius °C */
  // Temperature= (315.0*(float)tem/65535.0-49.0) ;     /*!< T = -45 + 175 * tem
  // / (2^16-1), this temperature conversion formula is for Fahrenheit °F */
  hum = (((uint16_t)buff[3] << 8) | buff[4]);
  Humidity = (100.0 * (float)hum / 65535.0); /*!< RH = hum*100 / (2^16-1) */

  if ((Temperature >= -20) && (Temperature <= 125) && (Humidity >= 0) &&
      (Humidity <= 100)) {
    *Tem_val = Temperature;
    *Hum_val = Humidity;
    return ESP_OK; /*!< here is measurement range */
  } else {
    return ESP_FAIL;
  }
}

esp_err_t sht3x_get_single_shot(sht3x_handle_t sensor, float *Tem_val,
                                float *Hum_val) {
  uint8_t buff[6];
  uint16_t tem, hum;
  static float Temperature = 0;
  static float Humidity = 0;
  static int64_t last_shot_time = 0;
  int64_t current_time = esp_timer_get_time();
  uint16_t min_delay = 0;
  int64_t min_delay_us = 0;
  sht3x_measure_period(false, &min_delay);
  min_delay_us = min_delay * 1000;

  if ((current_time - last_shot_time) < min_delay_us) {
    *Tem_val = Temperature;
    *Hum_val = Humidity;
    return ESP_OK;
  }

  esp_err_t ret = sht3x_get_data(sensor, 6, buff);

  /* check crc */
  if (ret != ESP_OK || CheckCrc8(buff, 0xFF) != buff[2] ||
      CheckCrc8(&buff[3], 0xFF) != buff[5]) {
    return ESP_FAIL;
  }

  last_shot_time = current_time;
  tem = (((uint16_t)buff[0] << 8) | buff[1]);
  Temperature = (175.0 * (float)tem / 65535.0 -
                 45.0); /*!< T = -45 + 175 * tem / (2^16-1), this temperature
                           conversion formula is for Celsius °C */
  // Temperature= (315.0*(float)tem/65535.0-49.0) ;     /*!< T = -45 + 175 * tem
  // / (2^16-1), this temperature conversion formula is for Fahrenheit °F */
  hum = (((uint16_t)buff[3] << 8) | buff[4]);
  Humidity = (100.0 * (float)hum / 65535.0); /*!< RH = hum*100 / (2^16-1) */

  if ((Temperature >= -20) && (Temperature <= 125) && (Humidity >= 0) &&
      (Humidity <= 100)) {
    *Tem_val = Temperature;
    *Hum_val = Humidity;
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
}

esp_err_t sht3x_soft_reset(sht3x_handle_t sensor) {
  esp_err_t ret = sht3x_write_cmd(sensor, SOFT_RESET_CMD);
  return ret;
}

esp_err_t sht3x_stop_periodic(sht3x_handle_t sensor) {
  esp_err_t ret = sht3x_write_cmd(sensor, SHT3x_STOP_PERIODIC);
  return ret;
}

esp_err_t sht3x_art(sht3x_handle_t sensor) {
  esp_err_t ret = sht3x_write_cmd(sensor, SHT3x_ART_CMD);
  return ret;
}

esp_err_t sht3x_set_measure_mode(sht3x_handle_t sensor,
                                 sht3x_cmd_measure_t sht3x_measure_mode) {
  esp_err_t ret = sht3x_write_cmd(sensor, sht3x_measure_mode);
  sht3x_measure_period(true, (uint16_t *)&sht3x_measure_mode);
  return ret;
}

esp_err_t sht3x_heater(sht3x_handle_t sensor,
                       sht3x_cmd_measure_t sht3x_heater_condition) {
  esp_err_t ret = sht3x_write_cmd(sensor, sht3x_heater_condition);
  return ret;
}

#ifdef CONFIG_SENSOR_INCLUDED_HUMITURE

static sht3x_handle_t sht3x = NULL;
static bool is_init = false;

esp_err_t humiture_sht3x_init(i2c_bus_handle_t i2c_bus, uint8_t addr) {
  if (is_init || !i2c_bus) {
    return ESP_FAIL;
  }

  sht3x = sht3x_create(i2c_bus, addr);

  if (!sht3x) {
    return ESP_FAIL;
  }

  esp_err_t ret = sht3x_set_measure_mode(
      sht3x, SHT3x_PER_4_MEDIUM); /**medium accuracy/repeatability with 250ms
                                     period (1000ms/4)**/

  if (ret != ESP_OK) {
    return ESP_FAIL;
  }

  is_init = true;
  return ESP_OK;
}

esp_err_t humiture_sht3x_deinit(void) {
  if (!is_init) {
    return ESP_FAIL;
  }

  esp_err_t ret = sht3x_delete(&sht3x);

  if (ret != ESP_OK) {
    return ESP_FAIL;
  }

  is_init = false;
  return ESP_OK;
}

esp_err_t humiture_sht3x_test(void) {
  if (!is_init) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t humiture_sht3x_acquire_humidity(float *h) {
  if (!is_init) {
    return ESP_FAIL;
  }

  float temperature = 0;
  float humidity = 0;
  esp_err_t ret = sht3x_get_single_shot(sht3x, &temperature, &humidity);

  if (ret == ESP_OK) {
    *h = humidity;
    return ESP_OK;
  }
  *h = 0;
  return ESP_FAIL;
}

esp_err_t humiture_sht3x_acquire_temperature(float *t) {
  if (!is_init) {
    return ESP_FAIL;
  }

  float temperature = 0;
  float humidity = 0;
  esp_err_t ret = sht3x_get_single_shot(sht3x, &temperature, &humidity);

  if (ret == ESP_OK) {
    *t = temperature;
    return ESP_OK;
  }

  *t = 0;
  return ESP_FAIL;
}

static humiture_impl_t sht3x_impl = {
    .init = humiture_sht3x_init,
    .deinit = humiture_sht3x_deinit,
    .test = humiture_sht3x_test,
    .acquire_humidity = humiture_sht3x_acquire_humidity,
    .acquire_temperature = humiture_sht3x_acquire_temperature,
};

SENSOR_HUB_DETECT_FN(HUMITURE_ID, sht3x, &sht3x_impl);

#endif
