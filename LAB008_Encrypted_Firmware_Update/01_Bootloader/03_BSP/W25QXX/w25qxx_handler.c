/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file w25qxx_handler.c
 *
 * @par dependencies
 * - w25qxx_handler.h
 * - stddef.h
 * - string.h
 * - debug.h
 * - sfud.h
 * - sfud_adapter.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Manages buffered writes and sequential reads in W25Q A/B regions.
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

/****************************** Includes ******************************/
#include "w25qxx_handler.h"  /* BSP 层：本模块接口 */

#include <stddef.h>  /* C 标准库：空指针和内存操作 */
#include <string.h>

#include "debug.h"  /* 中间件层：日志调试、SFUD 及本地适配接口 */
#include "sfud.h"
#include "sfud_adapter.h"
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define W25QXX_HANDLER_BUFFER_SIZE         4096U  /* 扇区缓冲字节数 */
#define W25QXX_HANDLER_REGION_COUNT        2U     /* 分区数量 */
#define W25QXX_HANDLER_VERIFY_BUFFER_SIZE  256U   /* 回读校验字节数 */
/****************************** Defines ******************************/

/****************************** Types ******************************/
/* 单个分区的读写状态。 */
typedef struct
{
    uint8_t data_buffer[W25QXX_HANDLER_BUFFER_SIZE]; /* 扇区缓冲 */
    uint16_t write_buffer_index; /* 缓冲区下一写入位置 */
    uint32_t write_index;        /* 已写入 Flash 的字节数 */
    uint8_t write_sector_index;  /* 写入扇区序号 */

    uint32_t read_index;         /* 已读取字节数 */
    uint8_t read_sector_index;   /* 读取扇区序号 */
} w25qxx_handler_t;
/****************************** Types ******************************/

/************************ Variable Definitions ************************/
/* A/B 分区管理状态。 */
static w25qxx_handler_t g_w25qxx_handlers[W25QXX_HANDLER_REGION_COUNT] = {0};
/* 通过初始化和存储布局检查的设备。 */
static const sfud_flash *g_p_w25qxx_flash = NULL;
/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/
/**
 * @brief 重置分区读写管理状态。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 无。
 * @note 仅清内存，不擦除 Flash；无效编号忽略。
 * @note 新文件写入前调用。
 */
void W25QxxHandler_ResetRegion(uint8_t region_index)
{
    if (region_index > W25QXX_HANDLER_REGION_B)
    {
        return;
    }
    memset(&g_w25qxx_handlers[region_index], 0,
           sizeof(g_w25qxx_handlers[region_index]));
}

/**
 * @brief 恢复已写长度并复位读取位置。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @param[in] app_size 已有文件字节数，最大 64 KiB。
 * @return 无。
 * @note 不访问 Flash，不恢复待写缓冲；无效参数忽略。
 */
void W25QxxHandler_RestoreRegion(uint8_t region_index, uint32_t app_size)
{
    if ((region_index > W25QXX_HANDLER_REGION_B) ||
        (app_size > W25QXX_HANDLER_REGION_SIZE))
    {
        return;
    }
    W25QxxHandler_ResetRegion(region_index);
    g_w25qxx_handlers[region_index].write_index = app_size;
    g_w25qxx_handlers[region_index].write_sector_index =
        (uint8_t)(app_size / W25QXX_HANDLER_BUFFER_SIZE);
}

/**
 * @brief 初始化 SFUD 设备和两个分区。
 * @param 无。
 * @return 无。
 * @note 要求容量至少 128 KiB，擦除粒度为 4096 字节。
 * @note 设备不可用时，后续读写返回错误。
 */
void W25QxxHandler_Init(void)
{
    const sfud_flash *p_flash = NULL;  /* SFUD 设备 */

    g_p_w25qxx_flash = NULL;
    SFUD_W25Q64_Init();
    p_flash = sfud_get_device(SFUD_W25Q64_DEVICE_INDEX);
    if ((NULL != p_flash) && p_flash->init_ok &&
        (p_flash->chip.capacity >=
         (W25QXX_HANDLER_REGION_COUNT * W25QXX_HANDLER_REGION_SIZE)) &&
        (W25QXX_HANDLER_BUFFER_SIZE == p_flash->chip.erase_gran))
    {
        g_p_w25qxx_flash = p_flash;
    }
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
}

/**
 * @brief 整片擦除并重置分区管理状态。
 * @param 无。
 * @return 擦除结果。
 * @retval W25QXX_HANDLER_OK 擦除成功。
 * @retval W25QXX_HANDLER_ERROR 设备不可用或擦除失败。
 * @note 擦除整个器件，包含 A/B 区外空间。
 * @note 仅成功时重置分区状态。
 */
w25qxx_handler_status_t W25QxxHandler_EraseChip(void)
{
    if ((NULL == g_p_w25qxx_flash) ||
        (SFUD_SUCCESS != sfud_chip_erase(g_p_w25qxx_flash)))
    {
        return W25QXX_HANDLER_ERROR;
    }
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
    return W25QXX_HANDLER_OK;
}

/**
 * @brief 擦写缓冲数据并分段回读校验。
 * @param[in] region_index 分区编号，由调用方检查有效性。
 * @return 刷新结果。
 * @retval W25QXX_HANDLER_OK 刷新成功或缓冲为空。
 * @retval W25QXX_HANDLER_ERROR 状态无效或底层操作失败。
 * @note 调用方保证设备可用；校验缓冲区为 256 字节。
 * @note 擦写或校验失败时作废分区状态，Flash 可能已部分写入。
 */
static w25qxx_handler_status_t W25QxxHandler_FlushBuffer(
    uint8_t region_index)
{
    /* 当前分区状态 */
    w25qxx_handler_t *p_handler = &g_w25qxx_handlers[region_index];
    /* 闪存访问地址 */
    uint32_t address = region_index * W25QXX_HANDLER_REGION_SIZE +
                       p_handler->write_index;
    /* 待刷新字节数 */
    uint16_t length = p_handler->write_buffer_index;
    /* 回读校验缓冲区。 */
    uint8_t read_buffer[W25QXX_HANDLER_VERIFY_BUFFER_SIZE] = {0U};

    if (0U == length)
    {
        return W25QXX_HANDLER_OK;
    }
    if ((p_handler->write_index > W25QXX_HANDLER_REGION_SIZE) ||
        (length > W25QXX_HANDLER_BUFFER_SIZE) ||
        (0U != (p_handler->write_index % W25QXX_HANDLER_BUFFER_SIZE)) ||
        (length > W25QXX_HANDLER_REGION_SIZE - p_handler->write_index))
    {
        return W25QXX_HANDLER_ERROR;
    }
    if (SFUD_SUCCESS != sfud_erase_write(g_p_w25qxx_flash, address, length,
                                       p_handler->data_buffer))
    {
        W25QxxHandler_ResetRegion(region_index);
        return W25QXX_HANDLER_ERROR;
    }

    for (uint16_t byte_index = 0U; byte_index < length; )
    {
        /* 本轮校验长度 */
        uint16_t read_size = length - byte_index;
        if (read_size > sizeof(read_buffer))
        {
            read_size = (uint16_t)sizeof(read_buffer);
        }
        if ((SFUD_SUCCESS != sfud_read(g_p_w25qxx_flash, address + byte_index,
                                      read_size, read_buffer)) ||
            (0 != memcmp(read_buffer,
                         &p_handler->data_buffer[byte_index], read_size)))
        {
            W25QxxHandler_ResetRegion(region_index);
            return W25QXX_HANDLER_ERROR;
        }
        byte_index += read_size;
    }

    p_handler->write_index += length;
    p_handler->write_sector_index =
        (uint8_t)(p_handler->write_index / W25QXX_HANDLER_BUFFER_SIZE);
    p_handler->write_buffer_index = 0U;
    return W25QXX_HANDLER_OK;
}

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
    uint8_t region_index, const uint8_t *p_data, uint32_t length)
{
    w25qxx_handler_t *p_handler = NULL;  /* 当前分区状态 */

    if ((region_index > W25QXX_HANDLER_REGION_B) ||
        ((NULL == p_data) && (0U != length)) ||
        (NULL == g_p_w25qxx_flash))
    {
        return W25QXX_HANDLER_ERROR;
    }
    p_handler = &g_w25qxx_handlers[region_index];
    if ((p_handler->write_index > W25QXX_HANDLER_REGION_SIZE) ||
        (p_handler->write_buffer_index >= W25QXX_HANDLER_BUFFER_SIZE) ||
        (0U != (p_handler->write_index % W25QXX_HANDLER_BUFFER_SIZE)) ||
        (p_handler->write_buffer_index >
         W25QXX_HANDLER_REGION_SIZE - p_handler->write_index) ||
        (length > W25QXX_HANDLER_REGION_SIZE - p_handler->write_index -
                  p_handler->write_buffer_index))
    {
        return W25QXX_HANDLER_ERROR;
    }

    for (uint32_t byte_index = 0U; byte_index < length; byte_index++)
    {
        p_handler->data_buffer[p_handler->write_buffer_index++] =
            p_data[byte_index];
        if (W25QXX_HANDLER_BUFFER_SIZE == p_handler->write_buffer_index)
        {
            if (W25QXX_HANDLER_OK !=
                W25QxxHandler_FlushBuffer(region_index))
            {
                return W25QXX_HANDLER_ERROR;
            }
        }
    }
    return W25QXX_HANDLER_OK;
}

/**
 * @brief 刷新分区剩余缓冲数据。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 刷新结果。
 * @retval W25QXX_HANDLER_OK 刷新成功或缓冲为空。
 * @retval W25QXX_HANDLER_ERROR 参数无效或底层操作失败。
 * @note 空缓冲可重复刷新，不重复累计长度。
 * @note 擦写或校验失败时作废分区状态，Flash 可能已部分写入。
 */
w25qxx_handler_status_t W25QxxHandler_Flush(uint8_t region_index)
{
    if ((region_index > W25QXX_HANDLER_REGION_B) || (NULL == g_p_w25qxx_flash))
    {
        return W25QXX_HANDLER_ERROR;
    }
    return W25QxxHandler_FlushBuffer(region_index);
}

/**
 * @brief 获取分区已写入的有效长度。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 已写字节数，无效编号返回零。
 * @note 不含尚未刷新的缓冲数据。
 */
uint32_t W25QxxHandler_GetWrittenSize(uint8_t region_index)
{
    if (region_index > W25QXX_HANDLER_REGION_B)
    {
        return 0U;
    }
    return g_w25qxx_handlers[region_index].write_index;
}

/**
 * @brief 将分区读取位置复位到区首。
 * @param[in] region_index 分区编号：0 为 A，1 为 B。
 * @return 无。
 * @note 无效编号忽略，写入状态不变。
 */
void W25QxxHandler_ResetReadPosition(uint8_t region_index)
{
    if (region_index <= W25QXX_HANDLER_REGION_B)
    {
        g_w25qxx_handlers[region_index].read_index = 0U;
        g_w25qxx_handlers[region_index].read_sector_index = 0U;
    }
}

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
    uint8_t region_index, uint8_t *p_data, uint16_t *p_length)
{
    w25qxx_handler_t *p_handler = NULL;  /* 当前分区状态 */
    uint32_t read_length = 0U;  /* 本次读取长度 */
    uint32_t address = 0U;  /* 闪存访问地址 */

    if (NULL == p_length)
    {
        return W25QXX_HANDLER_READ_ERROR;
    }
    *p_length = 0U;
    if ((region_index > W25QXX_HANDLER_REGION_B) || (NULL == p_data) ||
        (NULL == g_p_w25qxx_flash))
    {
        return W25QXX_HANDLER_READ_ERROR;
    }
    p_handler = &g_w25qxx_handlers[region_index];
    if (p_handler->read_index >= p_handler->write_index)
    {
        return W25QXX_HANDLER_READ_END;
    }
    read_length = p_handler->write_index - p_handler->read_index;
    if (read_length > W25QXX_HANDLER_BUFFER_SIZE)
    {
        read_length = W25QXX_HANDLER_BUFFER_SIZE;
    }
    address = region_index * W25QXX_HANDLER_REGION_SIZE + p_handler->read_index;
    if (SFUD_SUCCESS !=
        sfud_read(g_p_w25qxx_flash, address, read_length, p_data))
    {
        return W25QXX_HANDLER_READ_ERROR;
    }
    p_handler->read_index += read_length;
    p_handler->read_sector_index =
        (uint8_t)(p_handler->read_index / W25QXX_HANDLER_BUFFER_SIZE);
    *p_length = (uint16_t)read_length;
    return W25QXX_HANDLER_READ_OK;
}

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
int W25QxxHandler_Test(void)
{
    /* 初始化时已通过设备和擦除粒度检查的 SFUD 设备。 */
    const sfud_flash *p_flash = g_p_w25qxx_flash;
    /* 写入末扇区起始位置的固定数据。 */
    const uint8_t write_buf[8] =
    {
        0x01U, 0x02U, 0x03U,0x04U, 
        0x05U, 0x06U, 0x07U,0x08U
    };
    uint8_t read_buf[8] = {0U}; /* 回读缓冲区。 */
    /* Handler 要求擦除粒度与 4096 字节缓冲区大小一致。 */
    const uint32_t sector_size = W25QXX_HANDLER_BUFFER_SIZE;
    uint32_t address = 0U; /* 最后一个扇区的起始地址。 */

    /* 先检查容量，避免计算末扇区地址时下溢。 */
    if ((NULL == p_flash) || (p_flash->chip.capacity < sector_size))
    {
        return -1;
    }
    address = p_flash->chip.capacity - sector_size;

    /* 擦除末扇区，写入并读回测试数据。 */
    if (SFUD_SUCCESS != sfud_erase(p_flash, address, sector_size))
    {
        return -2;
    }
    if (SFUD_SUCCESS !=
        sfud_write(p_flash, address, sizeof(write_buf), write_buf))
    {
        return -3;
    }
    if (SFUD_SUCCESS !=
        sfud_read(p_flash, address, sizeof(read_buf), read_buf))
    {
        return -4;
    }

    /* 打印读回的地址和数据。 */
    for (uint8_t byte_index = 0U;
         byte_index < sizeof(read_buf); byte_index++)
    {
        elog_i("w25qxx_handler", "addr=0x%08X, read=0x%02X",
               (unsigned int)(address + byte_index),
               (unsigned int)read_buf[byte_index]);
    }

    /* 所有回读数据打印完成后再比较。 */
    if (0 != memcmp(write_buf, read_buf, sizeof(write_buf)))
    {
        return -5;
    }

    /* 验证成功后，再次擦除末扇区。 */
    return (SFUD_SUCCESS == sfud_erase(p_flash, address, sector_size))
           ? 0 : -6;
}

/************************ Function Definitions ************************/
