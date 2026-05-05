#include "radar.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>

#define RADAR_QUEUE_LENGTH 8
#define RADAR_TASK_STACK_SIZE 2048
#define RADAR_TASK_PRIORITY 10

static const char *TAG = "radar";

static QueueHandle_t s_radar_evt_queue;

static void radar_isr_handler(void *arg)
{
    const gpio_num_t gpio_num = (gpio_num_t)(intptr_t)arg;
    xQueueSendFromISR(s_radar_evt_queue, &gpio_num, NULL);
}

static void radar_task(void *arg)
{
    (void)arg;

    gpio_num_t gpio_num;

    while (1)
    {
        if (xQueueReceive(s_radar_evt_queue, &gpio_num, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "Radar target detected on GPIO:%d", gpio_num);
        }
    }
}

esp_err_t radar_init(void)
{
    s_radar_evt_queue = xQueueCreate(RADAR_QUEUE_LENGTH, sizeof(gpio_num_t));

    if (s_radar_evt_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    const gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << RADAR_OUT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        return ret;
    }

    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return ret;
    }

    ret = gpio_isr_handler_add(RADAR_OUT_GPIO, 
                             radar_isr_handler, 
                             (void *)(intptr_t)RADAR_OUT_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }

    BaseType_t task_ret = xTaskCreate(radar_task,
                                     "radar_task", 
                                     RADAR_TASK_STACK_SIZE,
                                     NULL, 
                                     RADAR_TASK_PRIORITY,
                                     NULL);
    if (task_ret != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
