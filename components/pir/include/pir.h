#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#define PIR_OUT_GPIO GPIO_NUM_22

typedef enum
{
    PIR_DIRECTION_NORTH = 0,
    PIR_DIRECTION_EAST,
    PIR_DIRECTION_SOUTH,
    PIR_DIRECTION_WEST,
    PIR_DIRECTION_COUNT,
} pir_direction_t;

typedef void (*pir_event_cb_t)(pir_direction_t direction, void *user_ctx);

esp_err_t pir_init(pir_event_cb_t event_cb, void *user_ctx);
esp_err_t pir_simulate_trigger(pir_direction_t direction);
const char *pir_direction_to_name(pir_direction_t direction);
