/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file iic.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares software I2C operations and device acknowledgement states.
 *
 * Processing flow:
 *
 * Initialize PB8/PB9 with IIC_Init() before using transfer APIs.
 * Use Start, byte transfer, ACK/NACK, and Stop APIs to form transactions.
 * IIC_WaitAck() samples the bus and returns iic_ack_status_t.
 * Device addressing and detection are handled by the caller.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef _IIC_H__
#define _IIC_H__

/****************************** Includes ******************************/

#include <stdint.h>           /* C 标准库：定宽整数 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* 设备地址写方向位。 */
#define EEPROM_I2C_WR        0

/* 设备地址读方向位。 */
#define EEPROM_I2C_RD        1

/* 应答状态：0 为 ACK，1 为 NACK。 */
typedef enum
{
    IIC_ACK_RECEIVED     = 0,  /* 已应答 */
    IIC_ACK_NOT_RECEIVED = 1   /* 未应答 */
} iic_ack_status_t;

/****************************** Defines ******************************/

/************************ Function Declarations ************************/

/**
 * @brief 初始化软件 IIC 总线。
 *
 * 处理步骤：
 * 1. 开启 GPIOB 时钟，预置 SCL、SDA 为高。
 * 2. 配置 PB8、PB9 为开漏输出，无上下拉。
 * 3. 发送停止信号，释放总线。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 需外部上拉；输出高表示释放总线。
 * @note 仅配置总线，不检测设备；使用通信接口前调用。
 */
void IIC_Init(void);

/**
 * @brief 发送起始信号。
 *
 * SCL 为高时拉低 SDA，再拉低 SCL。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_Start(void);

/**
 * @brief 发送停止信号。
 *
 * SCL 为高时释放 SDA。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_Stop(void);

/**
 * @brief 发送一个字节。
 *
 * 高位先发，发送后释放 SDA。
 *
 * @param[in] data_byte : 待发送字节。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()；应答由调用方处理。
 */
void IIC_SendByte(uint8_t data_byte);

/**
 * @brief 读取一个字节。
 *
 * 高位先读，逐位组合数据。
 *
 * @return uint8_t : 读取的数据。
 * @note 须先调用 IIC_Init() 并释放 SDA；ACK/NACK 由调用方发送。
 */
uint8_t IIC_ReadByte(void);

/**
 * @brief 采样设备应答。
 *
 * 处理步骤：
 * 1. 释放 SDA。
 * 2. 拉高 SCL，延时后采样 SDA。
 * 3. 拉低 SCL，返回结果。
 *
 * @return iic_ack_status_t : 应答状态。
 * @retval IIC_ACK_RECEIVED     已应答。
 * @retval IIC_ACK_NOT_RECEIVED 未应答。
 * @note 须先调用 IIC_Init()；仅采样一次，无轮询或超时。
 */
iic_ack_status_t IIC_WaitAck(void);

/**
 * @brief 发送 ACK。
 *
 * 应答时拉低 SDA，结束后释放。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_SendAck(void);

/**
 * @brief 发送 NACK。
 *
 * 应答时释放 SDA，由上拉保持高电平。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_SendNack(void);

/************************ Function Declarations ************************/

#endif /* _IIC_H__ */
