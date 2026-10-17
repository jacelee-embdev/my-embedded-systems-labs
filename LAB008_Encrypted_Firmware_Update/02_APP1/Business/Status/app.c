/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file app.c
 *
 * @par dependencies
 * - app.h
 * - led.h
 * - FreeRTOS.h
 * - task.h
 * - debug.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Runs the APP1 LED and status-log task.
 *
 * Processing flow:
 *
 * Toggle the status LED, log APP1 state, then delay 500 RTOS ticks.
 * Repeat from task context.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "app.h"       /* 业务层：APP 任务接口 */

#include "led.h"       /* BSP 层：状态 LED 接口 */

#include "FreeRTOS.h"  /* 中间件层：RTOS 与日志封装 */
#include "task.h"
#include "debug.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define TAG  "app"  /* 日志标识 */

/****************************** Defines ******************************/

/************************ Function Definitions ************************/

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
void App_TaskEntry(void *p_argument)
{
    (void)p_argument;

    for (;;)
    {
        /* 1. 翻转状态 LED。 */
        LED_Toggle();

        /* 2. 输出 APP1 运行日志。 */
        elog_i(TAG, "APP1 running...\r\n");

        /* 3. 延时 500 个系统节拍。 */
        vTaskDelay(500);
    }
}

/************************ Function Definitions ************************/
