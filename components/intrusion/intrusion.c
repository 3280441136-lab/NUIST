#include "intrusion.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define INTRUSION_QUEUE_LENGTH 12
#define INTRUSION_TASK_STACK_SIZE 3072
#define INTRUSION_TASK_PRIORITY 8
#define PIR_CONFIRM_TIMEOUT_MS 3000

typedef enum
{
    INTRUSION_EVENT_RADAR = 0,
    INTRUSION_EVENT_PIR,
} intrusion_event_type_t;

typedef struct
{
    intrusion_event_type_t type;
    int direction;
} intrusion_event_t;

typedef enum
{
    INTRUSION_STATE_IDLE = 0,
    INTRUSION_STATE_WAIT_PIR,
    INTRUSION_STATE_VISION_CONFIRM,
} intrusion_state_t;

static const char *TAG = "intrusion";

static QueueHandle_t s_intrusion_evt_queue;

static const char *direction_to_name(int direction)
{
    switch (direction)
    {
    case 0:
        return "north";
    case 1:
        return "east";
    case 2:
        return "south";
    case 3:
        return "west";
    default:
        return "unknown";
    }
}

static void log_state_change(intrusion_state_t from, intrusion_state_t to)
{
    ESP_LOGI(TAG, "state %d -> %d", from, to);
}

// 入侵检测状态机是本项目的“业务大脑”。
// 传感器驱动只上报事件，是否可信、是否进入视觉确认，都在这里统一判断。
static void intrusion_task(void *arg)
{
    (void)arg;

    intrusion_state_t state = INTRUSION_STATE_IDLE;
    int pending_direction = -1;
    intrusion_event_t evt;

    ESP_LOGI(TAG, "state machine started, current state: IDLE");

    while (1)
    {
        TickType_t wait_ticks = portMAX_DELAY;

        // 只有雷达先触发后，才给 PIR 一个确认窗口。
        // 超过这个时间还没有同方向 PIR，就先当作疑似误报处理。
        if (state == INTRUSION_STATE_WAIT_PIR)
        {
            wait_ticks = pdMS_TO_TICKS(PIR_CONFIRM_TIMEOUT_MS);
        }

        if (xQueueReceive(s_intrusion_evt_queue, &evt, wait_ticks) == pdTRUE)
        {
            if (evt.type == INTRUSION_EVENT_RADAR)
            {
                ESP_LOGI(TAG, "radar event direction:%s", direction_to_name(evt.direction));

                pending_direction = evt.direction;
                log_state_change(state, INTRUSION_STATE_WAIT_PIR);
                state = INTRUSION_STATE_WAIT_PIR;
                ESP_LOGI(TAG, "waiting same direction PIR confirm within %d ms", PIR_CONFIRM_TIMEOUT_MS);
            }
            else if (evt.type == INTRUSION_EVENT_PIR)
            {
                ESP_LOGI(TAG, "PIR event direction:%s", direction_to_name(evt.direction));

                if (state == INTRUSION_STATE_WAIT_PIR && evt.direction == pending_direction)
                {
                    log_state_change(state, INTRUSION_STATE_VISION_CONFIRM);
                    state = INTRUSION_STATE_VISION_CONFIRM;

                    // 这里是以后接舵机、摄像头、AI、屏幕的入口。
                    // 目前硬件不在手，所以先用日志占位，确认流程跑通。
                    ESP_LOGI(TAG, "double trigger confirmed, camera/AI stage is reserved here");
                    ESP_LOGI(TAG, "temporary action: print log instead of moving servo, camera capture, and display");

                    log_state_change(state, INTRUSION_STATE_IDLE);
                    state = INTRUSION_STATE_IDLE;
                    pending_direction = -1;
                }
                else
                {
                    ESP_LOGI(TAG, "PIR event is stored as observation only, no matching radar trigger now");
                }
            }
        }
        else
        {
            ESP_LOGI(TAG, "PIR confirm timeout, treat radar-only trigger as possible false alarm");
            log_state_change(state, INTRUSION_STATE_IDLE);
            state = INTRUSION_STATE_IDLE;
            pending_direction = -1;
        }
    }
}

esp_err_t intrusion_init(void)
{
    s_intrusion_evt_queue = xQueueCreate(INTRUSION_QUEUE_LENGTH, sizeof(intrusion_event_t));
    if (s_intrusion_evt_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    const BaseType_t task_ret = xTaskCreate(intrusion_task,
                                            "intrusion_task",
                                            INTRUSION_TASK_STACK_SIZE,
                                            NULL,
                                            INTRUSION_TASK_PRIORITY,
                                            NULL);
    if (task_ret != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

void intrusion_on_radar_event(radar_direction_t direction)
{
    if (s_intrusion_evt_queue == NULL)
    {
        return;
    }

    // 回调函数里不要直接写复杂业务，先投递到状态机队列。
    // 这样 radar_task 不会被入侵检测逻辑拖住。
    const intrusion_event_t evt = {
        .type = INTRUSION_EVENT_RADAR,
        .direction = direction,
    };

    (void)xQueueSend(s_intrusion_evt_queue, &evt, 0);
}

void intrusion_on_pir_event(pir_direction_t direction)
{
    if (s_intrusion_evt_queue == NULL)
    {
        return;
    }

    // PIR 和雷达最终都汇入同一个队列，状态机按事件顺序统一处理。
    const intrusion_event_t evt = {
        .type = INTRUSION_EVENT_PIR,
        .direction = direction,
    };

    (void)xQueueSend(s_intrusion_evt_queue, &evt, 0);
}
