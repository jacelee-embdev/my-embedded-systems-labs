/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file spi.h
 *
 * @par dependencies
 * - stm32f4xx.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares SPI1 initialization and polling transfer APIs.
 *
 * Processing flow:
 *
 * Call SPI_InitSpi1() before using the transfer APIs.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __SPI_H__
#define __SPI_H__

/****************************** Includes ******************************/
#include "stm32f4xx.h"  /* 厂商 Core 层：设备及整数类型定义 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define SPI_CS_PIN        GPIO_Pin_4  /* 片选引脚 */
#define SPI_CS_GPIO_PORT  GPIOA       /* 片选端口 */
/****************************** Defines ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 初始化 SPI1 时钟、引脚和通信参数。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 末尾发送 0xFF 启动字节。
 */
void SPI_InitSpi1(void);

/**
 * @brief 设置 SPI1 波特率预分频值。
 *
 * @param[in] baud_rate_prescaler : SPL 定义的预分频值。
 *
 * @return 无返回值。
 */
void SPI_SetSpeed(uint8_t baud_rate_prescaler);

/**
 * @brief 通过 SPI1 同步发送并接收一个字节。
 *
 * @param[in] write_data : 待发送字节。
 *
 * @return 接收到的字节。
 * @note 阻塞轮询，无超时。
 */
uint8_t SPI_ReadWriteByte(uint8_t write_data);

/**
 * @brief 轮询发送多个字节，并丢弃同步接收的数据。
 *
 * @param[in] p_write_data : 发送缓冲区，不得为空。
 * @param[in] data_size    : 发送字节数。
 * @param[in] timeout      : 超时时间，单位为毫秒。
 *
 * @return 传输结果。
 * @retval 0 传输完成。
 * @retval -1 传输超时。
 * @note 依赖运行中的毫秒计数。
 */
int SPI_WriteBytes(uint8_t *p_write_data, uint16_t data_size,
                   uint32_t timeout);

/**
 * @brief 轮询读取多个字节，同一缓冲区用于发送和接收。
 *
 * @param[in,out] p_read_data : 收发缓冲区，不得为空。
 * @param[in] data_size       : 接收字节数。
 * @param[in] timeout         : 超时时间，单位为毫秒。
 *
 * @return 传输结果。
 * @retval 0 传输完成。
 * @retval -1 传输超时。
 * @note 依赖运行中的毫秒计数。
 */
int SPI_ReadBytes(uint8_t *p_read_data, uint16_t data_size,
                  uint32_t timeout);
/************************ Function Declarations ************************/

#endif /* __SPI_H */
