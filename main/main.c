#include "esp_err.h"
#include "esp_log.h"

#include "radar.h"

void app_main(void)
{
    ESP_ERROR_CHECK(radar_init());
    ESP_LOGI("main", "Radar GPIO is GPIO:%d", RADAR_OUT_GPIO);
}
