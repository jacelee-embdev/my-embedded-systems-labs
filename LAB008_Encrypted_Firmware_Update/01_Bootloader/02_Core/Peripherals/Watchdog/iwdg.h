/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file iwdg.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares separate IWDG initialization and timed startup APIs.
 *
 * Processing flow:
 *
 * Call IWDG_Init() during peripheral initialization to select divide-by-64
 * without starting the watchdog. Call IWDG_Start() with a timeout in ms
 * before the first APP launch. The first-check flow lets IWDG reset the
 * device after APP confirmation or an unresponsive startup.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __IWDG_H__
#define __IWDG_H__

/****************************** Includes ******************************/

#include <stdint.h>  /* C 标准库：固定宽度整数类型 */

/****************************** Includes ******************************/

/************************ Function Declarations ************************/

/**
 * @brief 配置独立看门狗为固定 64 分频，不启动计时。
 *
 * 允许寄存器写入后设置预分频，供后续 IWDG_Start() 使用。
 *
 * @return 无返回值。
 *
 * @note 在 IWDG 未启动时调用；不等待分频寄存器更新完成。
 * @note 按软件启动模式使用，硬件自动启动模式不保证此时未计时。
 */
void IWDG_Init(void);

/**
 * @brief 按指定毫秒数配置重装载值，并启动独立看门狗。
 *
 * 处理步骤：
 * 1. 按固定 64 分频和 LSI 典型频率 32 kHz 计算重装载值。
 * 2. 写入重装载值并启动 IWDG，由硬件自动开启 LSI。
 * 3. 等待分频及重装载寄存器更新完成，再重装载计数器。
 *
 * @param[in] timeout_ms : 标称超时时间，单位为毫秒，范围 2 至 8192。
 *                        奇数毫秒向上取整至下一个 2 ms。
 *
 * @return 无返回值。
 *
 * @note 须先调用 IWDG_Init()；本接口不校验参数或调用顺序。
 * @note 实际超时时间随 LSI 频率变化；等待寄存器更新不设超时。
 * @note 启动后无法通过软件关闭，须经芯片复位停止。
 */
void IWDG_Start(uint32_t timeout_ms);

/************************ Function Declarations ************************/

#endif /* __IWDG_H__ */
