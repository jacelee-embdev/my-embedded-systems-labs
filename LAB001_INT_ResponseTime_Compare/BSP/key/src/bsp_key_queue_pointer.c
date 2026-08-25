/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_key_queue_pointer.c
 *
 * @par dependencies
 * - bsp_key_config.h
 * - bsp_key_queue_pointer.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides interrupt-driven key detection and classified result routing
 *        using pointer transfer for the internal key-edge queue.
 *
 * Processing flow:
 *
 * key_init() creates the internal edge queue and the public Key1 result queue.
 * The GPIO EXTI callback timestamps press and release edges from both keys and
 * forwards event pointers to the key task. The task dereferences each pointer
 * and runs one state machine per key,
 * rejects invalid press durations, and classifies valid presses as short or
 * long. Key1 results are published through g_key_queue, while Key2 results are
 * reported through logging.
 *
 * @version V1.4 2026-08-12
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "bsp_key_config.h"

#ifndef BSP_KEY_QUEUE_USE_POINTER
#error "BSP_KEY_QUEUE_USE_POINTER is not defined."
#endif

/* Compile this implementation only when pointer transfer is selected. */
#if (BSP_KEY_QUEUE_USE_POINTER == 1U)

#include "bsp_key_queue_pointer.h"

/************************ Thread Definitions ************************/
osThreadId_t key_task_handle;
const osThreadAttr_t key_task_attributes = 
{
    .name = "key_task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};

/************************ Queue Handles ************************/
QueueHandle_t           g_key_queue      = NULL;
static QueueHandle_t    s_key_edge_queue = NULL;


/**
 * @brief Initializes the queues used by the key module.
 *
 * Creates g_key_queue for classified Key1 results and s_key_edge_queue for raw
 * EXTI edge events. The public result queue stores key_event_t values, while
 * the internal edge queue stores key_press_event_t pointers. Both queues hold
 * 10 elements. If either creation fails, it deletes any queue created during
 * the attempt and resets both handles to NULL.
 *
 * @return pdPASS if both queues are created successfully; otherwise pdFAIL.
 *
 * @note Call once before starting key_task_entry().
 */
BaseType_t key_init(void)
{
    // 1. Create the key-result queue.
    g_key_queue = xQueueCreate(10U, sizeof(key_event_t));

    // 2. Create the internal key-edge queue.
    s_key_edge_queue = xQueueCreate(10U, sizeof(key_press_event_t *));

    // 3. Clean up if either queue creation failed.
    if ((NULL == g_key_queue) || (NULL == s_key_edge_queue))
    {
        if (NULL != g_key_queue)
        {
            vQueueDelete(g_key_queue);
        }

        if (NULL != s_key_edge_queue)
        {
            vQueueDelete(s_key_edge_queue);
        }

        g_key_queue      = NULL;
        s_key_edge_queue = NULL;

        return pdFAIL;
    }

    return pdPASS;
}

/**
 * @brief Processes interrupt-driven key edges and delivers classified results.
 *
 * Receives pointers to timestamped EXTI events from the internal raw-edge queue
 * initialized by key_init() and dereferences each event pointer for processing.
 * It maintains independent per-key state machines, rejects presses shorter than
 * KEY_DEBOUNCE_TIME_MS or longer than KEY_PRESS_TIMEOUT_MS, and classifies the
 * remaining presses using KEY_LONG_PRESS_TIME_MS. Key1 results are sent to
 * g_key_queue without waiting, while Key2 results are reported through logging.
 *
 * @param[in] argument Reserved task argument. The function does not access it.
 *
 * @return None. The task runs indefinitely while waiting for and processing
 *         key-edge events.
 *
 * @note key_init() must complete successfully before this task is started.
 */
void key_task_entry(void *argument)
{
    static key_runtime_t key_runtime[KEY_ID_COUNT] =
    {
        {KEY_FSM_WAIT_PRESS, 0U, 0U, 0U},
        {KEY_FSM_WAIT_PRESS, 0U, 0U, 0U},
    };

    key_press_event_t * p_edge_event = NULL;
    
    key_event_t key_result   = KEY_EVENT_NONE;
    key_runtime_t *p_runtime = NULL;
    bool press_complete      = false;

    for (;;)
    {
        /* Step 1: Wait for an event pointer from either physical key. */
        if (pdPASS != xQueueReceive(s_key_edge_queue,
                                    &p_edge_event,
                                    portMAX_DELAY))
        {
            printf("[E][KEY] Failed to receive key-edge event.\r\n");
            continue;
        }
        
        /* Step 2: Reject an invalid ID before it is used as an array index. */
        if (p_edge_event->key_id >= KEY_ID_COUNT)
        {
            printf("[E][KEY] Invalid key ID: %d.\r\n",
                   (int)p_edge_event->key_id);
            continue;
        }

        p_runtime = &key_runtime[p_edge_event->key_id];
        key_result = KEY_EVENT_NONE;
        press_complete = false;

        /* Step 3: Dereference the event pointer and process the source key. */
        switch (p_runtime->fsm_state)
        {
            case KEY_FSM_WAIT_PRESS:
            {
                if (KEY_EDGE_FALLING == p_edge_event->edge_type)
                {
                    p_runtime->press_tick   = p_edge_event->edge_tick;
                    p_runtime->release_tick = 0U;
                    p_runtime->hold_ticks   = 0U;
                    p_runtime->fsm_state    = KEY_FSM_WAIT_RELEASE;
                }
                break;
            }

            case KEY_FSM_WAIT_RELEASE:
            {
                if (KEY_EDGE_RISING == p_edge_event->edge_type)
                {
                    p_runtime->release_tick = p_edge_event->edge_tick;
                    p_runtime->hold_ticks   = p_runtime->release_tick - p_runtime->press_tick;
                    p_runtime->fsm_state    = KEY_FSM_WAIT_PRESS;
                    press_complete          = true;

                    if (p_runtime->hold_ticks < KEY_DEBOUNCE_TIME_MS)
                    {
                        key_result = KEY_EVENT_NONE;
                    }
                    else if (p_runtime->hold_ticks < KEY_LONG_PRESS_TIME_MS)
                    {
                        key_result = KEY_EVENT_SHORT_PRESS;
                    }
                    else if (p_runtime->hold_ticks <= KEY_PRESS_TIMEOUT_MS)
                    {
                        key_result = KEY_EVENT_LONG_PRESS;
                    }
                    else
                    {
                        key_result = KEY_EVENT_NONE;
                    }
                }
                else if (KEY_EDGE_FALLING == p_edge_event->edge_type)
                {
                    /* Reset this key after an unexpected falling edge. */
                    p_runtime->fsm_state    = KEY_FSM_WAIT_PRESS;
                    p_runtime->press_tick   = 0U;
                    p_runtime->release_tick = 0U;
                    p_runtime->hold_ticks   = 0U;
                    key_result              = KEY_EVENT_NONE;

                    printf("[E][KEY] Unexpected falling edge for key ID %d.\r\n",
                           (int)p_edge_event->key_id);
                }
                break;
            }

            default:
            {
                p_runtime->fsm_state    = KEY_FSM_WAIT_PRESS;
                p_runtime->press_tick   = 0U;
                p_runtime->release_tick = 0U;
                p_runtime->hold_ticks   = 0U;
                printf("[E][KEY] Invalid state for key ID %d.\r\n",
                       (int)p_edge_event->key_id);
                break;
            }
        }

        /* Step 4: Handle the detected result according to its source key. */
        switch (p_edge_event->key_id)
        {
            case KEY_ID_1:
            {
                /* Key1 preserves the original result-queue behavior. */
                switch (key_result)
                {
                    case KEY_EVENT_SHORT_PRESS:
                    case KEY_EVENT_LONG_PRESS:
                    {
                        if (pdPASS != xQueueSend(g_key_queue,
                                                 &key_result,
                                                 0U))
                        {
                            printf("[E][KEY] Key1 result send failed.\r\n");
                        }
                        break;
                    }

                    case KEY_EVENT_NONE:
                    {
                        break;
                    }

                    default:
                    {
                        printf("[E][KEY] Invalid Key1 result: %d.\r\n",
                               (int)key_result);
                        break;
                    }
                }
                break;
            }

            case KEY_ID_2:
            {
                /* Key2 only prints the detected result. */
                switch (key_result)
                {
                    case KEY_EVENT_SHORT_PRESS:
                    {
                        printf("[I][KEY] Key2 short press.\r\n");
                        break;
                    }

                    case KEY_EVENT_LONG_PRESS:
                    {
                        printf("[I][KEY] Key2 long press.\r\n");
                        break;
                    }

                    case KEY_EVENT_NONE:
                    {
                        break;
                    }

                    default:
                    {
                        printf("[E][KEY] Invalid Key2 result: %d.\r\n",
                               (int)key_result);
                        break;
                    }
                }
                break;
            }

            default:
            {
                printf("[E][KEY] Invalid key ID: %d.\r\n",
                       (int)p_edge_event->key_id);
                break;
            }
        }

        /* Step 5: Clear timing data after a complete press operation. */
        if (press_complete)
        {
            p_runtime->press_tick   = 0U;
            p_runtime->release_tick = 0U;
            p_runtime->hold_ticks   = 0U;
        }
    }
}

/**
 * @brief Captures a key EXTI edge, queues its event-object pointer, and emits a
 *        GPIO trace marker.
 *
 * Maps Key1_Pin or Key2_Pin to its logical key, timestamps the interrupt, and
 * derives the press or release edge from the current pin level. When the
 * internal edge queue is ready, it sends the event-object pointer with
 * xQueueSendFromISR. The recognized GPIO is then reconfigured for the opposite
 * edge, and INT_TRACE/PB12 is toggled to mark the callback trace point.
 * Unrecognized pin masks are ignored.
 *
 * @param[in] GPIO_Pin GPIO pin mask supplied by HAL. Key1_Pin and Key2_Pin are
 *                     supported; all other masks are ignored.
 *
 * @return None.
 *
 * @note Runs in ISR context. The INT_TRACE/PB12 marker is generated after
 *       pointer forwarding and GPIO reconfiguration, so it does not represent
 *       the ISR entry time.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /** The array order must match key_id_t. */
    static const key_hardware_t key_hardware[KEY_ID_COUNT] =
    {
        {Key1_GPIO_Port, Key1_Pin},
        {Key2_GPIO_Port, Key2_Pin},
    };

    static key_press_event_t edge_event = 
    {
        KEY_ID_1,
        KEY_EDGE_FALLING,
        0U,
    };
    
    key_press_event_t * p_edge_event = &edge_event;

    GPIO_InitTypeDef gpio_config =          {0};
    key_id_t key_id              = KEY_ID_COUNT;

    /* Step 1: Find the logical key ID through the GPIO configuration table. */
    for (uint32_t index = 0U;
         index < (uint32_t)KEY_ID_COUNT;
         index++)
    {
        if (GPIO_Pin == key_hardware[index].gpio_pin)
        {
            key_id = (key_id_t)index;
            break;
        }
    }

    if (KEY_ID_COUNT == key_id)
    {
        return;
    }

    /* Step 2: Capture the source key ID and interrupt timestamp. */
    edge_event.key_id = key_id;
    edge_event.edge_tick = HAL_GetTick();

    /* Step 3: Prepare the common GPIO settings once. */
    gpio_config.Pin = key_hardware[key_id].gpio_pin;
    gpio_config.Pull = GPIO_PULLUP;

    /* Step 4: Read the real pin level and configure the opposite edge. */
    if (GPIO_PIN_RESET ==
        HAL_GPIO_ReadPin(key_hardware[key_id].gpio_port,
                         key_hardware[key_id].gpio_pin))
    {
        edge_event.edge_type = KEY_EDGE_FALLING;
        gpio_config.Mode = GPIO_MODE_IT_RISING;
    }
    else
    {
        edge_event.edge_type = KEY_EDGE_RISING;
        gpio_config.Mode = GPIO_MODE_IT_FALLING;
    }

    /* Step 5: Send the event-object pointer when the queue is ready. */
    if (NULL != s_key_edge_queue)
    {
        
        xQueueSendFromISR(s_key_edge_queue,
                                &p_edge_event,
                                NULL);
    }

    /* Step 6: Configure this key for its opposite edge. */
    HAL_GPIO_Init(key_hardware[key_id].gpio_port, &gpio_config);

    /* Step 7: Toggle PB12 to mark the callback trace point. */
    HAL_GPIO_TogglePin(INT_TRACE_GPIO_Port, INT_TRACE_Pin);
}

#endif /* BSP_KEY_QUEUE_USE_POINTER == 1U */
