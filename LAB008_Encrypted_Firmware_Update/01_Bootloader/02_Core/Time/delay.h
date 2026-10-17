/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file delay.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares blocking microsecond, millisecond, and second delay APIs.
 *
 * Processing flow:
 *
 * Call Delay_Init() once before using the delay APIs. Call Delay_Tick() from
 * the 1 ms SysTick interrupt handler.
 *
 * @version V1.0 2026-10-14
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __DELAY_H__
#define __DELAY_H__

/****************************** Includes ******************************/

#include <stdint.h>  /* C 标准库：固定宽度整数类型 */

/****************************** Includes ******************************/

/************************ Function Declarations ************************/

/**
 * @brief 初始化 DWT 和 SysTick 延时时基。
 *
 * 处理步骤：
 * 1. 更新内核时钟值并清零毫秒延时计数。
 * 2. 计算每微秒周期数，开启并清零 DWT 周期计数器。
 * 3. 配置 SysTick，产生 1 ms 节拍中断。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 使用延时接口前须调用；本函数会重新配置 SysTick。
 * @note 每次 1 ms SysTick 中断须调用一次 Delay_Tick()。
 */
void Delay_Init(void);

/**
 * @brief 使用 DWT 周期计数器执行阻塞式微秒延时。
 *
 * 处理步骤：
 * 1. 时长或每微秒周期数为零时直接返回。
 * 2. 计算每段延时上限，并逐段等待目标周期数。
 *
 * @param[in] duration_us : 延时时长，单位为微秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 DWT 周期计数器运行。
 * @note 内核时钟变化后须重新初始化延时时基。
 */
void Delay_Us(uint32_t duration_us);

/**
 * @brief 使用 SysTick 递减计数执行阻塞式毫秒延时。
 *
 * 处理步骤：
 * 1. 将请求时长写入剩余毫秒计数。
 * 2. 等待 Delay_Tick() 将计数递减至零。
 *
 * @param[in] duration_ms : 延时时长，单位为毫秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 SysTick 正常运行。
 * @note 与 Delay_S() 共用计数器，不可重入或并发调用。
 * @note 不得在阻塞 SysTick 执行的中断上下文中调用。
 */
void Delay_Ms(uint32_t duration_ms);

/**
 * @brief 逐秒调用毫秒延时，执行阻塞式秒延时。
 *
 * 每秒调用一次 1000 ms 延时，直到完成请求的秒数。
 *
 * @param[in] duration_s : 延时时长，单位为秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 SysTick 正常运行。
 * @note 与 Delay_Ms() 共用计数器，不可重入或并发调用。
 * @note 不得在阻塞 SysTick 执行的中断上下文中调用。
 */
void Delay_S(uint32_t duration_s);

/**
 * @brief 在 1 ms SysTick 中断中更新毫秒延时计数。
 *
 * 计数非零时递减一次，计数为零时保持不变。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 每次 1 ms SysTick 中断须调用一次。
 */
void Delay_Tick(void);

/************************ Function Declarations ************************/

#endif /* __DELAY_H__ */
