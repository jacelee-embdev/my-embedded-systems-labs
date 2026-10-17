/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file led.h
 *
 * @par dependencies
 * - stm32f4xx_hal.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares active-low status LED initialization and control APIs.
 *
 * Processing flow:
 *
 * Call LED_Init() to configure PC13 and turn the LED off.
 * Then call LED_On(), LED_Off(), or LED_Toggle() to control the LED.
 * LED_ON() and LED_OFF() provide direct GPIO control after initialization.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __LED_H__
#define __LED_H__

/****************************** Includes ******************************/

#include "stm32f4xx_hal.h"    /* 芯片 HAL 层：GPIO 定义及接口 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define LED_C13_PORT     GPIOC        /* LED 所属 GPIO 端口 */
#define LED_C13_PIN      GPIO_PIN_13  /* LED 所连接的 PC13 引脚 */

/**
 * @brief 输出高电平，熄灭状态 LED。
 * @note 使用前须先调用 LED_Init()。
 */
#define LED_OFF() \
    HAL_GPIO_WritePin(LED_C13_PORT, LED_C13_PIN, GPIO_PIN_SET)

/**
 * @brief 输出低电平，点亮状态 LED。
 * @note 使用前须先调用 LED_Init()。
 */
#define LED_ON() \
    HAL_GPIO_WritePin(LED_C13_PORT, LED_C13_PIN, GPIO_PIN_RESET)

/****************************** Defines ******************************/

/************************ Function Declarations ************************/

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
void LED_Init(void);

/**
 * @brief 输出低电平，点亮状态 LED。
 *
 * @return 无返回值。
 *
 * @note 调用前须先执行 LED_Init()。
 */
void LED_On(void);

/**
 * @brief 输出高电平，熄灭状态 LED。
 *
 * @return 无返回值。
 *
 * @note 须在 LED_Init() 完成 GPIO 配置后调用。
 */
void LED_Off(void);

/**
 * @brief 翻转输出电平，切换状态 LED 的亮灭状态。
 *
 * @return 无返回值。
 *
 * @note 调用前须先执行 LED_Init()。
 */
void LED_Toggle(void);

/************************ Function Declarations ************************/

#endif /* __LED_H__ */
