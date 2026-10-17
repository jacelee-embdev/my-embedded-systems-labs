/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file usart.h
 *
 * @par dependencies
 * - stm32f4xx.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the USART1 initialization and polling communication APIs.
 *
 * Processing flow:
 *
 * Call USART_InitUsart1() once before using USART_SendChar() or
 * USART_ReceiveChar().
 *
 * @version V1.0 2026-10-14
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __USART_H__
#define __USART_H__

/****************************** Includes ******************************/
#include "stm32f4xx.h"  /* 厂商 Core 层：设备定义 */
/****************************** Includes ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 初始化 USART1 时钟、引脚和通信参数。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
void USART_InitUsart1(void);

/**
 * @brief 等待发送寄存器空闲后，写入一个字节。
 *
 * @param[in] p_usart : USART 实例指针，不得为空。
 * @param[in] data    : 待发送字节。
 *
 * @return 无返回值。
 * @note 阻塞轮询，无超时。
 */
void USART_SendChar(USART_TypeDef *p_usart, uint8_t data);

/**
 * @brief 等待接收数据就绪后，读取一个字节。
 *
 * @param[in] p_usart : USART 实例指针，不得为空。
 *
 * @return 接收数据的低 8 位。
 * @note 阻塞轮询，无超时。
 */
uint8_t USART_ReceiveChar(USART_TypeDef *p_usart);
/************************ Function Declarations ************************/

#endif /* __USART_H__ */
