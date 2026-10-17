/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file spi.c
 *
 * @par dependencies
 * - spi.h
 * - stm32f4xx_it.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements SPI1 initialization and polling byte transfers.
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

/****************************** Includes ******************************/
#include "spi.h"  /* 用户 Core 层：SPI 模块接口 */

#include "stm32f4xx_it.h"  /* 用户 Core 层：毫秒计数 */
/****************************** Includes ******************************/

/************************ Function Definitions ************************/
/**
 * @brief 开启 GPIOA 和 SPI1 时钟。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void SPI_ConfigClock(void)
{
    /* 1. 开启引脚时钟。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* 2. 开启 SPI1 时钟。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
}

/**
 * @brief 将 PA4 片选引脚配置为推挽输出、上拉。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void SPI_InitCsGpio(void)
{
    GPIO_InitTypeDef gpio_init_structure;  /* 片选引脚配置 */

    /* 配置 PA4 的输出模式。 */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_OUT;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Pin = GPIO_Pin_4;
    gpio_init_structure.GPIO_PuPd = GPIO_PuPd_UP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &gpio_init_structure);
}

/**
 * @brief 初始化 SPI1 时钟、引脚和通信参数。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 末尾发送 0xFF 启动字节。
 */
void SPI_InitSpi1(void)
{
    /* 1. 在配置引脚前开启时钟。 */
    SPI_ConfigClock();

    /* 2. 配置片选引脚，默认输出高电平。 */
    SPI_InitCsGpio();
    GPIO_WriteBit(SPI_CS_GPIO_PORT, SPI_CS_PIN, Bit_SET);

    GPIO_InitTypeDef gpio_init_structure;  /* SPI1 复用引脚配置 */

    /* 3. 配置 PA5、PA6、PA7 并映射到 SPI1。 */
    gpio_init_structure.GPIO_Mode = GPIO_Mode_AF;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio_init_structure.GPIO_PuPd = GPIO_PuPd_UP;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &gpio_init_structure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    /* 4. 复位 SPI1。 */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_SPI1, DISABLE);

    SPI_InitTypeDef spi_init_structure;  /* SPI1 通信参数 */

    /* 5. 配置为全双工主机、模式 0、8 位、分频 256。 */
    spi_init_structure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init_structure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    spi_init_structure.SPI_CPHA = SPI_CPHA_1Edge;
    spi_init_structure.SPI_CPOL = SPI_CPOL_Low;
    spi_init_structure.SPI_CRCPolynomial = 7;
    spi_init_structure.SPI_DataSize = SPI_DataSize_8b;
    spi_init_structure.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init_structure.SPI_Mode = SPI_Mode_Master;
    spi_init_structure.SPI_NSS = SPI_NSS_Soft;
    SPI_Init(SPI1, &spi_init_structure);

    /* 6. 使能 SPI1 并发送启动字节。 */
    SPI_Cmd(SPI1, ENABLE);

    SPI_ReadWriteByte(0xff);
}

/**
 * @brief 设置 SPI1 波特率预分频值。
 *
 * @param[in] baud_rate_prescaler : SPL 定义的预分频值。
 *
 * @return 无返回值。
 */
void SPI_SetSpeed(uint8_t baud_rate_prescaler)
{
    /* 1. 检查预分频参数。 */
    assert_param(IS_SPI_BAUDRATE_PRESCALER(baud_rate_prescaler));

    /* 2. 更新预分频字段。 */
    SPI1->CR1 &= 0xFFC7;
    SPI1->CR1 |= baud_rate_prescaler;

    /* 3. 使能 SPI1。 */
    SPI_Cmd(SPI1, ENABLE);
}

/**
 * @brief 通过 SPI1 同步发送并接收一个字节。
 *
 * @param[in] write_data : 待发送字节。
 *
 * @return 接收到的字节。
 * @note 阻塞轮询，无超时。
 */
uint8_t SPI_ReadWriteByte(uint8_t write_data)
{
    /* 1. 等待发送缓冲区可写后发送。 */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, write_data);

    /* 2. 等待接收数据就绪后读取。 */
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

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
                   uint32_t timeout)
{
    uint32_t timeout_ms = timeout;           /* 超时阈值，单位为毫秒 */
    uint32_t start_tick = SysTickUpdateTime; /* 起始毫秒计数 */
    uint16_t tx_size = data_size;            /* 剩余发送字节数 */
    uint16_t rx_size = data_size;            /* 剩余接收字节数 */

    uint8_t *p_tx_buffer = p_write_data; /* 发送缓冲区当前位置 */
    uint8_t tx_flow = 1u;               /* 1：可发送；0：等待接收 */

    /* 1. 轮询完成全部收发字节。 */
    while ((tx_size > 0) || (rx_size > 0))
    {
        /* 1.1 发送下一个字节。 */
        if ((SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET) &&
            (tx_size > 0) && (tx_flow == 1))
        {
            SPI_I2S_SendData(SPI1, *p_tx_buffer);
            p_tx_buffer += sizeof(uint8_t);
            tx_size--;
            tx_flow = 0;
        }

        /* 1.2 读取并丢弃同步接收的数据。 */
        if ((SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET) &&
            (rx_size > 0) && (tx_flow == 0))
        {
            SPI_I2S_ReceiveData(SPI1);
            rx_size--;
            tx_flow = 1;
        }

        /* 1.3 超时后清理标志并返回。 */
        if ((SysTickUpdateTime - start_tick) >= timeout_ms)
        {
            SPI_I2S_ClearFlag(SPI1, SPI_I2S_FLAG_OVR);
            return -1;
        }
    }

    /* 2. 完成传输后清理状态标志。 */
    SPI_I2S_ClearFlag(SPI1, SPI_I2S_FLAG_OVR);
    return 0;
}

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
                  uint32_t timeout)
{
    uint32_t timeout_ms = timeout;           /* 超时阈值，单位为毫秒 */
    uint32_t start_tick = SysTickUpdateTime; /* 起始毫秒计数 */
    uint16_t tx_size = data_size;            /* 剩余发送字节数 */
    uint16_t rx_size = data_size;            /* 剩余接收字节数 */

    uint8_t *p_tx_buffer = p_read_data; /* 发送缓冲区当前位置 */
    uint8_t *p_rx_buffer = p_read_data; /* 接收缓冲区当前位置 */
    uint8_t tx_flow = 1u;              /* 1：可发送；0：等待接收 */

    /* 1. 轮询完成全部收发字节。 */
    while ((tx_size > 0) || (rx_size > 0))
    {
        /* 1.1 发送下一个字节。 */
        if ((SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != RESET) &&
            (tx_size > 0) && (tx_flow == 1))
        {
            SPI_I2S_SendData(SPI1, *p_tx_buffer);
            p_tx_buffer += sizeof(uint8_t);
            tx_size--;
            tx_flow = 0;
        }

        /* 1.2 将接收数据写回缓冲区。 */
        if ((SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != RESET) &&
            (rx_size > 0) && (tx_flow == 0))
        {
            *p_rx_buffer = SPI_I2S_ReceiveData(SPI1);
            p_rx_buffer += sizeof(uint8_t);
            rx_size--;
            tx_flow = 1;
        }

        /* 1.3 超时后返回。 */
        if ((SysTickUpdateTime - start_tick) >= timeout_ms)
        {
            return -1;
        }
    }

    /* 2. 返回传输完成状态。 */
    return 0;
}
/************************ Function Definitions ************************/
