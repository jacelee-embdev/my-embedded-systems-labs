/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file sfud_adapter.h
 *
 * @par dependencies
 * - inc/sfud.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares local SFUD wrappers and A/B region constants.
 *
 * Processing flow:
 *
 * Expose the course-compatible wrappers used to initialize SFUD and
 * access flash regions; wrapper success values are documented per API.
 *
 * @par provenance
 * Adapted with reference to the course driver portability example.
 * SFUD library code retains its original upstream license.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __SFUD_ADAPTER_H
#define __SFUD_ADAPTER_H

/****************************** Includes ******************************/

#include "inc/sfud.h" /* 中间件层：SFUD 接口与配置 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* 保留本地分区编号与 64 KiB 分区大小。 */
#define BLOCK_1    (0)
#define BLOCK_2    (1)
#define BLOCK_SIZE (0x10000)

/****************************** Defines ******************************/

/****************************** Declaring ******************************/

/**
 * @brief 初始化库并保存配置设备的指针。
 *
 * @return 无。
 *
 * @note 库初始化失败时直接结束；获取设备指针为空时停留在循环中。
 */
void SFUD_W25Q64_Init(void);
/**
 * @brief 通过适配层执行整片擦除。
 *
 * @return 1 表示擦除成功；0 表示设备指针为空或底层操作失败。
 *
 * @note 此适配层成功返回 1，与管理层
 *       W25QxxHandler_EraseChip 的约定不同。
 */
uint8_t SFUD_W25Q64_EraseChip(void);
/**
 * @brief 请求擦除指定的 64 KiB 分区，并清零适配层写偏移。
 *
 * @param[in] block_index : 分区编号，调用者须保证为 BLOCK_1 或 BLOCK_2。
 *
 * @return 无。
 *
 * @note 本接口不返回底层擦除结果；与仅重置内存状态的管理层接口不同。
 */
void SFUD_Erase_Flash_Block(uint8_t block_index);
/**
 * @brief 在当前偏移处执行擦除写入，成功后累计适配层偏移。
 *
 * @param[in] block_index : 有效分区编号。
 * @param[in] data        : 待写入的缓冲区。
 * @param[in] length      : 本次写入字节数。
 *
 * @return 1 表示写入成功；0 表示设备指针为空或底层操作失败。
 *
 * @note 每次调用均执行擦除写入，可能擦除同一擦除单元内的已有数据。
 */
uint8_t SFUD_W25Q64_WriteData(uint8_t block_index,
                              uint8_t *data,
                              uint32_t length);
/**
 * @brief 保留写入结束接口，当前不执行额外操作。
 *
 * @param[in] block_index : 保留的分区参数，当前未使用。
 *
 * @return 固定返回 1。
 *
 * @note 本适配层没有待刷新的扇区缓冲区。
 */
uint8_t SFUD_W25Q64_WriteData_End(uint8_t block_index);
/**
 * @brief 从指定分区首地址读取指定长度的数据。
 *
 * @param[in] block_index : 有效分区编号。
 * @param[out] data       : 接收缓冲区。
 * @param[in] length      : 待读取字节数的地址；本接口不修改该值。
 *
 * @return 1 表示读取成功；0 表示设备指针为空或底层操作失败。
 *
 * @note 每次均从区首读取，不维护顺序读取索引。
 */
uint8_t SFUD_W25Q64_ReadData(uint8_t block_index,
                             uint8_t *data,
                             uint16_t *length);
/**
 * @brief 记录适配层分区长度，并将该分区写偏移清零。
 *
 * @param[in] block_index : 分区编号，无效编号直接忽略。
 * @param[in] app_size    : 待记录的文件字节数。
 *
 * @return 无。
 */
void SFUD_SetBlockParmeter(uint8_t block_index, uint32_t app_size);
/**
 * @brief 获取适配层记录的分区长度。
 *
 * @param[in] block_index : 分区编号。
 *
 * @return 由参数设置接口记录的字节数；无效编号返回零。
 *
 * @note 本值不随适配层写入自动更新。
 */
uint32_t SFUD_Read_BlockSize(uint8_t block_index);

/****************************** Declaring ******************************/

#endif /* __SFUD_ADAPTER_H */
