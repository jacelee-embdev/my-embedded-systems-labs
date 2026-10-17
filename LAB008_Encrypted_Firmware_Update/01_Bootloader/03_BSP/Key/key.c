/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file key.c
 *
 * @par dependencies
 * - key.h
 * - stm32f4xx_gpio.h
 * - stm32f4xx_rcc.h
 * - delay.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Configures the user key and polls its debounced pressed state.
 *
 * Processing flow:
 *
 * Call Key_Init() once before using Key_Scan(). Initialize the delay
 * time base and keep SysTick running for debouncing.
 *
 * @version V1.0 2026-10-14
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "key.h"  /* 用户 BSP 层：按键模块接口 */

#include "stm32f4xx_gpio.h"  /* 厂商 SPL 层：GPIO 和 RCC 驱动接口 */
#include "stm32f4xx_rcc.h"

#include "delay.h"  /* 用户 Core 层：阻塞延时接口 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define KEY_GPIO_PORT         GPIOA       /* 按键端口 */
#define KEY_GPIO_PIN          GPIO_Pin_0  /* 按键引脚 */
#define KEY_DEBOUNCE_TIME_MS  10U         /* 消抖时间，单位为毫秒 */
/****************************** Defines ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 开启 GPIOA 时钟并将 PA0 配置为上拉输入。
 *
 * 处理步骤：
 * 1. 开启 GPIOA 外设时钟。
 * 2. 配置 PA0 为上拉输入，低电平表示按下。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
void Key_Init(void)
{
    GPIO_InitTypeDef gpio_init_structure = {0};  /* 按键引脚配置 */

    /* 1. 开启 GPIOA 外设时钟。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    /* 2. 将 PA0 配置为上拉输入。 */
    gpio_init_structure.GPIO_Pin   = KEY_GPIO_PIN;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_IN;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_Init(KEY_GPIO_PORT, &gpio_init_structure);
}

/**
 * @brief 扫描按键并对按下状态进行消抖。
 *
 * 处理步骤：
 * 1. 读取按键电平，首次为低电平时进入消抖。
 * 2. 延时 10 ms 后再次采样。
 * 3. 两次采样均为低电平时确认按下，否则返回未按下。
 *
 * @param 无。
 *
 * @return 按键扫描结果。
 * @retval KEY_PRESSED  两次采样均为低电平，确认按下。
 * @retval KEY_RELEASED 未确认按下。
 * @note 返回按下状态，持续按住可重复返回 KEY_PRESSED。
 * @note 调用前须完成 Key_Init() 和 Delay_Init()。
 * @note 消抖依赖 SysTick，须保证其正常运行。
 * @note 不得在阻塞 SysTick 执行的中断上下文中调用。
 */
uint8_t Key_Scan(void)
{
    uint8_t key_state = KEY_RELEASED;  /* 按键扫描结果 */

    /* 1. 首次检测为低电平时进入消抖。 */
    if (Bit_RESET == GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN))
    {
        /* 2. 延时消抖后再次采样。 */
        Delay_Ms(KEY_DEBOUNCE_TIME_MS);

        /* 3. 再次采样仍为低电平时确认按下。 */
        if (Bit_RESET == GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_GPIO_PIN))
        {
            key_state = KEY_PRESSED;
        }
    }
    return key_state;
}

/************************ Function Definitions ************************/
