/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file key.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares key states and user-key initialization and scan APIs.
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

#ifndef __KEY_H__
#define __KEY_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define KEY_RELEASED  ((uint8_t)0U)  /* 未确认按下 */
#define KEY_PRESSED   ((uint8_t)1U)  /* 已确认按下 */

/****************************** Defines ******************************/

/************************ Function Declarations ************************/
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
void Key_Init(void);

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
uint8_t Key_Scan(void);

/************************ Function Declarations ************************/

#endif /* __KEY_H__ */
