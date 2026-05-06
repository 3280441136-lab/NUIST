#include "radar.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>

#define RADAR_QUEUE_LENGTH 8    //雷达GPIO信号队列长度
#define RADAR_TASK_STACK_SIZE 2048  //雷达任务栈
#define RADAR_TASK_PRIORITY 10  //雷达任务优先级

typedef struct
{
    radar_direction_t direction;
    gpio_num_t gpio_num;
} radar_evt_t;

static const char *TAG = "radar";   //雷达打印前缀名

static QueueHandle_t s_radar_evt_queue; //雷达GPIO信号队列
static radar_event_cb_t s_event_cb;
static void *s_event_user_ctx;

//雷达中断服务函数
static void radar_isr_handler(void *arg)
{
    const radar_evt_t evt = {
        .direction = RADAR_DIRECTION_NORTH,
        .gpio_num = (gpio_num_t)(intptr_t)arg,
    };

    xQueueSendFromISR(s_radar_evt_queue, &evt, NULL);
}

//雷达任务
static void radar_task(void *arg)   
{
    (void)arg;

    radar_evt_t evt;

    while (1)
    {
        if (xQueueReceive(s_radar_evt_queue, &evt, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "Radar target detected direction:%s GPIO:%d",
                     radar_direction_to_name(evt.direction),
                     evt.gpio_num);

            if (s_event_cb != NULL)
            {
                s_event_cb(evt.direction, s_event_user_ctx);
            }
        }
    }
}

//雷达初始化
esp_err_t radar_init(radar_event_cb_t event_cb, void *user_ctx)
{
    s_event_cb = event_cb;
    s_event_user_ctx = user_ctx;

    //创建雷达GPIO信号队列
    s_radar_evt_queue = xQueueCreate(RADAR_QUEUE_LENGTH, sizeof(radar_evt_t));

    if (s_radar_evt_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    //配置雷达GPIO口
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

    //安装雷达GPIO口中断
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        return ret;
    }

    //添加雷达GPIO口中断服务函数,并给中断服务函数传参
    ret = gpio_isr_handler_add(RADAR_OUT_GPIO, 
                             radar_isr_handler, 
                             (void *)(intptr_t)RADAR_OUT_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }

    //创建雷达任务
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

esp_err_t radar_simulate_trigger(radar_direction_t direction)
{
    if (direction >= RADAR_DIRECTION_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_radar_evt_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    const radar_evt_t evt = {
        .direction = direction,
        .gpio_num = RADAR_OUT_GPIO,
    };

    if (xQueueSend(s_radar_evt_queue, &evt, 0) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

const char *radar_direction_to_name(radar_direction_t direction)
{
    switch (direction)
    {
    case RADAR_DIRECTION_NORTH:
        return "north";
    case RADAR_DIRECTION_EAST:
        return "east";
    case RADAR_DIRECTION_SOUTH:
        return "south";
    case RADAR_DIRECTION_WEST:
        return "west";
    default:
        return "unknown";
    }
}
