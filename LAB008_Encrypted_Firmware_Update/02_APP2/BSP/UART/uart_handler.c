/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file uart_handler.c
 *
 * @par dependencies
 * - uart_handler.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Queues USART1 receive-event lengths.
 *
 * Processing flow:
 *
 * Create or reuse the queue before scheduling and reception.
 * Queue USART1 IDLE/TC lengths from interrupt context.
 * Keep pending notifications and count drops.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "uart_handler.h"  /* BSP 层：串口接收接口 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define UART_HANDLER_RX_LENGTH_QUEUE_DEPTH  1U  /* 通知队列深度 */

/****************************** Defines ******************************/

/************************ Variable Definitions ************************/

/* 接收长度队列。 */
QueueHandle_t g_uart_rx_length_queue = NULL;

/* 丢通知计数，仅供调试。 */
static volatile uint32_t g_uart_rx_dropped_event_count = 0U;

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 创建或复用接收长度队列。
 *
 * 处理步骤：
 * 1. 已创建则复用，保留待处理通知。
 * 2. 否则创建单元素 uint16_t 队列。
 * 3. 返回初始化结果。
 *
 * @return int : 初始化结果。
 * @retval 0 创建成功或复用已有队列。
 * @retval -1 创建失败。
 *
 * @note 调度器启动前串行调用，随后启动接收。
 * @note 仅传递长度，数据仍在 DMA 缓冲区。
 */
int UARTHandler_InitReceiveQueue(void)
{
    /* 1. 复用队列，保留待处理通知。 */
    if (NULL != g_uart_rx_length_queue)
    {
        return 0;
    }

    /* 2. 创建接收长度队列。 */
    g_uart_rx_length_queue = xQueueCreate(
        UART_HANDLER_RX_LENGTH_QUEUE_DEPTH, sizeof(uint16_t));

    /* 3. 返回结果，失败时句柄为空。 */
    return (NULL != g_uart_rx_length_queue) ? 0 : -1;
}

/**
 * @brief 转发 USART1 的 IDLE/TC 接收长度。
 *
 * 处理步骤：
 * 1. 检查句柄、串口实例和队列。
 * 2. 忽略 HT，仅处理 IDLE/TC。
 * 3. 投递长度，失败则保留原通知并计数。
 * 4. 按唤醒结果请求任务切换。
 *
 * @param[in] p_huart       : 串口句柄；为空时忽略。
 * @param[in] received_size : 接收字节数。
 *
 * @return 无。
 * @note 适用于普通 DMA，HAL 在回调前结束接收。
 * @note 仅传递长度，数据仍在 DMA 缓冲区。
 * @note 中断优先级须符合 FreeRTOS FromISR 限制。
 * @note 队列未创建时忽略；发送失败不打印日志。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *p_huart,
                                uint16_t received_size)
{
    uint16_t received_length = 0U;  /* 接收长度。 */
    BaseType_t higher_priority_task_woken = pdFALSE;  /* 任务唤醒标记。 */
    BaseType_t queue_result = pdFAIL;  /* 投递结果。 */
    /* HAL 接收事件类型。 */
    HAL_UART_RxEventTypeTypeDef event_type = HAL_UART_RXEVENT_TC;

    /* 1. 检查句柄、USART1 和队列。 */
    if (NULL == p_huart)
    {
        return;
    }
    if ((USART1 != p_huart->Instance) || (NULL == g_uart_rx_length_queue))
    {
        return;
    }

    /* 2. 仅处理 IDLE/TC。 */
    event_type = HAL_UARTEx_GetRxEventType(p_huart);
    if ((HAL_UART_RXEVENT_IDLE != event_type) &&
        (HAL_UART_RXEVENT_TC != event_type))
    {
        return;
    }

    /* 3. 投递长度，失败时保留原通知并计数。 */
    received_length = received_size;
    queue_result = xQueueSendFromISR(g_uart_rx_length_queue,
                                    &received_length,
                                    &higher_priority_task_woken);
    if (pdPASS != queue_result)
    {
        g_uart_rx_dropped_event_count++;
    }

    /* 4. 按唤醒结果请求任务切换。 */
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/************************ Function Definitions ************************/
