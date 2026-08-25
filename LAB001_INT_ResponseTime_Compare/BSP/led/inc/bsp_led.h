/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_led.h
 *
 * @par dependencies
 * - stdio.h
 * - stdint.h
 * - main.h
 * - cmsis_os.h
 * - FreeRTOS.h
 * - task.h
 * - queue.h
 * - gpio.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides LED queue initialization, control, and RTOS command handling.
 *
 * Processing flow:
 *
 * Call led_operate() and led_process_command() from task context,
 * or run led_task_entry() as an RTOS thread.
 *
 * @version V1.2 2026-08-08
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_LED_H__
#define __BSP_LED_H__

/****************************** Includes ******************************/

#include <stdint.h>     // The Compiler Library
#include <stdio.h>

#include "main.h"       // Core / OS layer
#include "cmsis_os.h"

#include "FreeRTOS.h"   // Specific file for operation
#include "task.h"
#include "queue.h"
#include "gpio.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/
/************************ Thread Definitions ************************/
extern osThreadId_t led_task_handle;
extern const osThreadAttr_t led_task_attributes;

/************************ Queue Handles ************************/
extern QueueHandle_t g_led_queue;


typedef enum
{
    LED_OPERATE_OK = 0,                    /* Success. */
    LED_OPERATE_ERROR,                     /* General error. */
    LED_OPERATE_TIMEOUT,                   /* Polling timed out. */
    LED_OPERATE_RESOURCE_UNAVAILABLE,      /* Resource unavailable. */
    LED_OPERATE_INVALID_PARAMETER,         /* Invalid parameter. */
    LED_OPERATE_OUT_OF_MEMORY,             /* Out of memory. */
    LED_OPERATE_ISR_NOT_ALLOWED,           /* Invalid in ISR context. */
    LED_OPERATE_RESERVED = 0x7FFFFFFF      /* Reserved. */
} led_operate_result_t;                    /* LED operation result. */

typedef enum
{
    LED_STATE_OFF = 0,                     /* LED is off. */
    LED_STATE_ON,                          /* LED is on. */
    LED_STATE_TOGGLE,                      /* Toggle LED state. */
} led_state_t;                             /* Requested LED state. */

typedef enum
{
    LED_PROCESS_OK = 0,                    /* Command completed. */
    LED_PROCESS_ERROR,                     /* LED operation failed. */
    LED_PROCESS_INVALID_COMMAND,           /* Unsupported command. */
} led_process_result_t;                    /* LED command result. */

typedef enum
{
    LED_CMD_NONE = 0,                      /* No command. */
    LED_CMD_TOGGLE,                        /* Toggle LED state. */
    LED_CMD_BLINK_3_TIMES,                 /* Blink three times. */
} led_command_t;                           /* LED command. */
/****************************** Defines ******************************/


/************************ Function Declarations ************************/

/**
 * @brief Initializes the LED command queue.
 *
 * Creates the queue used to transfer LED commands to the LED task.
 *
 * @return BaseType_t Result of the queue initialization.
 * @retval pdPASS The LED command queue was created successfully.
 * @retval pdFAIL The LED command queue could not be created.
 */
BaseType_t              led_init                (void);

/**
 * @brief Controls the LED according to the requested state.
 *
 * Drives the active-low LED GPIO to turn the LED on, turn it off,
 * or toggle its current state.
 *
 * @param[in] led_state Requested LED state.
 *
 * @return led_operate_result_t Result of the LED operation.
 * @retval LED_OPERATE_OK    The requested LED operation was completed.
 * @retval LED_OPERATE_ERROR The requested LED state is invalid.
 */
led_operate_result_t    led_operate             (led_state_t led_state);

/**
 * @brief Processes an LED control command.
 *
 * LED_CMD_NONE performs no operation. LED_CMD_TOGGLE toggles the LED once.
 * LED_CMD_BLINK_3_TIMES toggles the LED six times at 250 ms intervals,
 * producing three 2 Hz blink cycles and restoring the original LED state.
 *
 * @param[in] led_command LED command to process.
 *
 * @return led_process_result_t Result of command processing.
 * @retval LED_PROCESS_OK              The command was processed successfully.
 * @retval LED_PROCESS_ERROR           A lower-level LED operation failed.
 * @retval LED_PROCESS_INVALID_COMMAND The command is not supported.
 *
 * @note This function must be called from task context because the blink
 *       command uses vTaskDelay().
 */
led_process_result_t    led_process_command     (led_command_t led_command);


/**
 * @brief Entry function for the LED-control task.
 *
 * Waits indefinitely for commands on the queue initialized by led_init().
 * Whenever a command is received, led_process_command() is called to process
 * it and update the LED. Command-processing failures are reported through
 * logging.
 *
 * @param[in] argument Pointer to the task argument. This parameter is not used.
 *
 * @return None. The task runs indefinitely while waiting for and processing
 *         LED commands.
 *
 * @note led_init() must complete successfully before this task is started.
 */
void                    led_task_entry          (void *argument);
/************************ Function Declarations ************************/

#endif /* End of __BSP_LED_H__ */
