/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file iic.c
 *
 * @par dependencies
 * - iic.h
 * - stm32f4xx_gpio.h
 * - stm32f4xx_rcc.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements SPL GPIO-based software I2C bus operations.
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

/****************************** Includes ******************************/

#include "iic.h"               /* 用户 BSP 层：软件 IIC 接口 */

#include "stm32f4xx_gpio.h"    /* 芯片 SPL 层：GPIO、RCC 接口 */
#include "stm32f4xx_rcc.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define EEPROM_I2C_GPIO_PORT GPIOB               /* GPIO 端口 */
#define EEPROM_I2C_GPIO_CLK  RCC_AHB1Periph_GPIOB /* 端口时钟 */
#define EEPROM_I2C_SCL_PIN   GPIO_Pin_8          /* 时钟线 PB8 */
#define EEPROM_I2C_SDA_PIN   GPIO_Pin_9          /* 数据线 PB9 */

/* 释放 SCL，由外部上拉拉高。 */
#define EEPROM_I2C_SCL_1() \
    GPIO_SetBits(EEPROM_I2C_GPIO_PORT, EEPROM_I2C_SCL_PIN)

/* 拉低 SCL。 */
#define EEPROM_I2C_SCL_0() \
    GPIO_ResetBits(EEPROM_I2C_GPIO_PORT, EEPROM_I2C_SCL_PIN)

/* 释放 SDA，由上拉或从机驱动。 */
#define EEPROM_I2C_SDA_1() \
    GPIO_SetBits(EEPROM_I2C_GPIO_PORT, EEPROM_I2C_SDA_PIN)

/* 拉低 SDA。 */
#define EEPROM_I2C_SDA_0() \
    GPIO_ResetBits(EEPROM_I2C_GPIO_PORT, EEPROM_I2C_SDA_PIN)

/* 读取 SDA：低为 0，高为 1。 */
#define EEPROM_I2C_SDA_READ() \
    GPIO_ReadInputDataBit(EEPROM_I2C_GPIO_PORT, EEPROM_I2C_SDA_PIN)

/****************************** Defines ******************************/

/************************ Private Function Definitions ************************/

/**
 * @brief 执行位操作延时。
 *
 * 使用 40 次空循环。
 *
 * @return 无返回值。
 * @note 时长受主频和优化影响，无固定时间精度。
 */
static void IIC_Delay(void)
{
    uint8_t delay_index; /* 延时计数。 */

    /* 固定次数空循环。 */
    for (delay_index = 0; delay_index < 40; delay_index++)
        ;
}

/************************ Private Function Definitions ************************/

/************************ Public Function Definitions ************************/

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
void IIC_Init(void)
{
    GPIO_InitTypeDef gpio_init_config = {0}; /* 引脚配置。 */

    /* 1. 开启 GPIO 时钟并预置输出为高。 */
    RCC_AHB1PeriphClockCmd(EEPROM_I2C_GPIO_CLK, ENABLE);

    GPIO_SetBits(EEPROM_I2C_GPIO_PORT,
                 EEPROM_I2C_SCL_PIN | EEPROM_I2C_SDA_PIN);

    /* 2. 配置开漏输出。 */
    GPIO_StructInit(&gpio_init_config);
    gpio_init_config.GPIO_Pin =
        EEPROM_I2C_SCL_PIN | EEPROM_I2C_SDA_PIN;
    gpio_init_config.GPIO_Mode  = GPIO_Mode_OUT;
    gpio_init_config.GPIO_OType = GPIO_OType_OD;
    gpio_init_config.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    /* 引脚速度；总线频率由位延时决定。 */
    gpio_init_config.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(EEPROM_I2C_GPIO_PORT, &gpio_init_config);

    /* 3. 发送停止信号，释放总线。 */
    IIC_Stop();
}

/**
 * @brief 发送起始信号。
 *
 * SCL 为高时拉低 SDA，再拉低 SCL。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_Start(void)
{
    /* SCL 为高时拉低 SDA。 */
    EEPROM_I2C_SCL_1();
    EEPROM_I2C_SDA_1();
    IIC_Delay();
    EEPROM_I2C_SDA_0();
    IIC_Delay();
    EEPROM_I2C_SCL_0();
    IIC_Delay();
}

/**
 * @brief 发送停止信号。
 *
 * SCL 为高时释放 SDA。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_Stop(void)
{
    /* SCL 为高时释放 SDA。 */
    EEPROM_I2C_SDA_0();
    EEPROM_I2C_SCL_1();
    IIC_Delay();
    EEPROM_I2C_SDA_1();
    IIC_Delay();
}

/**
 * @brief 发送 ACK。
 *
 * 应答时拉低 SDA，结束后释放。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_SendAck(void)
{
    EEPROM_I2C_SDA_0();
    IIC_Delay();
    EEPROM_I2C_SCL_1();
    IIC_Delay();

    EEPROM_I2C_SCL_0();
    IIC_Delay();

    EEPROM_I2C_SDA_1(); /* 释放 SDA。 */
    IIC_Delay();
}

/**
 * @brief 发送 NACK。
 *
 * 应答时释放 SDA，由上拉保持高电平。
 *
 * @return 无返回值。
 * @note 须先调用 IIC_Init()。
 */
void IIC_SendNack(void)
{
    EEPROM_I2C_SDA_1();
    IIC_Delay();
    EEPROM_I2C_SCL_1();
    IIC_Delay();
    EEPROM_I2C_SCL_0();
    IIC_Delay();
}

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
iic_ack_status_t IIC_WaitAck(void)
{
    iic_ack_status_t ack_status = IIC_ACK_NOT_RECEIVED; /* 应答状态。 */

    /* 1. 释放 SDA。 */
    EEPROM_I2C_SDA_1();
    IIC_Delay();

    /* 2. 拉高 SCL。 */
    EEPROM_I2C_SCL_1();
    IIC_Delay();

    /* 3. 采样 SDA：低为 ACK，高为 NACK。 */
    if (EEPROM_I2C_SDA_READ())
    {
        ack_status = IIC_ACK_NOT_RECEIVED;
    }
    else
    {
        ack_status = IIC_ACK_RECEIVED;
    }
    EEPROM_I2C_SCL_0();
    IIC_Delay();

    return ack_status;
}

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
void IIC_SendByte(uint8_t data_byte)
{
    /* 高位先发。 */
    for (uint8_t bit_index = 0; bit_index < 8; bit_index++)
    {
        if (data_byte & 0x80)
        {
            EEPROM_I2C_SDA_1();
        }
        else
        {
            EEPROM_I2C_SDA_0();
        }
        IIC_Delay();

        EEPROM_I2C_SCL_1();
        IIC_Delay();
        EEPROM_I2C_SCL_0();
        IIC_Delay();

        if (7 == bit_index)
        {
            EEPROM_I2C_SDA_1();
        }
        data_byte <<= 1;
        IIC_Delay();
    }
}

/**
 * @brief 读取一个字节。
 *
 * 高位先读，逐位组合数据。
 *
 * @return uint8_t : 读取的数据。
 * @note 须先调用 IIC_Init() 并释放 SDA；ACK/NACK 由调用方发送。
 */
uint8_t IIC_ReadByte(void)
{
    uint8_t data_byte = 0; /* 接收字节。 */

    /* 高位先读。 */
    for (uint8_t bit_index = 0; bit_index < 8; bit_index++)
    {
        data_byte <<= 1;
        EEPROM_I2C_SCL_1();
        IIC_Delay();
        if (EEPROM_I2C_SDA_READ())
        {
            data_byte++;
        }
        EEPROM_I2C_SCL_0();
        IIC_Delay();
    }
    return data_byte;
}

/************************ Public Function Definitions ************************/
