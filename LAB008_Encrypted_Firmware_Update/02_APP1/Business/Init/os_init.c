/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file os_init.c
 *
 * @par dependencies
 * - os_init.h
 * - stddef.h
 * - uart_handler.h
 * - FreeRTOS.h
 * - task.h
 * - debug.h
 * - app.h
 * - fw_update.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Creates the receive-length queue and application tasks.
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

/****************************** Includes ******************************/

#include "os_init.h"       /* 业务层：RTOS 初始化接口 */

#include <stddef.h>        /* C 标准库 */

#include "uart_handler.h"  /* BSP 层：串口接收适配 */

#include "FreeRTOS.h"      /* 中间件层：RTOS 与日志封装 */
#include "task.h"
#include "debug.h"

#include "app.h"           /* 业务层：APP 与固件更新任务 */
#include "fw_update.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define TAG  "os_init"  /* 日志标识 */

/****************************** Defines ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 创建串口接收长度队列及固件更新、APP 任务。
 *
 * 处理步骤：
 * 1. 创建或复用接收长度队列。
 * 2. 依次创建固件更新、APP 任务。
 * 3. 失败时记录日志并返回，保留已创建资源。
 *
 * @return 无。
 * @note 调度器启动前调用一次；任务栈深度按 StackType_t 元素计。
 */
void OS_Init(void)
{
    /* 1. 创建或复用接收长度队列。 */
    if (0 != UARTHandler_InitReceiveQueue())
    {
        elog_e(TAG, "Queue creation failed.\r\n");
        return;
    }

    /* 2. 创建固件更新任务。 */
    if (pdPASS != xTaskCreate(FWUpdate_TaskEntry,
                              "fw_update_task",
                              512U,
                              NULL,
                              1U,
                              NULL))
    {
        elog_e(TAG, "Swc ota task creation failed.\r\n");
        return;
    }

    /* 3. 创建 APP 任务。 */
    if (pdPASS != xTaskCreate(App_TaskEntry,
                              "app",
                              128U,
                              NULL,
                              1U,
                              NULL))
    {
        elog_e(TAG, "App task creation failed.\r\n");
        return;
    }

    elog_i(TAG, "OS resource initialized successfully.\r\n");
}

/************************ Function Definitions ************************/
