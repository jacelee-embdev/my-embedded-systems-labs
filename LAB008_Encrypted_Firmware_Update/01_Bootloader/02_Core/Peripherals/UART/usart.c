/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file usart.c
 *
 * @par dependencies
 * - usart.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements USART1 initialization and polling communication.
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

/****************************** Includes ******************************/
#include "usart.h"  /* 用户 Core 层：USART 模块接口 */
/****************************** Includes ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 开启 GPIOA 和 USART1 时钟。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void USART_ConfigClock(void)
{
    /* 1. 开启引脚时钟。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* 2. 开启 USART1 时钟。 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}

/**
 * @brief 将 PA9、PA10 配置为 USART1 收发引脚。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void USART_ConfigGpio(void)
{
    GPIO_InitTypeDef gpio_init_structure = {0};  /* 引脚配置 */

    /* 1. 配置 PA9 为复用推挽、上拉发送引脚。 */
    gpio_init_structure.GPIO_Pin   = GPIO_Pin_9;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_AF;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &gpio_init_structure);

    /* 2. 将相同配置应用到 PA10 接收引脚。 */
    gpio_init_structure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &gpio_init_structure);

    /* 3. 将 PA9、PA10 映射到 USART1。 */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
}

/**
 * @brief 配置 USART1 为 115200、8N1、无流控并使能收发。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void USART_Configuration(void)
{
    USART_InitTypeDef usart_init_structure = {0};  /* 串口配置 */

    /* 1. 设置通信参数。 */
    usart_init_structure.USART_BaudRate = 115200U;
    usart_init_structure.USART_WordLength = USART_WordLength_8b;
    usart_init_structure.USART_StopBits = USART_StopBits_1;
    usart_init_structure.USART_Parity = USART_Parity_No;
    usart_init_structure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;
    usart_init_structure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    /* 2. 应用配置并使能 USART1。 */
    USART_Init(USART1, &usart_init_structure);
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief 等待发送寄存器空闲后，写入一个字节。
 *
 * @param[in] p_usart : USART 实例指针，不得为空。
 * @param[in] data    : 待发送字节。
 *
 * @return 无返回值。
 * @note 阻塞轮询，无超时。
 */
void USART_SendChar(USART_TypeDef *p_usart, uint8_t data)
{
    /* 1. 等待发送寄存器空闲。 */
    while (RESET == USART_GetFlagStatus(p_usart, USART_FLAG_TXE))
    {
    }

    /* 2. 写入待发送字节。 */
    USART_SendData(p_usart, data);
}

/**
 * @brief 等待接收数据就绪后，读取一个字节。
 *
 * @param[in] p_usart : USART 实例指针，不得为空。
 *
 * @return 接收数据的低 8 位。
 * @note 阻塞轮询，无超时。
 */
uint8_t USART_ReceiveChar(USART_TypeDef *p_usart)
{
    /* 1. 等待接收数据就绪。 */
    while (RESET == USART_GetFlagStatus(p_usart, USART_FLAG_RXNE))
    {
    }

    /* 2. 读取并返回低 8 位。 */
    return (uint8_t)USART_ReceiveData(p_usart);
}

/**
 * @brief 初始化 USART1 时钟、引脚和通信参数。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
void USART_InitUsart1(void)
{
    /* 1. 开启时钟。 */
    USART_ConfigClock();

    /* 2. 配置收发引脚。 */
    USART_ConfigGpio();

    /* 3. 配置通信参数并使能 USART1。 */
    USART_Configuration();
}
/************************ Function Definitions ************************/
