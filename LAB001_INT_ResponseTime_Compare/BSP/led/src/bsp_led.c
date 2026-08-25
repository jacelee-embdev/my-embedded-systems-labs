/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_led.c
 *
 * @par dependencies
 * - bsp_led.h
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

#include "bsp_led.h"

/************************ Thread Definitions ************************/
osThreadId_t led_task_handle;
const osThreadAttr_t led_task_attributes = {
    .name = "led_Task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};

/************************ Queue Handles ************************/
QueueHandle_t g_led_queue = NULL;

/**
 * @brief Initializes the LED command queue.
 *
 * Creates the queue used to transfer LED commands to the LED task.
 *
 * @return BaseType_t Result of the queue initialization.
 * @retval pdPASS The LED command queue was created successfully.
 * @retval pdFAIL The LED command queue could not be created.
 */
BaseType_t led_init(void)
{
    // Create and check the LED-command queue.
    g_led_queue = xQueueCreate(10U, sizeof(led_command_t));

    if (NULL == g_led_queue)
    {
        return pdFAIL;
    }

    return pdPASS;
}

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
led_operate_result_t led_operate(led_state_t led_state)
{
    led_operate_result_t led_ops_ret = LED_OPERATE_ERROR;

    // 1. Turn on the LED.
    if (LED_STATE_ON == led_state)
    {
        HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_RESET);
        led_ops_ret = LED_OPERATE_OK;

        return led_ops_ret;
    }

    // 2. Turn off the LED.
    if (LED_STATE_OFF == led_state)
    {
        HAL_GPIO_WritePin(Led_GPIO_Port, Led_Pin, GPIO_PIN_SET);
        led_ops_ret = LED_OPERATE_OK;

        return led_ops_ret;
    }

    // 3. Toggle the LED.
    if (LED_STATE_TOGGLE == led_state)
    {
        HAL_GPIO_TogglePin(Led_GPIO_Port, Led_Pin);
        led_ops_ret = LED_OPERATE_OK;
        printf("TEST_LOG:LED toggle successfully.\r\n");
        return led_ops_ret;
    }

    return led_ops_ret;
}

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
led_process_result_t led_process_command(led_command_t led_command)
{
    led_operate_result_t led_ops_ret = LED_OPERATE_ERROR;

    // 1. Handle the no-operation command.
    if (LED_CMD_NONE == led_command)
    {
        return LED_PROCESS_OK;
    }

    // 2. Handle the LED toggle command.
    else if (LED_CMD_TOGGLE == led_command)
    {
        led_ops_ret = led_operate(LED_STATE_TOGGLE);

        // 2.1 Check whether the LED operation succeeded.
        if (LED_OPERATE_OK == led_ops_ret)
        {
            return LED_PROCESS_OK;
        }
        else
        {
            return LED_PROCESS_ERROR;
        }
    }

    // 3. Handle the three-blink command.
    else if (LED_CMD_BLINK_3_TIMES == led_command)
    {
        // 3.1 Toggle six times to produce three complete blink cycles.
        for (int i = 0; i < 6; i++)
        {
            led_ops_ret = led_operate(LED_STATE_TOGGLE);

            // 3.1.1 Stop processing if an LED operation fails.
            if (LED_OPERATE_OK != led_ops_ret)
            {
                return LED_PROCESS_ERROR;
            }

            // 3.1.2 Wait before performing the next toggle.
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        return LED_PROCESS_OK;
    }

    // 4. Reject unsupported LED commands.
    else
    {
        return LED_PROCESS_INVALID_COMMAND;
    }
}


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
void led_task_entry(void *argument)
{
    led_command_t led_command               = LED_CMD_NONE;
    led_process_result_t led_process_result = LED_PROCESS_ERROR;

    for (;;)
    {
        // printf("LED thread test.\r\n");

        if (NULL != g_led_queue)
        {
            if (pdPASS == xQueueReceive(g_led_queue,
                                        &led_command,
                                        portMAX_DELAY))
            {
                led_process_result = led_process_command(led_command);

                // Log only if command execution fails.
                if (LED_PROCESS_OK != led_process_result)
                {
                    printf(
                        "[E][LED] Command failed, command=%d, result=%d\r\n",
                        (int)led_command,
                        (int)led_process_result);
                }
            }
        }
    }
}
