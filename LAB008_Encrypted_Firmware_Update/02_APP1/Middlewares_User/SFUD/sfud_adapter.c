/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file sfud_adapter.c
 *
 * @par dependencies
 * - sfud_adapter.h
 * - string.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides local SFUD wrappers for the course flash interfaces.
 *
 * Processing flow:
 *
 * Initialize the library, retain the device pointer, and expose direct
 * erase, write, read and region metadata operations.
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

/****************************** Includes ******************************/

#include "sfud_adapter.h" /* 用户适配层：SFUD 封装接口 */

#include "string.h" /* C 标准库 */

/****************************** Includes ******************************/

/************************ Variable Definitions ************************/

static uint32_t block_sizes[2]   = { 0 }; /* 适配层单独记录的两个分区长度。 */
static uint32_t write_offsets[2] = { 0 }; /* 适配层下一次写入的区内偏移。 */
/* 适配层保存的设备指针。 */
static const sfud_flash *flash = NULL;

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 初始化库并保存配置设备的指针。
 *
 * @return 无。
 *
 * @note 库初始化失败时直接结束；获取设备指针为空时停留在循环中。
 */
void SFUD_W25Q64_Init(void)
{
    /* 1. 初始化库中的设备表。 */
    if (sfud_init() == SFUD_SUCCESS)
    {
        /* 2. 保存配置设备的指针。 */
        flash = sfud_get_device(SFUD_W25Q64_DEVICE_INDEX);
        if (flash == NULL)
        {
            /* 获取不到设备时停留在原有错误处理循环。 */
            while (1)
                ; /* 保留现有停止处理方式。 */
        }
    }
}

/**
 * @brief 通过适配层执行整片擦除。
 *
 * @return 1 表示擦除成功；0 表示设备指针为空或底层操作失败。
 *
 * @note 此适配层成功返回 1，与管理层
 *       W25QxxHandler_EraseChip 的约定不同。
 */
uint8_t SFUD_W25Q64_EraseChip(void)
{
    if (flash == NULL)
        return 0;

    sfud_err result = sfud_chip_erase(flash);
    return (result == SFUD_SUCCESS) ? 1 : 0;
}

/**
 * @brief 请求擦除指定的 64 KiB 分区，并清零适配层写偏移。
 *
 * @param[in] block_index : 分区编号，调用者须保证为 BLOCK_1 或 BLOCK_2。
 *
 * @return 无。
 *
 * @note 本接口不返回底层擦除结果；与仅重置内存状态的管理层接口不同。
 */
void SFUD_Erase_Flash_Block(uint8_t block_index)
{
    if (flash == NULL)
        return;

    uint32_t addr = block_index * BLOCK_SIZE;
    sfud_erase(flash, addr, BLOCK_SIZE);
    write_offsets[block_index] = 0; /* 将写入位置复位到分区首地址。 */
}

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
                              uint32_t length)
{
    if (flash == NULL)
        return 0;

    uint32_t addr = (block_index * BLOCK_SIZE) + write_offsets[block_index];

    /* 先擦除覆盖范围，再写入本次数据。 */
    sfud_err result = sfud_erase_write(flash, addr, length, data);

    if (result == SFUD_SUCCESS)
    {
        write_offsets[block_index] += length;
        return 1;
    }
    return 0;
}

/**
 * @brief 保留写入结束接口，当前不执行额外操作。
 *
 * @param[in] block_index : 保留的分区参数，当前未使用。
 *
 * @return 固定返回 1。
 *
 * @note 本适配层没有待刷新的扇区缓冲区。
 */
uint8_t SFUD_W25Q64_WriteData_End(uint8_t block_index)
{
    /* 当前适配层直接写入，无需刷新缓冲数据。 */
    return 1;
}

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
                             uint16_t *length)
{
    if (flash == NULL)
        return 0;

    uint32_t addr   = block_index * BLOCK_SIZE;
    sfud_err result = sfud_read(flash, addr, *length, data);
    return (result == SFUD_SUCCESS) ? 1 : 0;
}

/**
 * @brief 记录适配层分区长度，并将该分区写偏移清零。
 *
 * @param[in] block_index : 分区编号，无效编号直接忽略。
 * @param[in] app_size    : 待记录的文件字节数。
 *
 * @return 无。
 */
void SFUD_SetBlockParmeter(uint8_t block_index, uint32_t app_size)
{
    if (block_index < 2)
    {
        block_sizes[block_index]   = app_size;
        write_offsets[block_index] = 0; /* 将写入位置复位到分区首地址。 */
    }
}

/**
 * @brief 获取适配层记录的分区长度。
 *
 * @param[in] block_index : 分区编号。
 *
 * @return 由参数设置接口记录的字节数；无效编号返回零。
 *
 * @note 本值不随适配层写入自动更新。
 */
uint32_t SFUD_Read_BlockSize(uint8_t block_index)
{
    if (block_index < 2)
    {
        return block_sizes[block_index];
    }
    return 0;
}

/************************ Function Definitions ************************/
