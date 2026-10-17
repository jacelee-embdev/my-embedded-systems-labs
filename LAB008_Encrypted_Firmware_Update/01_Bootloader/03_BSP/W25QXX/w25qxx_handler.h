/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file w25qxx_handler.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares buffered A/B region access and status types.
 *
 * Processing flow:
 *
 * Initialize SFUD and region state, then append and flush file data.
 * Restore a saved length when needed and read the recorded valid bytes.
 * W25QxxHandler_Test() writes 00-07 to the last sector, logs and verifies
 * the readback, then erases that sector again on a match.
 *
 * @par provenance
 * Adapted with reference to the course driver portability example.
 * SFUD library code retains its original upstream license.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __W25QXX_HANDLER_H__
#define __W25QXX_HANDLER_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define W25QXX_HANDLER_REGION_A     0U        /* A 区编号 */
#define W25QXX_HANDLER_REGION_B     1U        /* B 区编号 */
#define W25QXX_HANDLER_REGION_SIZE  0x10000U  /* 分区容量，字节 */
/****************************** Defines ******************************/

/****************************** Types ******************************/
/* 写入、刷新和擦除结果。 */
typedef enum
{
    W25QXX_HANDLER_OK = 0,    /* 操作成功 */
    W25QXX_HANDLER_ERROR = 1  /* 参数或底层操作失败 */
} w25qxx_handler_status_t;

/* 顺序读取结果。 */
typedef enum
{
    W25QXX_HANDLER_READ_OK = 0,    /* 读取成功 */
    W25QXX_HANDLER_READ_END = 1,   /* 已无有效数据 */
    W25QXX_HANDLER_READ_ERROR = 2  /* 参数或读取失败 */
} w25qxx_handler_read_status_t;
/****************************** Types ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 重置分区读写管理状态。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 无。
 * @note 仅清内存，不擦除 Flash；无效编号忽略。
 * @note 新文件写入前调用。
 */
void W25QxxHandler_ResetRegion(uint8_t region_index);

/**
 * @brief 恢复已写长度并复位读取位置。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @param[in] app_size 已有文件字节数，最大 64 KiB。
 * @return 无。
 * @note 不访问 Flash，不恢复待写缓冲；无效参数忽略。
 */
void W25QxxHandler_RestoreRegion(uint8_t region_index, uint32_t app_size);

/**
 * @brief 初始化 SFUD 设备和两个分区。
 * @param 无。
 * @return 无。
 * @note 要求容量至少 128 KiB，擦除粒度为 4096 字节。
 * @note 设备不可用时，后续读写返回错误。
 */
void W25QxxHandler_Init(void);

/**
 * @brief 整片擦除并重置分区管理状态。
 * @param 无。
 * @return 擦除结果。
 * @retval W25QXX_HANDLER_OK 擦除成功。
 * @retval W25QXX_HANDLER_ERROR 设备不可用或擦除失败。
 * @note 擦除整个器件，包含 A/B 区外空间。
 * @note 仅成功时重置分区状态。
 */
w25qxx_handler_status_t W25QxxHandler_EraseChip(void);

/**
 * @brief 追加数据，满扇区时刷新并校验。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @param[in] p_data 数据缓冲区，非零长度时非空。
 * @param[in] length 追加字节数，须在分区剩余容量内。
 * @return 追加结果。
 * @retval W25QXX_HANDLER_OK 处理成功。
 * @retval W25QXX_HANDLER_ERROR 参数无效或底层操作失败。
 * @note 尾部不足整扇区时，新文件须先重置管理状态。
 * @note 擦写或校验失败时作废分区状态，Flash 可能已部分写入。
 */
w25qxx_handler_status_t W25QxxHandler_WriteData(
    uint8_t region_index, const uint8_t *p_data, uint32_t length);

/**
 * @brief 刷新分区剩余缓冲数据。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 刷新结果。
 * @retval W25QXX_HANDLER_OK 刷新成功或缓冲为空。
 * @retval W25QXX_HANDLER_ERROR 参数无效或底层操作失败。
 * @note 空缓冲可重复刷新，不重复累计长度。
 * @note 擦写或校验失败时作废分区状态，Flash 可能已部分写入。
 */
w25qxx_handler_status_t W25QxxHandler_Flush(uint8_t region_index);

/**
 * @brief 获取分区已写入的有效长度。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 已写字节数，无效编号返回零。
 * @note 不含尚未刷新的缓冲数据。
 */
uint32_t W25QxxHandler_GetWrittenSize(uint8_t region_index);

/**
 * @brief 将分区读取位置复位到区首。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 无。
 * @note 无效编号忽略，写入状态不变。
 */
void W25QxxHandler_ResetReadPosition(uint8_t region_index);

/**
 * @brief 从当前位置顺序读取有效数据。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @param[out] p_data 接收缓冲区，至少容纳 4096 字节。
 * @param[out] p_length 实际读取长度输出，不得为空。
 * @return 读取结果。
 * @retval W25QXX_HANDLER_READ_OK 读取成功。
 * @retval W25QXX_HANDLER_READ_END 已无有效数据。
 * @retval W25QXX_HANDLER_READ_ERROR 参数无效或读取失败。
 * @note 长度输出先清零，每次最多读取 4096 字节。
 * @note 仅成功时推进读取位置。
 */
w25qxx_handler_read_status_t W25QxxHandler_ReadData(
    uint8_t region_index, uint8_t *p_data, uint16_t *p_length);

/**
 * @brief 测试整颗 Flash 最后一个扇区的擦除、写入和回读。
 *
 * 处理步骤：
 * 1. 检查设备和容量，计算最后一个 4 KiB 扇区的起始地址。
 * 2. 擦除该扇区，写入 8 字节固定数据 0x00～0x07 并读回。
 * 3. 逐字节打印回读地址和数据，比较全部 8 字节。
 * 4. 验证成功后再次擦除该扇区，返回操作结果。
 *
 * @param 无。
 *
 * @return 测试结果。
 * @retval 0 读写验证及最终擦除操作均成功。
 * @retval -1 设备不可用或容量不足。
 * @retval -2 首次擦除失败。
 * @retval -3 写入失败。
 * @retval -4 读取失败。
 * @retval -5 回读数据不一致。
 * @retval -6 最终擦除失败。
 * @note 须先初始化日志并调用 W25QxxHandler_Init()。
 * @note 会擦除末扇区原数据；任一步失败即返回，不执行后续步骤。
 */
int W25QxxHandler_Test(void);

/************************ Function Declarations ************************/

#endif /* __W25QXX_HANDLER_H__ */
