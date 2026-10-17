/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file bootloader.h
 *
 * @par dependencies
 * - None.
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares APP startup, download, and EEPROM state processing.
 *
 * Processing flow:
 *
 * Call Bootloader_ProcessState() after service initialization.
 * Use Bootloader_DownloadApp() to receive and install a package.
 * Bootloader_JumpToApp() also serves the early retained request.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

/************************ Function Declarations ************************/

/**
 * @brief 检查内部 APP 入口并交接控制权。
 * @return 无；启动受限或入口无效时返回。
 * @note 可在外设初始化前调用；APP 负责初始化并重新使能中断。
 */
void Bootloader_JumpToApp(void);

/**
 * @brief 接收加密包并执行解密、备份和安装。
 * @return 无；成功时复位，失败时尝试请求启动当前 APP 或返回。
 * @note 使用静态缓冲区接收 YMODEM 文件到 A 区。
 */
void Bootloader_DownloadApp(void);

/**
 * @brief 按 EEPROM 状态选择启动、安装或旧备份恢复。
 * @return 无；成功时复位或跳转，其他情况返回下载等待。
 * @note 首次试运行启用 IWDG；复位后仍为升级失败状态则尝试恢复旧 APP。
 * @note 恢复失败或无有效旧备份时，禁止启动当前 APP 并等待下载。
 */
void Bootloader_ProcessState(void);

/************************ Function Declarations ************************/

#endif /* __BOOTLOADER_H__ */
