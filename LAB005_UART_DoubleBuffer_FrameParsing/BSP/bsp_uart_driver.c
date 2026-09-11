/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file bsp_uart_driver.c
 *
 * @par dependencies
 * - bsp_uart_driver.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements interrupt-driven UART reception with two alternating
 *        receive buffers.
 *
 * Processing flow:
 *
 * uart_rx_task() creates two circular-buffer objects and starts reception into
 * the first object's data array. HAL_UART_RxCpltCallback() advances the
 * completed buffer and selects the alternate buffer. The task forwards the
 * completed-buffer identifier to the parser and restarts UART reception.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "bsp_uart_driver.h"
/****************************** Includes ******************************/

/****************************** Defines *******************************/
#define TAG                          "bsp_uart"
#define NOTICE_DATA_ISR_TO_THREAD    (0xF4)  /* RX-complete notice value. */
#define BUFFER_WRITE_ONCE_SIZE       (9U)    /* Bytes received per transfer. */

/* Ensure that one transfer fits completely inside the receive buffer. */
#if (BUFFER_WRITE_ONCE_SIZE >= BUFFER_SIZE)
#error "BUFFER_WRITE_ONCE_SIZE must be smaller than BUFFER_SIZE"
#endif

/* Keep every fixed-size transfer aligned at the circular-buffer boundary. */
#if ((BUFFER_SIZE % BUFFER_WRITE_ONCE_SIZE) != 0U)
#error "BUFFER_SIZE must be an integer multiple of BUFFER_WRITE_ONCE_SIZE"
#endif

/****************************** Defines *******************************/

/************************ Global Resources ************************/
/************************ Global Resources ************************/

/*************************** Static Resources ************************/
static circular_buf_t *g_p_circular_buf1 = NULL;
static circular_buf_t *g_p_circular_buf2 = NULL;
static volatile buf_curr_slt_t buf_curr_slt = BUF_NO;
/*************************** Static Resources ************************/

/**
 * @brief Entry function for interrupt-driven UART reception.
 *
 * Creates two circular-buffer objects and alternates fixed-size interrupt-
 * driven reception between them. For each completed transfer, it forwards
 * the completed-buffer identifier to the packet-parser queue and restarts
 * reception with the alternate buffer.
 *
 * @param[in] pvParameters Pointer to the task argument. This parameter is not
 *                         used.
 *
 * @return None. This task runs indefinitely.
 */
void uart_rx_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t rx_to_parser_data = 0;

    /* 1. Create two circular-buffer objects for the receive-buffer test. */
    g_p_circular_buf1 = create_empty_circular_buf();
    g_p_circular_buf2 = create_empty_circular_buf();
    if ((NULL == g_p_circular_buf1) || (NULL == g_p_circular_buf2))
    {
        elog_e(TAG, "Circular buffer creation failed.\r\n");
        Error_Handler();
    }

    /* 2. Select buffer 1 and start the first interrupt-driven reception. */
    uint32_t notice_data_rx = 0;
    buf_curr_slt = BUF_1;

    if (HAL_OK != HAL_UART_Receive_IT(&huart1,
                                      g_p_circular_buf1->data,
                                      BUFFER_WRITE_ONCE_SIZE))
    {
        buf_curr_slt = BUF_NO;
    }

    for (;;)
    {
        /* 3. Wait for the UART receive-completion notification. */
        if (pdPASS != xQueueReceive(rx_ok_queue,
                                    &notice_data_rx,
                                    portMAX_DELAY))
        {
            elog_e(TAG, "Failed to receive queue.");
        }

        /* 4. Forward the completed buffer and start the next reception. */
        switch (buf_curr_slt)
        {
            case BUF_1:
                rx_to_parser_data = BUF_2;
                if (pdPASS != xQueueSend(rx_to_parser_queue,
                                         &rx_to_parser_data,
                                         0))
                {
                    elog_e(TAG, "Queue send failed.\r\n");
                }

                if (HAL_OK != HAL_UART_Receive_IT(
                                  &huart1,
                                  &g_p_circular_buf1->data[
                                      g_p_circular_buf1->head],
                                  BUFFER_WRITE_ONCE_SIZE))
                {
                    buf_curr_slt = BUF_NO;
                }
                break;

            case BUF_2:
                rx_to_parser_data = BUF_1;
                if (pdPASS != xQueueSend(rx_to_parser_queue,
                                         &rx_to_parser_data,
                                         0))
                {
                    elog_e(TAG, "Queue send failed.\r\n");
                }

                if (HAL_OK != HAL_UART_Receive_IT(
                                  &huart1,
                                  &g_p_circular_buf2->data[
                                      g_p_circular_buf2->head],
                                  BUFFER_WRITE_ONCE_SIZE))
                {
                    buf_curr_slt = BUF_NO;
                }
                break;

            default:
                elog_e(TAG, "Invalidbuffer state:%u\r\n", buf_curr_slt);
                break;
        }

        /* 5. Report a failure to restart interrupt-driven reception. */
        if (BUF_NO == buf_curr_slt)
        {
            elog_e(TAG, "Failed to enable UART interrupt reception.\r\n");
        }

        /* Clear the parser message before processing the next transfer. */
        rx_to_parser_data = 0;

#if 1
        elog_i(TAG, "notice_data_rx:%x\r\n", notice_data_rx);
#endif

        // elog_i(TAG, "Thread uart_rx test.\r\n");  /* Retained test code. */
        // vTaskDelay(pdMS_TO_TICKS(1000U));         /* Retained test code. */
    }
}

/**
 * @brief Returns one UART receive circular-buffer object.
 *
 * Selects the requested buffer created by uart_rx_task().
 *
 * @param[in] buf_num Receive-buffer number. Valid values are 1 and 2.
 *
 * @return circular_buf_t * Pointer to the requested receive buffer.
 * @retval NULL The buffer is unavailable or buf_num is invalid.
 */
circular_buf_t *get_circualr_buffer(uint8_t buf_num)
{
    /* Select and validate the requested receive-buffer object. */
    switch (buf_num)
    {
        case 1:
            if (NULL == g_p_circular_buf1)
            {
                elog_e(TAG, "Null pointer error.\r\n");
                return NULL;
            }
            else
            {
                return g_p_circular_buf1;
            }
//            break; //Keep the commented-out code.

        case 2:
            if (NULL == g_p_circular_buf2)
            {
                elog_e(TAG, "Null pointer error.\r\n");
                return NULL;
            }
            else
            {
                return g_p_circular_buf2;
            }
//            break; //Keep the commented-out code.

        default:
            elog_e(TAG, "Invalid buffer number.\r\n");
            break;
    }

    return NULL;
}

/**
 * @brief Handles completion of an interrupt-driven UART reception.
 *
 * For USART1, selects the alternate receive buffer and sends a completion
 * notification to the UART receive task from interrupt context. A wake-state
 * variable is supplied to the ISR-safe queue operation.
 *
 * @param[in] huart Pointer to the UART handle that completed reception.
 *
 * @return None.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 1. Process receive-completion events from USART1 only. */
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        uint32_t notice_data_tx = NOTICE_DATA_ISR_TO_THREAD;

        /* 2. Select the alternate buffer for the next reception. */
        switch (buf_curr_slt)
        {
            case BUF_1:
                g_p_circular_buf1->head =
                    (g_p_circular_buf1->head + BUFFER_WRITE_ONCE_SIZE) %
                    BUFFER_SIZE;
                buf_curr_slt = BUF_2;
                break;

            case BUF_2:
                g_p_circular_buf2->head =
                    (g_p_circular_buf2->head + BUFFER_WRITE_ONCE_SIZE) %
                    BUFFER_SIZE;
                buf_curr_slt = BUF_1;
                break;

            default:
                elog_e(TAG, "Invalidbuffer state:%u\r\n", buf_curr_slt);
                break;
        }

        /* 3. Notify the receive task that one UART transfer completed. */
        if (pdPASS != xQueueSendFromISR(rx_ok_queue,
                                        &notice_data_tx,
                                        &xHigherPriorityTaskWoken))
        {
            elog_e(TAG, "Failed to send queue.");
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
