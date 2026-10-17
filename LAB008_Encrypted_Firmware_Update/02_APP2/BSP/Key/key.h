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
 * Call Key_Init() once before using Key_Scan(). Debouncing uses
 * FreeRTOS task delay while the scheduler runs, or HAL delay otherwise.
 *
 * @version v1.0 2026-10-16
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
 * @note 调用前须完成 Key_Init()，并保持 HAL 毫秒时基运行。
 * @note 调度器运行时按系统节拍延时，否则使用 HAL 延时。
 * @note 仅限启动阶段或任务上下文调用，不得在中断中调用。
 */
uint8_t Key_Scan(void);

/************************ Function Declarations ************************/

#endif /* __KEY_H__ */
