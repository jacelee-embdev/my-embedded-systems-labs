/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file fw_update.h
 *
 * @par dependencies
 * - stdint.h
 * - w25qxx_handler.h
 * - FreeRTOS.h
 * - queue.h
 * - semphr.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares firmware update tasks, shared resources, and boot flags.
 *
 * Processing flow:
 *
 * Create FWUpdate_TaskEntry() during RTOS initialization. It receives an
 * encrypted package into A, commits its metadata, and waits for installation
 * confirmation. Bootloader installs the package after a software reset.
 *
 * @version v1.0 2026-10-07
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef FW_UPDATE_H
#define FW_UPDATE_H

/****************************** Includes ******************************/

#include <stdint.h>           /* C 标准库 */

#include "w25qxx_handler.h"    /* BSP 层：Flash 分区容量 */

#include "FreeRTOS.h"          /* 中间件层：RTOS 共享资源 */
#include "queue.h"
#include "semphr.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* EEPROM 启动标志：名称和值与 Bootloader 保持一致。 */
#define APP_STATE_NORMAL_BOOT          (0x00U)  /* 正常启动 */
#define APP_STATE_DOWNLOAD_INCOMPLETE  (0x11U)  /* 下载未完成 */
#define APP_STATE_INSTALL_PENDING      (0x22U)  /* 等待安装 */
#define APP_STATE_FIRST_BOOT_PENDING   (0x33U)  /* 等待首次启动 */
#define APP_STATE_UPDATE_FAILED        (0x44U)  /* 试运行未确认 */
#define APP_STATE_DOWNLOAD_REQUIRED    (0x55U)  /* 必须重新下载 */

#define FW_UPDATE_MAX_SIZE  W25QXX_HANDLER_REGION_SIZE  /* 加密包容量 */

/****************************** Defines ******************************/

/****************************** Declaring ******************************/

extern QueueHandle_t g_fw_data_queue;  /* 文件长度及数据指针队列 */
extern SemaphoreHandle_t g_fw_write_done_semaphore;  /* 分包处理完成通知 */
extern volatile uint8_t g_fw_write_fault;  /* 本次写入故障标记 */

/**
 * @brief 屏蔽异常并请求系统软复位。
 * @return 无；复位后由 Bootloader 处理 EEPROM 状态。
 */
void FWUpdate_SoftReset(void);

/**
 * @brief 接收加密包，并等待安装命令和按键确认。
 *
 * 将包写入 A 区并校验长度，再提交 EEPROM 长度和待安装标志。
 * 完成应答前先准备命令接收，收到安装命令后等待按键复位。
 * 下载失败或按键超时后重新等待下载；按键超时保留待安装标志。
 *
 * @param[in] p_argument 线程参数，当前未使用，允许为空。
 * @return 无；线程持续运行，或复位进入 Bootloader。
 * @note 调度器运行后调用；启动状态清除失败仅记录错误。
 */
void FWUpdate_TaskEntry(void *p_argument);

/****************************** Declaring ******************************/

#endif /* FW_UPDATE_H */
