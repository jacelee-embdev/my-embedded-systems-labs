/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file os_init.h
 *
 * @par dependencies
 * None.
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the RTOS initializer.
 *
 * Processing flow:
 *
 * Call OS_Init() once before starting the scheduler.
 *
 * @version v1.0 2026-10-13
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __OS_INIT_H__
#define __OS_INIT_H__

/****************************** Declaring ******************************/

/**
 * @brief 创建串口接收长度队列及 OTA、APP 任务。
 *
 * 处理步骤：
 * 1. 创建或复用接收长度队列。
 * 2. 依次创建 OTA、APP 任务。
 * 3. 失败时记录日志并返回，保留已创建资源。
 *
 * @return 无。
 * @note 调度器启动前调用一次；任务栈深度按 StackType_t 元素计。
 */
void OS_Init(void);

/****************************** Declaring ******************************/

#endif /* __OS_INIT_H__ */
