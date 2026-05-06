#include "pir.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdint.h>

#define PIR_QUEUE_LENGTH 8
#define PIR_TASK_STACK_SIZE 2048
#define PIR_TASK_PRIORITY 9

typedef struct
{
    pir_direction_t direction;
    gpio_num_t gpio_num;
} pir_evt_t;

static const char *TAG = "pir";

static QueueHandle_t s_pir_evt_queue;
static pir_event_cb_t s_event_cb;
static void *s_event_user_ctx;

// PIR 中断里只做一件事：把 GPIO 触发事件塞进队列。
// 不在 ISR 里打印日志，是因为中断上下文里做耗时操作容易影响系统实时性。
static void pir_isr_handler(void *arg)
{
    const pir_evt_t evt = {
        .direction = PIR_DIRECTION_NORTH,
        .gpio_num = (gpio_num_t)(intptr_t)arg,
    };

    xQueueSendFromISR(s_pir_evt_queue, &evt, NULL);
}

// PIR 任务相当于 Keil 裸机 while(1) 里处理外设标志位的那段逻辑。
// 区别是这里阻塞等待队列，不会空转占用 CPU。
static void pir_task(void *arg)
{
    (void)arg;

    pir_evt_t evt;

    while (1)
    {
        if (xQueueReceive(s_pir_evt_queue, &evt, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "PIR detected direction:%s GPIO:%d",
                     pir_direction_to_name(evt.direction),
                     evt.gpio_num);

            if (s_event_cb != NULL)
            {
                s_event_cb(evt.direction, s_event_user_ctx);
            }
        }
    }
}

esp_err_t pir_init(pir_event_cb_t event_cb, void *user_ctx)
{
    s_event_cb = event_cb;
    s_event_user_ctx = user_ctx;

    // 队列用于把“中断里发生了什么”安全地交给普通任务处理。
    s_pir_evt_queue = xQueueCreate(PIR_QUEUE_LENGTH, sizeof(pir_evt_t));
    if (s_pir_evt_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    // AM312 这类 PIR 模块通常输出数字电平：检测到人体热释电变化时输出高电平。
    const gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << PIR_OUT_GPIO,
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

    ret = gpio_isr_handler_add(PIR_OUT_GPIO,
                               pir_isr_handler,
                               (void *)(intptr_t)PIR_OUT_GPIO);
    if (ret != ESP_OK)
    {
        return ret;
    }

    const BaseType_t task_ret = xTaskCreate(pir_task,
                                            "pir_task",
                                            PIR_TASK_STACK_SIZE,
                                            NULL,
                                            PIR_TASK_PRIORITY,
                                            NULL);
    if (task_ret != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "PIR initialized on GPIO:%d", PIR_OUT_GPIO);

    return ESP_OK;
}

esp_err_t pir_simulate_trigger(pir_direction_t direction)
{
    if (direction >= PIR_DIRECTION_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_pir_evt_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    // 没有 PIR 实物时，直接往同一个队列里塞事件。
    // 后面的状态机不知道这是模拟事件还是硬件中断事件，学习时更容易分层。
    const pir_evt_t evt = {
        .direction = direction,
        .gpio_num = PIR_OUT_GPIO,
    };

    if (xQueueSend(s_pir_evt_queue, &evt, 0) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

const char *pir_direction_to_name(pir_direction_t direction)
{
    switch (direction)
    {
    case PIR_DIRECTION_NORTH:
        return "north";
    case PIR_DIRECTION_EAST:
        return "east";
    case PIR_DIRECTION_SOUTH:
        return "south";
    case PIR_DIRECTION_WEST:
        return "west";
    default:
        return "unknown";
    }
}
