/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file led.c
 *
 * @par dependencies
 * - led.h
 * - stm32f4xx_rcc.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements active-low status LED initialization and control.
 *
 * Processing flow:
 *
 * Call LED_Init() to configure PC13 and turn the LED off.
 * Then call LED_On(), LED_Off(), or LED_Toggle() to control the LED.
 * LED_ON() and LED_OFF() provide direct GPIO control after initialization.
 *
 * @version V1.0 2026-10-14
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "led.h"              /* 用户 BSP 层：LED 模块接口 */

#include "stm32f4xx_rcc.h"     /* 芯片 SPL 层：外设时钟接口 */

/****************************** Includes ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 初始化状态 LED，并默认熄灭。
 *
 * 处理步骤：
 * 1. 使能 GPIOC 外设时钟。
 * 2. 将 PC13 配置为推挽输出，无上下拉。
 * 3. 配置完成后调用 LED_Off()，输出高电平使 LED 熄灭。
 *
 * @return 无返回值。
 */
void LED_Init(void)
{
    GPIO_InitTypeDef gpio_init_structure = {0};  /* LED 引脚配置 */

    /* 1. 使能 GPIOC 外设时钟。 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* 2. 将 PC13 配置为推挽输出，无上下拉。 */
    gpio_init_structure.GPIO_Pin   = LED_C13_PIN;
    gpio_init_structure.GPIO_Mode  = GPIO_Mode_OUT;
    gpio_init_structure.GPIO_Speed = GPIO_Speed_100MHz;
    gpio_init_structure.GPIO_OType = GPIO_OType_PP;
    gpio_init_structure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_C13_PORT, &gpio_init_structure);

    /* 3. 输出高电平，使 LED 默认熄灭。 */
    LED_Off();
}

/**
 * @brief 输出低电平，点亮状态 LED。
 *
 * @return 无返回值。
 *
 * @note 调用前须先执行 LED_Init()。
 */
void LED_On(void)
{
    /* 将 LED 引脚输出复位为低电平。 */
    GPIO_ResetBits(LED_C13_PORT, LED_C13_PIN);
}

/**
 * @brief 输出高电平，熄灭状态 LED。
 *
 * @return 无返回值。
 *
 * @note 须在 LED_Init() 完成 GPIO 配置后调用。
 */
void LED_Off(void)
{
    /* 将 LED 引脚输出置位为高电平。 */
    GPIO_SetBits(LED_C13_PORT, LED_C13_PIN);
}

/**
 * @brief 翻转输出电平，切换状态 LED 的亮灭状态。
 *
 * @return 无返回值。
 *
 * @note 调用前须先执行 LED_Init()。
 */
void LED_Toggle(void)
{
    /* 翻转 LED 引脚的输出电平。 */
    GPIO_ToggleBits(LED_C13_PORT, LED_C13_PIN);
}

/************************ Function Definitions ************************/
