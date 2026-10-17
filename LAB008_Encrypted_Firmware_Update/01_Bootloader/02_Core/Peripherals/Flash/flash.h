/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file flash.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares internal Flash sector erase and word-programming services.
 *
 * Processing flow:
 *
 * Check Flash_Erase() when erasing an image range. Check Flash_Write()
 * for each programmed word, then read it back to verify the stored data.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __FLASH_H__
#define __FLASH_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/************************ Function Declarations ************************/

/**
 * @brief 擦除指定地址范围覆盖的全部内部 Flash 扇区。
 *
 * 处理步骤：
 * 1. 检查长度和地址范围，拒绝空区域或越界请求。
 * 2. 获取首尾地址对应的扇区编号。
 * 3. 逐个擦除范围内的扇区，失败时立即返回。
 *
 * @param[in] addr : 待擦除区域的起始地址。
 * @param[in] size : 待擦除区域的长度，单位为字节。
 *
 * @return 擦除结果。
 * @retval 0 所选扇区全部擦除成功。
 * @retval -1 长度为零或地址范围越界。
 * @retval -2 扇区擦除失败。
 * @note 按完整扇区擦除，可能覆盖指定字节范围之外的数据。
 * @note 解锁忙等待无超时。
 */
int Flash_Erase(uint32_t addr, uint32_t size);

/**
 * @brief 向内部 Flash 写入一个 32 位字并返回编程结果。
 *
 * @param[in] address : 目标 Flash 地址，须按 4 字节对齐。
 * @param[in] data    : 待写入的 32 位数据。
 *
 * @return 字编程结果。
 * @retval 0 驱动报告编程完成。
 * @retval -1 驱动报告编程失败。
 * @note 调用方须保证目标地址有效，并自行回读校验写入结果。
 * @note 失败时输出错误日志；解锁忙等待无超时。
 */
int Flash_Write(uint32_t address, uint32_t data);

/************************ Function Declarations ************************/

#endif /* __FLASH_H__ */
