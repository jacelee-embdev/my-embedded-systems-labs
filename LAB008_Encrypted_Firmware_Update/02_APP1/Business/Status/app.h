/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file app.h
 *
 * @par dependencies
 * None.
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the application demonstration task.
 *
 * Processing flow:
 *
 * Create App_TaskEntry() as an RTOS task during initialization.
 *
 * @version v1.0 2026-10-04
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __APP_H__
#define __APP_H__

/****************************** Declaring ******************************/

/**
 * @brief 运行状态 LED 和日志任务。
 *
 * 处理步骤：
 * 1. 翻转状态 LED。
 * 2. 输出 APP1 运行日志。
 * 3. 延时 500 个系统节拍后重复。
 *
 * @param[in] p_argument : 任务参数，当前未使用，允许为空。
 *
 * @return 无；任务持续运行，不返回。
 * @note 创建任务前须完成 LED 和日志初始化。
 */
void App_TaskEntry(void *p_argument);

/****************************** Declaring ******************************/

#endif /* __APP_H__ */
