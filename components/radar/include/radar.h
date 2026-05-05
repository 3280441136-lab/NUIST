#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#define RADAR_OUT_GPIO GPIO_NUM_21

esp_err_t radar_init(void);
