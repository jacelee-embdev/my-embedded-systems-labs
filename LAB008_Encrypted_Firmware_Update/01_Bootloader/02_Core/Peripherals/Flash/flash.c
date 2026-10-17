/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file flash.c
 *
 * @par dependencies
 * - flash.h
 * - stm32f4xx_flash.h
 * - debug.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides internal Flash sector erase and word-programming services.
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

/****************************** Includes ******************************/
#include "flash.h"  /* 用户 Core 层：内部 Flash 模块接口 */

#include "stm32f4xx_flash.h"  /* 厂商 SPL 层：Flash 驱动接口 */

#include "debug.h"  /* 用户中间件层：日志封装接口 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define TAG "flash"  /* 内部 Flash 模块日志标签 */

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000)  /* 16 KB 扇区 */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000)  /* 16 KB 扇区 */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000)  /* 16 KB 扇区 */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000)  /* 16 KB 扇区 */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000)  /* 64 KB 扇区 */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000)  /* 128 KB 扇区 */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000)  /* 128 KB 扇区 */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000)  /* 128 KB 扇区 */
#define ADDR_FLASH_END          ((uint32_t)0x08080000U) /* 结束地址，不含 */
/****************************** Defines ******************************/

/************************ Variable Definitions ************************/
/* STM32F411CE 的 8 个扇区编号，按地址升序排列。 */
static const uint32_t g_flash_sector_ids[8] =
{
    FLASH_Sector_0,
    FLASH_Sector_1,
    FLASH_Sector_2,
    FLASH_Sector_3,
    FLASH_Sector_4,
    FLASH_Sector_5,
    FLASH_Sector_6,
    FLASH_Sector_7,
};
/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 获取指定地址对应的 SPL 扇区编号。
 *
 * @param[in] addr : 待查询的内部 Flash 地址。
 *
 * @return 对应的 SPL 扇区编号。
 * @note 按扇区起始地址比较，不检查地址有效性。
 */
static uint16_t Flash_GetSector(uint32_t addr)
{
    /* 按扇区起始地址逐级判断，末尾默认返回扇区 7。 */
    if (addr < ADDR_FLASH_SECTOR_1)
    {
        return FLASH_Sector_0;
    }
    else if (addr < ADDR_FLASH_SECTOR_2)
    {
        return FLASH_Sector_1;
    }
    else if (addr < ADDR_FLASH_SECTOR_3)
    {
        return FLASH_Sector_2;
    }
    else if (addr < ADDR_FLASH_SECTOR_4)
    {
        return FLASH_Sector_3;
    }
    else if (addr < ADDR_FLASH_SECTOR_5)
    {
        return FLASH_Sector_4;
    }
    else if (addr < ADDR_FLASH_SECTOR_6)
    {
        return FLASH_Sector_5;
    }
    else if (addr < ADDR_FLASH_SECTOR_7)
    {
        return FLASH_Sector_6;
    }

    return FLASH_Sector_7;
}

/**
 * @brief 解锁 Flash 控制器并等待忙状态结束。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @warning 此处为无超时的阻塞忙等待；持续忙状态将使函数无法返回。
 * @note 仅等待 FLASH_BUSY 结束，不在此处处理其他状态。
 */
static void Flash_Unlock(void)
{
    /* 1. 解锁 Flash 控制器。 */
    FLASH_Unlock();

    /* 2. 等待忙状态结束，无超时退出路径。 */
    while (FLASH_BUSY == FLASH_GetStatus())
    {
        /* 保持原有阻塞等待行为。 */
    }
}

/**
 * @brief 锁定 Flash 控制器。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
static void Flash_Lock(void)
{
    FLASH_Lock();
}

/**
 * @brief 擦除一个内部 Flash 扇区。
 *
 * @param[in] sector_id : 待擦除的 SPL 扇区编号。
 *
 * @return SPL Flash 驱动返回的操作状态。
 * @note 使用 VoltageRange_3，要求供电电压为 2.7～3.6 V。
 * @note 解锁后等待忙状态结束，无超时；完成后重新锁定。
 */
static FLASH_Status Flash_EraseSector(uint32_t sector_id)
{
    FLASH_Status flash_status = FLASH_COMPLETE;  /* 扇区擦除状态 */

    /* 1. 解锁并等待忙状态结束。 */
    Flash_Unlock();

    /* 2. 擦除指定扇区并保存驱动返回状态。 */
    flash_status = FLASH_EraseSector(sector_id, VoltageRange_3);

    /* 3. 重新锁定控制器并返回擦除状态。 */
    Flash_Lock();

    return flash_status;
}

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
int Flash_Erase(uint32_t addr, uint32_t size)
{
    uint32_t flash_start_sector = 0;  /* 起始扇区编号 */
    uint32_t flash_end_sector = 0;    /* 末尾扇区编号 */

    /* 1. 拒绝长度为零的请求。 */
    if (0U == size)
    {
        return -1;
    }

    /* 1.1 检查地址和剩余容量，避免末尾地址计算溢出。 */
    if ((addr < ADDR_FLASH_SECTOR_0) ||
        (addr >= ADDR_FLASH_END) ||
        (size > (ADDR_FLASH_END - addr)))
    {
        return -1;
    }

    /* 2. 获取首尾地址对应的扇区编号。 */
    flash_start_sector = Flash_GetSector(addr);
    flash_end_sector = Flash_GetSector(addr + size - 1U);

    /* 3. 遍历扇区表，擦除范围内的完整扇区。 */
    for (uint8_t sector_index = 0U;
        sector_index < (sizeof(g_flash_sector_ids) /
                        sizeof(g_flash_sector_ids[0]));
        sector_index++)
    {
        if ((g_flash_sector_ids[sector_index] >= flash_start_sector) &&
            (g_flash_sector_ids[sector_index] <= flash_end_sector))
        {
            if (FLASH_COMPLETE !=
                Flash_EraseSector(g_flash_sector_ids[sector_index]))
            {
                return -2;
            }
        }
    }

    return 0;
}

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
int Flash_Write(uint32_t address, uint32_t data)
{
    FLASH_Status status = FLASH_COMPLETE;  /* 字编程状态 */

    /* 1. 解锁并执行字编程。 */
    Flash_Unlock();
    status = FLASH_ProgramWord(address, data);

    /* 2. 重新锁定 Flash 控制器。 */
    Flash_Lock();

    /* 3. 编程失败时记录地址和底层状态，并返回失败。 */
    if (FLASH_COMPLETE != status)
    {
        elog_e(TAG, "Word program failed: addr=0x%08lX, status=%d.",
                (unsigned long)address, (int)status);
        return -1;
    }

    return 0;
}

/************************ Function Definitions ************************/
