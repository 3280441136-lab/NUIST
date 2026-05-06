#pragma once

#include "esp_err.h"
#include "pir.h"
#include "radar.h"

esp_err_t intrusion_init(void);
void intrusion_on_radar_event(radar_direction_t direction);
void intrusion_on_pir_event(pir_direction_t direction);
