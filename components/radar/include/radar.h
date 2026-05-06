#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#define RADAR_OUT_GPIO GPIO_NUM_21

typedef enum
{
    RADAR_DIRECTION_NORTH = 0,
    RADAR_DIRECTION_EAST,
    RADAR_DIRECTION_SOUTH,
    RADAR_DIRECTION_WEST,
    RADAR_DIRECTION_COUNT,
} radar_direction_t;

typedef void (*radar_event_cb_t)(radar_direction_t direction, void *user_ctx);

esp_err_t radar_init(radar_event_cb_t event_cb, void *user_ctx);
esp_err_t radar_simulate_trigger(radar_direction_t direction);
const char *radar_direction_to_name(radar_direction_t direction);
