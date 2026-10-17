/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file uart_handler.h
 *
 * @par dependencies
 * - stdint.h
 * - stm32f4xx_hal.h
 * - FreeRTOS.h
 * - queue.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares USART1 receive-event and queue APIs.
 *
 * Processing flow:
 *
 * Create or reuse the queue before scheduling and reception.
 * Read USART1 IDLE/TC lengths from the queue.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __UART_HANDLER_H__
#define __UART_HANDLER_H__

/****************************** Includes ******************************/

#include <stdint.h>          /* C 标准库 */

#include "stm32f4xx_hal.h"   /* HAL 层：STM32 接口 */

#include "FreeRTOS.h"        /* 中间件层：FreeRTOS 队列 */
#include "queue.h"

/****************************** Includes ******************************/

/****************************** Declaring ******************************/

/* 接收长度队列：uint16_t 元素，深度为 1。 */
extern QueueHandle_t g_uart_rx_length_queue;

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
int UARTHandler_InitReceiveQueue(void);

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
                                uint16_t received_size);

/****************************** Declaring ******************************/

#endif /* __UART_HANDLER_H__ */
