#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "intrusion.h"
#include "pir.h"
#include "radar.h"

static const char *TAG = "main";

static void radar_event_handler(radar_direction_t direction, void *user_ctx)
{
    (void)user_ctx;
    intrusion_on_radar_event(direction);
}

static void pir_event_handler(pir_direction_t direction, void *user_ctx)
{
    (void)user_ctx;
    intrusion_on_pir_event(direction);
}

static void demo_task(void *arg)
{
    (void)arg;

    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "demo: simulate radar north trigger");
    ESP_ERROR_CHECK(radar_simulate_trigger(RADAR_DIRECTION_NORTH));

    vTaskDelay(pdMS_TO_TICKS(800));
    ESP_LOGI(TAG, "demo: simulate PIR north confirm");
    ESP_ERROR_CHECK(pir_simulate_trigger(PIR_DIRECTION_NORTH));

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(intrusion_init());
    ESP_ERROR_CHECK(radar_init(radar_event_handler, NULL));
    ESP_ERROR_CHECK(pir_init(pir_event_handler, NULL));

    ESP_LOGI(TAG, "Radar GPIO is GPIO:%d", RADAR_OUT_GPIO);
    ESP_LOGI(TAG, "PIR GPIO is GPIO:%d", PIR_OUT_GPIO);
    ESP_LOGI(TAG, "hardware is not required now, demo task will simulate one double-trigger flow");

    const BaseType_t task_ret = xTaskCreate(demo_task,
                                            "demo_task",
                                            2048,
                                            NULL,
                                            5,
                                            NULL);
    if (task_ret != pdPASS)
    {
        ESP_LOGE(TAG, "failed to create demo task");
    }
}
