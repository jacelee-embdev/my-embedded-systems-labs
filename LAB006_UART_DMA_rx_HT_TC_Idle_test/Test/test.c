/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file test.c
 *
 * @par dependencies
 * - test.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Validates UART Receive-to-Idle events with circular DMA.
 *
 * Processing flow:
 *
 * The test task starts UART Receive-to-Idle reception with circular DMA. The
 * UART receive-event callback identifies and logs DMA half-transfer, DMA
 * transfer-complete, and UART idle events.
 *
 * @version V1.0 2026-09-12
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#include "test.h"

/****************************** Defines ******************************/
#define TAG                  "test"
#define UART_RX_BUFFER_SIZE  (10U)  /* Circular DMA Rx buffer size. */
/****************************** Defines ******************************/

/************************ Thread Resources ************************/
static TaskHandle_t test_task_handle = NULL;

/************************ UART Rx Resources ************************/
static uint8_t rx_buf[UART_RX_BUFFER_SIZE] = {0U};

/**
 * @brief Entry function for the UART DMA Receive-to-Idle test task.
 *
 * Starts USART1 circular DMA reception and then keeps the test task alive. If
 * DMA reception cannot be started, the function logs the failure and enters
 * the system error handler.
 *
 * @param[in] pvParameters Pointer to the task argument. This parameter is
 *                         unused.
 *
 * @return None. This task never returns during normal operation.
 */
static void test_task(void *pvParameters)
{
    /* Step 1: Mark the unused task parameter explicitly. */
    (void)pvParameters;

    /* Step 2: Start USART1 Receive-to-Idle reception with circular DMA. */
    if (HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                               rx_buf,
                                               UART_RX_BUFFER_SIZE))
    {
        elog_e(TAG, "Enabling UART DMA idle reception failed.\r\n");
        Error_Handler();
    }

    for (;;)
    {
        /* Optional heartbeat log retained for test-task debugging. */
        // elog_i(TAG, "Thread test_task runing test.\r\n");

        /* Step 3: Keep the task alive without occupying the processor. */
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

/**
 * @brief Creates the UART DMA Receive-to-Idle test task.
 *
 * Creates the application test task and reports whether task creation was
 * successful.
 *
 * @param None.
 *
 * @return None.
 */
void test_init(void)
{
    /* Step 1: Create the application test task. */
    if (pdPASS != xTaskCreate(test_task,
                              "test",
                              128U,
                              NULL,
                              1U,
                              &test_task_handle))
    {
        elog_e(TAG, "test task creation failed.\r\n");
        return;
    }

    /* Step 2: Report successful test-task initialization. */
    elog_i(TAG, "test initialed successfully.\r\n");
}

/**
 * @brief Handles UART Receive-to-Idle events reported by the HAL driver.
 *
 * Validates that the callback belongs to USART1, obtains the receive-event
 * type, and logs DMA half-transfer, DMA transfer-complete, or UART idle events.
 *
 * @param[in] huart Pointer to the UART handle that generated the event.
 * @param[in] Size  Current write position in the circular DMA buffer. This is
 *                  not necessarily the number of bytes added since the prior
 *                  callback.
 *
 * @return None.
 *
 * @note This callback runs in interrupt context. Logging is intentionally
 *       retained because the experiment measures HT, TC, and IDLE events.
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Step 1: Ignore invalid handles and events from UARTs other than
     * USART1. */
    if ((NULL == huart) || (USART1 != huart->Instance))
    {
        return;
    }

    /* Step 2: Identify and record the event that triggered this callback. */
    switch (HAL_UARTEx_GetRxEventType(huart))
    {
        case HAL_UART_RXEVENT_HT:
        {
            elog_i(TAG, "DMA HT, Size=%u\r\n", Size);
            break;
        }

        case HAL_UART_RXEVENT_TC:
        {
            elog_i(TAG, "DMA TC, Size=%u\r\n", Size);
            break;
        }

        case HAL_UART_RXEVENT_IDLE:
        {
            elog_i(TAG, "UART IDLE, Size=%u\r\n", Size);
            break;
        }

        default:
        {
            elog_i(TAG, "Interruption event error.\r\n");
            break;
        }
    }
}
