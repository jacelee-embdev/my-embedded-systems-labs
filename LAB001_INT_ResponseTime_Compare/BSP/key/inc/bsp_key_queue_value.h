/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_key_queue_value.h
 *
 * @par dependencies
 * - stdint.h
 * - stdio.h
 * - stdbool.h
 * - main.h
 * - cmsis_os.h
 * - FreeRTOS.h
 * - queue.h
 * - bsp_key_config.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides interrupt-driven key detection and classified result routing
 *        using queue-by-value transfer.
 *
 * Processing flow:
 *
 * key_init() creates the internal edge queue and the public Key1 result queue.
 * The GPIO EXTI callback timestamps press and release edges from both keys and
 * forwards complete event values to the key task. The task runs one state
 * machine per key,
 * rejects invalid press durations, and classifies valid presses as short or
 * long. Key1 results are published through g_key_queue, while Key2 results are
 * reported through logging.
 *
 * @version V1.3 2026-08-08
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_KEY_QUEUE_VALUE_H__
#define __BSP_KEY_QUEUE_VALUE_H__

#include "bsp_key_config.h"

/* Expose this interface only when queue-by-value transfer is selected. */
#if (BSP_KEY_QUEUE_USE_POINTER == 0U)

/****************************** Includes ******************************/
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "main.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "queue.h"
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define KEY_DEBOUNCE_TIME_MS      (10U)    /* Minimum valid press time. */
#define KEY_LONG_PRESS_TIME_MS    (1000U)  /* Long-press threshold. */
#define KEY_PRESS_TIMEOUT_MS      (5000U)  /* Maximum valid press time. */

typedef enum
{
    KEY_ID_1 = 0,              /* Key1 sends results to the queue. */
    KEY_ID_2,                  /* Key2 prints results to the log. */
    KEY_ID_COUNT,              /* Number of supported keys. */
} key_id_t;                    /* Logical key identifier. */

typedef struct
{
    GPIO_TypeDef *gpio_port;       /* GPIO port assigned to the key. */
    uint16_t      gpio_pin;        /* GPIO pin mask assigned to the key. */
} key_hardware_t;                  /* Physical-key GPIO mapping. */

typedef enum
{
    KEY_EDGE_FALLING = 0,      /* Falling edge starts a press. */
    KEY_EDGE_RISING,           /* Rising edge completes a press. */
} key_trigger_edge_t;          /* Captured key-edge type. */

typedef struct
{
    key_id_t           key_id;     /* Source key identifier. */
    key_trigger_edge_t edge_type;  /* Captured press or release edge. */
    uint32_t           edge_tick;  /* HAL tick captured at the edge. */
    
    /* Only used to amplify queue-copy timing during testing. */
//    uint32_t           test_payload[32];
} key_press_event_t;               /* Timestamped raw edge from the ISR. */

typedef enum
{
    KEY_FSM_WAIT_PRESS = 0,    /* Wait for a falling edge. */
    KEY_FSM_WAIT_RELEASE,      /* Wait for a rising edge. */
} key_fsm_state_t;             /* Per-key detection state. */

typedef struct
{
    key_fsm_state_t fsm_state;     /* Current per-key FSM state. */
    uint32_t        press_tick;    /* Tick captured on the falling edge. */
    uint32_t        release_tick;  /* Tick captured on the rising edge. */
    uint32_t        hold_ticks;    /* Duration used for press classification. */
} key_runtime_t;                   /* Per-key state and timing data. */

typedef enum
{
    KEY_EVENT_NONE = 0,        /* No valid completed press. */
    KEY_EVENT_SHORT_PRESS,     /* Short press below KEY_LONG_PRESS_TIME_MS. */
    KEY_EVENT_LONG_PRESS,      /* Long press through KEY_PRESS_TIMEOUT_MS. */
} key_event_t;                 /* Classified key-press result. */

/****************************** Defines ******************************/

/****************************** Declaring ******************************/
/************************ Thread Resources ************************/
extern osThreadId_t         key_task_handle;       /* Key-task handle. */
extern const osThreadAttr_t key_task_attributes;   /* Key-task attributes. */

/************************ Queue Resources ************************/
extern QueueHandle_t        g_key_queue;           /* Key1 result queue. */

/************************ API Declaration ************************/

/**
 * @brief Initializes the queues used by the key module.
 *
 * Creates g_key_queue for classified Key1 results and s_key_edge_queue for raw
 * EXTI edge events. The internal edge queue stores complete key_press_event_t
 * values. Both queues hold 10 elements. If either creation fails, it deletes
 * any queue created during the attempt and resets both handles to NULL.
 *
 * @return pdPASS if both queues are created successfully; otherwise pdFAIL.
 *
 * @note Call once before starting key_task_entry().
 */
BaseType_t  key_init                   (void);

/**
 * @brief Processes interrupt-driven key edges and delivers classified results.
 *
 * Waits for timestamped EXTI events from both keys on the internal raw-edge
 * queue initialized by key_init(). It maintains independent per-key state
 * machines, rejects presses shorter than KEY_DEBOUNCE_TIME_MS or longer than
 * KEY_PRESS_TIMEOUT_MS, and classifies the remaining presses using
 * KEY_LONG_PRESS_TIME_MS. Key1 results are sent to g_key_queue without waiting,
 * while Key2 results are reported through logging.
 *
 * @param[in] argument Reserved task argument. The function does not access it.
 *
 * @return None. The task runs indefinitely while waiting for and processing
 *         key-edge events.
 *
 * @note key_init() must complete successfully before this task is started.
 */
void        key_task_entry             (void *argument);

/**
 * @brief Captures a key EXTI edge, forwards it to the key task, and emits a
 *        GPIO trace marker.
 *
 * Maps Key1_Pin or Key2_Pin to its logical key, timestamps the interrupt, and
 * derives the press or release edge from the current pin level. When the
 * internal edge queue is ready, it copies the complete raw event value into the
 * queue with xQueueSendFromISR. The recognized GPIO is then reconfigured for
 * the opposite edge, and INT_TRACE/PB12 is toggled to mark the callback trace
 * point. Unrecognized pin masks are ignored.
 *
 * @param[in] GPIO_Pin GPIO pin mask supplied by HAL. Key1_Pin and Key2_Pin are
 *                     supported; all other masks are ignored.
 *
 * @return None.
 *
 * @note Runs in ISR context. The INT_TRACE/PB12 marker is generated after event
 *       forwarding and GPIO reconfiguration, so it does not represent the ISR
 *       entry time.
 */
void        HAL_GPIO_EXTI_Callback     (uint16_t GPIO_Pin);

/****************************** Declaring ******************************/

#endif /* BSP_KEY_QUEUE_USE_POINTER == 0U */

#endif /* End of __BSP_KEY_QUEUE_VALUE_H__ */
