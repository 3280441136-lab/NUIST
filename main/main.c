#include "esp_err.h"
#include "radar.h"

void app_main(void)
{
    ESP_ERROR_CHECK(radar_init());
}
