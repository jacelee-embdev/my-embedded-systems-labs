/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file bootloader.c
 *
 * @par dependencies
 * - bootloader.h
 * - stdint.h
 * - string.h
 * - stm32f4xx.h
 * - stm32f4xx_rcc.h
 * - stm32f4xx_iwdg.h
 * - main.h
 * - flash.h
 * - delay.h
 * - iwdg.h
 * - key.h
 * - at24cxx.h
 * - w25qxx_handler.h
 * - aes.h
 * - ymodem.h
 * - debug.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Manages APP startup, installation, first boots, and rollback.
 *
 * Processing flow:
 *
 * Read the EEPROM state to select startup, download, or recovery.
 * Receive into A, decrypt into B, and preserve a valid old APP in A.
 * Install B, save its length, and reset for the first APP trial.
 * Start IWDG with a nominal 6000 ms timeout before the first APP launch.
 * If the failure state remains after reset, try to restore the old APP.
 * Wait for a download if no valid backup can be restored.
 * Keep reset jump requests in retained RAM.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/******************************* Includes *******************************/

#include "bootloader.h"  /* 本模块接口 */

#include <stdint.h>  /* C 标准库 */
#include <string.h>

#include "stm32f4xx.h"  /* 厂商库层 */
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_iwdg.h"

#include "main.h"  /* Core 层 */
#include "flash.h"
#include "delay.h"
#include "iwdg.h"

#include "key.h"  /* BSP 层 */
#include "at24cxx.h"
#include "w25qxx_handler.h"

#include "aes.h"  /* 中间件层 */
#include "ymodem.h"
#include "debug.h"

/******************************* Includes *******************************/

/******************************** Defines ********************************/

#define TAG "bootloader"  /* 日志标签 */
#define YMODEM_RECEIVE_BUFFER_SIZE (1024U)  /* YMODEM 接收缓冲区大小 */

#define APP_RUN_BASE_ADDR ((uint32_t)0x08008000U)  /* APP 运行区基址 */
#define APP_RUN_SIZE ((uint32_t)0x00018000U)  /* APP 运行区容量 */
#define APP_BACKUP_SIZE \
    ((uint32_t)W25QXX_HANDLER_REGION_SIZE)  /* 加密包长度上限 */
#define APP_SRAM_SIZE ((uint32_t)0x00020000U)  /* SRAM 容量 */

/* EEPROM 0x00 保存的升级状态 */
#define APP_STATE_NORMAL_BOOT         (0x00)  /* 正常启动，允许启动有效 APP */
#define APP_STATE_DOWNLOAD_INCOMPLETE (0x11)  /* 下载未完成，升级包不可用于安装 */
#define APP_STATE_INSTALL_PENDING     (0x22)  /* 升级包下载完成，等待安装 */
#define APP_STATE_FIRST_BOOT_PENDING  (0x33)  /* 新 APP 安装完成，等待首次试运行 */
#define APP_STATE_UPDATE_FAILED       (0x44)  /* 升级失败，复位后尝试恢复旧 APP */
#define APP_STATE_DOWNLOAD_REQUIRED   (0x55)  /* 禁止启动当前 APP，等待按键下载 */

/* 旧备份记录：标志、版本、保留、长度、明文 CRC、记录 CRC */
#define BOOT_ROLLBACK_RECORD_ADDR (0x10U)  /* EEPROM 记录起始地址 */
#define BOOT_ROLLBACK_RECORD_SIZE (16U)  /* 记录字节数 */
#define BOOT_ROLLBACK_VALID (0xA5U)  /* 有效记录标志 */
#define BOOT_ROLLBACK_VERSION (1U)  /* 记录格式版本 */

/******************************** Defines ********************************/

/********************************* Types *********************************/

/* APP 复位入口函数类型 */
typedef void (*app_entry_t)(void);

/********************************* Types *********************************/

/************************* Variable Definitions *************************/

/* AES-256 密钥配置，只读数据由链接器放入内部 Flash。 */
static const uint8_t g_app_aes_key[32] = {
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U
};
/* CBC 初始 IV 配置，每个升级包解密前复制到工作 IV。 */
static const uint8_t g_app_aes_initial_iv[16] = {
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U,
    0x31U, 0x32U, 0x31U, 0x32U
};

/* YMODEM 接收暂存，避免占用启动栈。 */
static uint8_t g_ymodem_receive_buffer[YMODEM_RECEIVE_BUFFER_SIZE] = {0U};
/* 外部闪存搬运缓冲区。 */
static uint8_t g_flash_read_buffer[4096] = {0U};
/* 当前镜像不允许启动时置位。 */
static uint8_t g_app_launch_blocked = 0U;

/************************* Variable Definitions *************************/

/************************* Function Definitions *************************/

/**
 * @brief 请求系统软件复位。
 * @return 无；复位后不返回。
 * @note 调用前须保存 EEPROM 状态或保留 RAM 请求。
 */
static void Bootloader_SoftwareReset(void)
{
    NVIC_SystemReset();
}

/**
 * @brief 检查 APP 栈顶、复位入口范围和 Thumb 状态。
 * @param[in] p_app_data : 包含初始向量表的镜像数据。
 * @param[in] app_size   : APP 明文字节数。
 * @return 操作结果。
 * @retval 0 入口有效。
 * @retval -1 参数或长度无效。
 * @retval -2 栈顶地址或复位入口无效。
 * @note 仅检查启动入口，不校验完整镜像。
 */
static int Bootloader_CheckAppEntry(const uint8_t *p_app_data,
                                    uint32_t app_size)
{
    uint32_t app_sp = 0U; /* APP 向量表中的初始栈顶地址。 */
    uint32_t app_entry_addr = 0U; /* APP 向量表中的复位入口地址。 */

    if ((NULL == p_app_data) || (app_size < 8U) || (app_size > APP_RUN_SIZE))
    {
        return -1;
    }
    memcpy(&app_sp, p_app_data, sizeof(app_sp));
    memcpy(&app_entry_addr, p_app_data + 4U, sizeof(app_entry_addr));
    if ((app_sp <= SRAM_BASE) || (app_sp > SRAM_BASE + APP_SRAM_SIZE) ||
        (0U != (app_sp & 7U)) || (0U == (app_entry_addr & 1U)) ||
        ((app_entry_addr & ~1U) < APP_RUN_BASE_ADDR) ||
        ((app_entry_addr & ~1U) >= APP_RUN_BASE_ADDR + app_size))
    {
        return -2;
    }
    return 0;
}

/**
 * @brief 累加 CRC-32/ISO-HDLC。
 * @param[in] crc    : 当前累加值，首次传入 0xFFFFFFFF。
 * @param[in] p_data : 有效数据指针，不得为空。
 * @param[in] size   : 数据字节数。
 * @return 更新后的累加值；调用者负责最终异或 0xFFFFFFFF。
 */
static uint32_t Bootloader_UpdateCRC32(uint32_t crc,
                                     const uint8_t *p_data,
                                     uint32_t size)
{
    for (uint32_t index = 0U; index < size; index++)
    {
        crc ^= p_data[index];
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = (0U != (crc & 1U))
                      ? ((crc >> 1U) ^ 0xEDB88320U) : (crc >> 1U);
        }
    }
    return crc;
}

/**
 * @brief 将数值编码为四字节小端数据。
 * @param[out] p_data : 至少四字节的有效输出缓冲区。
 * @param[in] value   : 待编码数值。
 * @return 无。
 */
static void Bootloader_EncodeU32(uint8_t *p_data, uint32_t value)
{
    for (uint8_t index = 0U; index < 4U; index++)
    {
        p_data[index] = (uint8_t)(value >> (index * 8U));
    }
}

/**
 * @brief 解码四字节小端数据。
 * @param[in] p_data : 至少四字节的有效输入缓冲区。
 * @return 解码后的数值。
 */
static uint32_t Bootloader_DecodeU32(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] | ((uint32_t)p_data[1] << 8U) |
           ((uint32_t)p_data[2] << 16U) | ((uint32_t)p_data[3] << 24U);
}

/**
 * @brief 写入 EEPROM，等待写完并回读确认。
 * @param[in] address : EEPROM 起始地址。
 * @param[in] p_data  : 待写数据，不得为空。
 * @param[in] size    : 字节数，范围为 1～16。
 * @return 操作结果。
 * @retval 0 写入且回读一致。
 * @retval -1 参数无效。
 * @retval -2 写入失败。
 * @retval -3 读取失败。
 * @retval -4 回读数据不一致。
 */
static int Bootloader_WriteVerified(uint16_t address,
                                    const uint8_t *p_data,
                                    uint16_t size)
{
    uint8_t readback[BOOT_ROLLBACK_RECORD_SIZE] = {0U}; /* EEPROM 回读数据。 */
    if ((NULL == p_data) || (0U == size) || (size > sizeof(readback)))
    {
        return -1;
    }
    if (0 != AT24Cxx_WriteBytes((uint8_t *)p_data, address, size))
    {
        return -2;
    }
    Delay_Ms(10U);
    if (0 != AT24Cxx_ReadBytes(readback, address, size))
    {
        return -3;
    }
    return (0 == memcmp(readback, p_data, size)) ? 0 : -4;
}

/**
 * @brief 保存并确认 EEPROM 升级状态。
 * @param[in] state : 升级状态字节。
 * @return 操作结果。
 * @retval 0 写入且回读验证成功。
 * @retval -2 写入失败。
 * @retval -3 读取失败。
 * @retval -4 回读数据不一致。
 */
static int Bootloader_SaveState(uint8_t state)
{
    return Bootloader_WriteVerified(0x00U, &state, sizeof(state));
}

/**
 * @brief 作废旧备份记录，成功后才允许覆盖 A 区。
 * @return 操作结果。
 * @retval 0 写入且回读验证成功。
 * @retval -2 写入失败。
 * @retval -3 读取失败。
 * @retval -4 回读数据不一致。
 */
static int Bootloader_InvalidateBackup(void)
{
    uint8_t invalid = 0U; /* 备份无效标志。 */
    return Bootloader_WriteVerified(BOOT_ROLLBACK_RECORD_ADDR,
                                    &invalid, sizeof(invalid));
}

/**
 * @brief 恢复 A 区管理长度并检查旧备份入口及完整 CRC。
 * @param[in] size : 旧 APP 明文字节数。
 * @param[in] crc  : 旧 APP 完整明文 CRC。
 * @return 操作结果。
 * @retval 0 备份长度、入口及完整 CRC 均有效。
 * @retval -1 长度无效。
 * @retval -2 读取、数据长度或入口检查失败。
 * @retval -3 完整 CRC 不一致。
 */
static int Bootloader_VerifyBackup(uint32_t size, uint32_t crc)
{
    uint32_t total = 0U; /* 累计读取长度。 */
    uint32_t calculated_crc = 0xFFFFFFFFU; /* 备份 CRC 累加值。 */
    uint16_t length = 0U; /* 本次读取长度。 */
    if ((size < 8U) || (size > APP_RUN_SIZE) ||
        (size > W25QXX_HANDLER_REGION_SIZE))
    {
        return -1;
    }
    W25QxxHandler_RestoreRegion(W25QXX_HANDLER_REGION_A, size);
    W25QxxHandler_ResetReadPosition(W25QXX_HANDLER_REGION_A);
    while (total < size)
    {
        if ((W25QXX_HANDLER_READ_OK !=
             W25QxxHandler_ReadData(W25QXX_HANDLER_REGION_A,
                                   g_flash_read_buffer, &length)) ||
            (0U == length) || (length > sizeof(g_flash_read_buffer)) ||
            (length > size - total) ||
            ((0U == total) && ((length < 8U) ||
             (0 != Bootloader_CheckAppEntry(g_flash_read_buffer, size)))))
        {
            return -2;
        }
        calculated_crc = Bootloader_UpdateCRC32(calculated_crc,
                                               g_flash_read_buffer, length);
        total += length;
    }
    W25QxxHandler_ResetReadPosition(W25QXX_HANDLER_REGION_A);
    return ((calculated_crc ^ 0xFFFFFFFFU) == crc) ? 0 : -3;
}

/**
 * @brief 提交旧备份记录，最后写入有效标志。
 * @param[in] size : 旧 APP 明文字节数。
 * @param[in] crc  : 旧 APP 完整明文 CRC。
 * @return 操作结果。
 * @retval 0 备份和记录已确认，有效标志提交成功。
 * @retval -1 旧备份验证失败。
 * @retval -2 备份记录写入或回读验证失败。
 * @retval -3 有效标志写入或回读验证失败。
 */
static int Bootloader_SaveBackup(uint32_t size, uint32_t crc)
{
    uint8_t record[BOOT_ROLLBACK_RECORD_SIZE] = {0U}; /* 持久备份记录。 */
    uint8_t valid = BOOT_ROLLBACK_VALID; /* 备份有效标志。 */
    uint32_t record_crc = 0U; /* 备份记录 CRC。 */
    if (0 != Bootloader_VerifyBackup(size, crc))
    {
        return -1;
    }
    record[1] = BOOT_ROLLBACK_VERSION;
    Bootloader_EncodeU32(&record[4], size);
    Bootloader_EncodeU32(&record[8], crc);
    record_crc = Bootloader_UpdateCRC32(0xFFFFFFFFU, &record[1], 11U) ^
                 0xFFFFFFFFU;
    Bootloader_EncodeU32(&record[12], record_crc);
    if (0 != Bootloader_WriteVerified(BOOT_ROLLBACK_RECORD_ADDR,
                                       record, sizeof(record)))
    {
        return -2;
    }
    return (0 == Bootloader_WriteVerified(BOOT_ROLLBACK_RECORD_ADDR,
                                           &valid, sizeof(valid))) ? 0 : -3;
}

/**
 * @brief 读取持久记录并校验 A 区完整旧备份。
 * @param[out] p_size : 旧 APP 长度输出，不得为空。
 * @param[out] p_crc  : 旧 APP CRC 输出，不得为空。
 * @return 操作结果。
 * @retval 0 持久记录及 A 区旧备份有效。
 * @retval -1 参数无效或 EEPROM 读取失败。
 * @retval -2 记录标志、版本、保留字段或 CRC 无效。
 * @retval -3 A 区旧备份验证失败。
 */
static int Bootloader_LoadBackup(uint32_t *p_size, uint32_t *p_crc)
{
    uint8_t record[BOOT_ROLLBACK_RECORD_SIZE] = {0U}; /* 持久备份记录。 */
    uint32_t record_crc = 0U; /* 备份记录 CRC。 */
    if ((NULL == p_size) || (NULL == p_crc) ||
        (0 != AT24Cxx_ReadBytes(record, BOOT_ROLLBACK_RECORD_ADDR,
                                sizeof(record))))
    {
        return -1;
    }
    record_crc = Bootloader_UpdateCRC32(0xFFFFFFFFU, &record[1], 11U) ^
                 0xFFFFFFFFU;
    if ((BOOT_ROLLBACK_VALID != record[0]) ||
        (BOOT_ROLLBACK_VERSION != record[1]) ||
        (0U != record[2]) || (0U != record[3]) ||
        (record_crc != Bootloader_DecodeU32(&record[12])))
    {
        return -2;
    }
    *p_size = Bootloader_DecodeU32(&record[4]);
    *p_crc = Bootloader_DecodeU32(&record[8]);
    return (0 == Bootloader_VerifyBackup(*p_size, *p_crc)) ? 0 : -3;
}

/**
 * @brief 保存正常启动状态后请求复位启动当前 APP。
 * @return 无；启动受限或入口无效时返回，成功时复位。
 */
static void Bootloader_RequestCurrentApp(void)
{
    if ((0U != g_app_launch_blocked) ||
        (0 != Bootloader_CheckAppEntry(
                   (const uint8_t *)APP_RUN_BASE_ADDR, APP_RUN_SIZE)))
    {
        return;
    }
    if (0 != Bootloader_SaveState(APP_STATE_NORMAL_BOOT))
    {
        g_app_launch_blocked = 1U;
        return;
    }
    g_app_jump_request = APP_JUMP_REQUEST_MAGIC;
    Bootloader_SoftwareReset();
}

/**
 * @brief 检查内部 APP 入口并交接控制权。
 * @return 无；启动受限或入口无效时返回。
 * @note 可在外设初始化前调用；APP 负责初始化并重新使能中断。
 */
void Bootloader_JumpToApp(void)
{
    if (0U != g_app_launch_blocked)
    {
        return;
    }

    /* APP 向量表中的初始栈顶地址。 */
    uint32_t app_sp = *(__IO uint32_t *)APP_RUN_BASE_ADDR;
    uint32_t app_entry_addr = 0U; /* APP 向量表中的复位入口地址。 */
    app_entry_t pf_app_entry = NULL; /* APP 复位入口函数指针。 */

    if (0 != Bootloader_CheckAppEntry(
                  (const uint8_t *)APP_RUN_BASE_ADDR, APP_RUN_SIZE))
    {
        /* 提前跳转时日志尚未初始化，失败后交由调用者处理。 */
        return;
    }
    app_entry_addr = *(__IO uint32_t *)(APP_RUN_BASE_ADDR + 4U);
    pf_app_entry = (app_entry_t)app_entry_addr;

    /* 关闭中断后交接；APP 负责初始化外设并重新使能中断。 */
    __disable_irq();
    SCB->VTOR = APP_RUN_BASE_ADDR;
    RCC_DeInit();
    __set_MSP(app_sp);
    pf_app_entry();
}

/**
 * @brief 将 A 区密文解密为 B 区有效 APP 明文。
 * @param[in] firmware_size : 完整加密包字节数。
 * @retval 0 解密、写入和入口检查完成。
 * @retval -1 长度、包格式或 A 区读取无效。
 * @retval -2 B 区写入、刷新或长度检查失败。
 * @retval -3 解密后的 APP 入口无效。
 * @note 密钥使用只读配置，每个升级包解密前初始化 CBC 工作 IV。
 * @note CBC 工作 IV 在文件内逐块更新，仅写入有效明文。
 */
static int8_t Bootloader_DecryptAToB(int32_t firmware_size)
{
    static uint8_t backup_read_buffer[4096] = {0U}; /* 每次读取一个外部扇区。 */
    /* 当前待解密数据块的源地址。 */
    const uint8_t *p_backup_flash_source = NULL;
    uint16_t backup_read_length = 0U; /* A 区本次读取长度。 */
    uint32_t backup_read_index = 16U; /* 缓冲区解密位置。 */
    uint8_t decrypted_block[16] = {0U}; /* 当前 AES 解密块。 */
    uint32_t decrypted_app_size = 0U; /* 文件头记录的有效 APP 明文长度。 */
    uint32_t block_count = 0U; /* 不含文件头的块数。 */
    uint32_t bytes_written = 0U; /* 本次已搬运的有效字节数。 */
    uint8_t iv[16] = {0U}; /* CBC 工作 IV，随本文件的逐块解密更新。 */

    /* 每个新升级包仅初始化一次，后续数据块延续当前 IV。 */
    memcpy(iv, g_app_aes_initial_iv, sizeof(iv));
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
    if ((firmware_size <= 16) || ((uint32_t)firmware_size > APP_BACKUP_SIZE) ||
        (0U != ((uint32_t)firmware_size % 16U)) ||
        (W25QxxHandler_GetWrittenSize(W25QXX_HANDLER_REGION_A) !=
         (uint32_t)firmware_size))
    {
        return -1;
    }

    /* 密文头部和后续数据始终从 A 区读取。 */
    W25QxxHandler_ResetReadPosition(W25QXX_HANDLER_REGION_A);
    if ((W25QXX_HANDLER_READ_OK !=
         W25QxxHandler_ReadData(W25QXX_HANDLER_REGION_A, backup_read_buffer,
                               &backup_read_length)) ||
        (backup_read_length < 16U))
    {
        return -1;
    }
    memcpy(decrypted_block, backup_read_buffer, 16U);
    AES_Decrypt256(iv, decrypted_block, g_app_aes_key);
    decrypted_app_size = (uint32_t)decrypted_block[12] |
                        ((uint32_t)decrypted_block[13] << 8U) |
                        ((uint32_t)decrypted_block[14] << 16U) |
                        ((uint32_t)decrypted_block[15] << 24U);
    if ((decrypted_app_size < 8U) || (decrypted_app_size > APP_RUN_SIZE) ||
        (decrypted_app_size > W25QXX_HANDLER_REGION_SIZE))
    {
        return -1;
    }
    block_count = (decrypted_app_size + 15U) / 16U;
    if ((uint32_t)firmware_size != (16U + block_count * 16U))
    {
        return -1;
    }
    elog_i(TAG, "Decrypt A to B: APP size=%lu.\r\n",
           (unsigned long)decrypted_app_size);

    for (uint16_t block_index = 0U; block_index < block_count; block_index++)
    {
        /* 当前解密块需要保存的有效字节数。 */
        uint32_t bytes_in_block = decrypted_app_size - bytes_written;
        if (bytes_in_block > 16U)
        {
            bytes_in_block = 16U;
        }
        if (backup_read_index == backup_read_length)
        {
            if (W25QXX_HANDLER_READ_OK !=
                W25QxxHandler_ReadData(W25QXX_HANDLER_REGION_A,
                                      backup_read_buffer,
                                      &backup_read_length))
            {
                W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
                return -1;
            }
            backup_read_index = 0U;
        }
        if ((backup_read_length < backup_read_index) ||
            ((backup_read_length - backup_read_index) < 16U))
        {
            W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
            return -1;
        }
        p_backup_flash_source = backup_read_buffer + backup_read_index;
        memcpy(decrypted_block, p_backup_flash_source, 16U);
        backup_read_index += 16U;
        AES_Decrypt256(iv, decrypted_block, g_app_aes_key);
        if (W25QXX_HANDLER_OK !=
            W25QxxHandler_WriteData(W25QXX_HANDLER_REGION_B,
                                   decrypted_block, bytes_in_block))
        {
            W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
            return -2;
        }
        bytes_written += bytes_in_block;
    }

    /* 全部解密完成后统一刷新，仅记录有效明文，不保留 AES 填充。 */
    if ((W25QXX_HANDLER_OK != W25QxxHandler_Flush(W25QXX_HANDLER_REGION_B)) ||
        (W25QxxHandler_GetWrittenSize(W25QXX_HANDLER_REGION_B) !=
         decrypted_app_size))
    {
        W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
        return -2;
    }
    W25QxxHandler_ResetReadPosition(W25QXX_HANDLER_REGION_B);
    if ((W25QXX_HANDLER_READ_OK !=
         W25QxxHandler_ReadData(W25QXX_HANDLER_REGION_B, backup_read_buffer,
                               &backup_read_length)) ||
        (0 != Bootloader_CheckAppEntry(backup_read_buffer,
                                       decrypted_app_size)))
    {
        W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_B);
        return -3;
    }
    W25QxxHandler_ResetReadPosition(W25QXX_HANDLER_REGION_B);
    return 0;
}

/**
 * @brief 按有效长度将内部旧 APP 备份到 A 区。
 * @param[in] app_size : EEPROM 记录的旧 APP 明文字节数。
 * @retval 0 备份完成。
 * @retval -1 旧 APP 长度或入口无效。
 * @retval -2 A 区写入或刷新失败；管理长度已作废。
 */
static int8_t Bootloader_BackupAppToA(int32_t app_size)
{
    uint32_t flash_source_addr = APP_RUN_BASE_ADDR; /* 内部旧 APP 读取地址。 */

    if ((app_size < 8) || ((uint32_t)app_size > APP_RUN_SIZE) ||
        ((uint32_t)app_size > W25QXX_HANDLER_REGION_SIZE) ||
        (0 != Bootloader_CheckAppEntry((const uint8_t *)flash_source_addr,
                                       (uint32_t)app_size)))
    {
        return -1;
    }
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);
    if ((W25QXX_HANDLER_OK !=
         W25QxxHandler_WriteData(W25QXX_HANDLER_REGION_A,
                                (uint8_t *)flash_source_addr,
                                (uint32_t)app_size)) ||
        (W25QXX_HANDLER_OK != W25QxxHandler_Flush(W25QXX_HANDLER_REGION_A)))
    {
        W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);
        return -2;
    }
    return 0;
}

/**
 * @brief 校验源区入口后，将 APP 明文安装到内部运行区。
 * @param[in] block_index : W25QXX_HANDLER_REGION_A 或 B。
 * @retval 0 安装及入口检查完成。
 * @retval -1 长度或后续读取无效。
 * @retval -2 擦除、写入或回读检查失败。
 * @retval -3 首段读取或源、目标入口检查失败。
 * @note 逐字写入并回读，尾部不足四字节时用 0xFF 补齐。
 */
static int8_t Bootloader_InstallFromRegion(uint8_t block_index)
{
    uint32_t flash_destination = APP_RUN_BASE_ADDR; /* 内部 APP 写入地址。 */
    uint32_t flash_size =
        W25QxxHandler_GetWrittenSize(block_index); /* 源区有效长度。 */
    uint32_t bytes_written = 0U; /* 本次已搬运的有效字节数。 */
    uint16_t read_length = 0U; /* 当前读取段的有效字节数。 */

    if ((flash_size < 8U) || (flash_size > APP_RUN_SIZE) ||
        (flash_size > W25QXX_HANDLER_REGION_SIZE))
    {
        return -1;
    }

    /* 先读并检查入口，确认源数据有效后才擦除内部 Flash。 */
    W25QxxHandler_ResetReadPosition(block_index);
    if ((W25QXX_HANDLER_READ_OK !=
         W25QxxHandler_ReadData(block_index, g_flash_read_buffer,
                               &read_length)) ||
        (0 != Bootloader_CheckAppEntry(g_flash_read_buffer, flash_size)))
    {
        return -3;
    }
    if (0 != Flash_Erase(flash_destination, flash_size))
    {
        return -2;
    }

    for (;;)
    {
        if ((0U == read_length) ||
            (read_length > flash_size - bytes_written))
        {
            return -1;
        }
        for (uint16_t write_times = 0U; write_times < read_length;
             write_times += 4U)
        {
            uint32_t word = 0xFFFFFFFFU; /* 待写入的四字节数据。 */
            /* 尾部不足一个字时用 0xFF 补齐后写入。 */
            for (uint8_t byte_index = 0U;
                 (byte_index < 4U) &&
                 ((write_times + byte_index) < read_length);
                 byte_index++)
            {
                uint32_t shift = byte_index * 8U; /* 字节位移。 */
                word = (word & ~(0xFFU << shift)) |
                       ((uint32_t)g_flash_read_buffer[write_times + byte_index]
                        << shift);
            }
            if (0 != Flash_Write(flash_destination, word))
            {
                return -2;
            }
            if (*(volatile uint32_t *)flash_destination != word)
            {
                return -2;
            }
            flash_destination += 4U;
        }
        bytes_written += read_length;
        if (bytes_written == flash_size)
        {
            break;
        }
        if (W25QXX_HANDLER_READ_OK !=
            W25QxxHandler_ReadData(block_index, g_flash_read_buffer,
                                  &read_length))
        {
            return -1;
        }
    }
    return (0 == Bootloader_CheckAppEntry(
                      (const uint8_t *)APP_RUN_BASE_ADDR, flash_size))
               ? 0
               : -3;
}

/**
 * @brief 将 B 区新 APP 安装到内部运行区。
 * @retval 0 安装完成。
 * @retval -1 长度或后续读取无效。
 * @retval -2 擦除、写入或回读检查失败。
 * @retval -3 首段读取或入口检查失败。
 */
static int8_t Bootloader_InstallFromB(void)
{
    return Bootloader_InstallFromRegion(W25QXX_HANDLER_REGION_B);
}

/**
 * @brief 将 A 区旧 APP 备份恢复到内部运行区。
 * @retval 0 恢复完成。
 * @retval -1 长度或后续读取无效。
 * @retval -2 擦除、写入或回读检查失败。
 * @retval -3 首段读取或入口检查失败。
 */
static int8_t Bootloader_RestoreFromA(void)
{
    return Bootloader_InstallFromRegion(W25QXX_HANDLER_REGION_A);
}

/**
 * @brief 保存并回读确认当前 APP 明文长度。
 * @param[in] app_size : 已安装 APP 的实际明文字节数。
 * @return 操作结果。
 * @retval 0 APP 长度写入且回读一致。
 * @retval -1 写入失败。
 * @retval -2 读取失败。
 * @retval -3 回读长度不一致。
 * @note 写入 EEPROM 0x05～0x08 后等待 10 毫秒再回读。
 */
static int Bootloader_SaveAppSize(uint32_t app_size)
{
    uint32_t read_app_size = 0U; /* EEPROM 回读的 APP 明文长度。 */

    if (0 != AT24Cxx_WriteBytes((uint8_t *)&app_size, 0x05U, sizeof(app_size)))
    {
        return -1;
    }
    Delay_Ms(10U);
    if (0 != AT24Cxx_ReadBytes((uint8_t *)&read_app_size, 0x05U,
                              sizeof(read_app_size)))
    {
        return -2;
    }
    return (read_app_size == app_size) ? 0 : -3;
}

/**
 * @brief 验证并恢复旧 APP，失败时保留下载入口。
 * @return 无；恢复成功后复位，失败时返回下载等待。
 * @note 恢复前后检查完整 CRC，并确认旧长度和正常启动状态已保存。
 */
static void Bootloader_RecoverApp(void)
{
    uint32_t old_size = 0U; /* 旧 APP 长度。 */
    uint32_t old_crc = 0U; /* 旧 APP 记录 CRC。 */
    uint32_t restored_crc = 0U; /* 恢复后内部 APP CRC。 */
    g_app_launch_blocked = 1U;
    if (0 == Bootloader_LoadBackup(&old_size, &old_crc))
    {
        if (0 == Bootloader_RestoreFromA())
        {
            restored_crc = Bootloader_UpdateCRC32(
                0xFFFFFFFFU, (const uint8_t *)APP_RUN_BASE_ADDR, old_size) ^
                0xFFFFFFFFU;
            if ((restored_crc == old_crc) &&
                (0 == Bootloader_SaveAppSize(old_size)) &&
                (0 == Bootloader_SaveState(APP_STATE_NORMAL_BOOT)))
            {
                elog_w(TAG, "Old APP restored and verified.\r\n");
                g_app_jump_request = APP_JUMP_REQUEST_MAGIC;
                Bootloader_SoftwareReset();
                return;
            }
        }
        elog_e(TAG, "Old APP recovery failed.\r\n");
    }
    else
    {
        elog_e(TAG, "No verified old APP backup.\r\n");
    }
    /* 无法恢复旧 APP 时，禁止启动当前 APP 并等待按键下载。 */
    if (0 != Bootloader_SaveState(APP_STATE_DOWNLOAD_REQUIRED))
    {
        elog_e(TAG, "Failed to save download-only state.\r\n");
    }
    elog_e(TAG, "Stay in Bootloader. Press key to download.\r\n");
}

/**
 * @brief 保存可靠旧备份，再安装新 APP 并准备首次试运行。
 * @return 无；安装或恢复成功后复位，失败时返回下载等待。
 * @note 旧长度读取失败时中止；启动受限时不备份当前镜像。
 */
static void Bootloader_InstallApp(void)
{
    uint32_t app_size = 0U; /* 旧 APP 长度。 */
    uint32_t app_crc = 0U; /* 旧 APP CRC。 */
    int8_t install_state = 0; /* 安装结果。 */
    if (0 != Bootloader_InvalidateBackup())
    {
        elog_e(TAG, "Cannot invalidate stale backup record.\r\n");
        Bootloader_RequestCurrentApp();
        return;
    }
    /* 旧长度读取失败时保留当前 APP，不进入擦写流程。 */
    if ((0U == g_app_launch_blocked) &&
        (0 != AT24Cxx_ReadBytes((uint8_t *)&app_size, 0x05U,
                                sizeof(app_size))))
    {
        elog_e(TAG, "Failed to read current APP size.\r\n");
        Bootloader_RequestCurrentApp();
        return;
    }
    if ((0U == g_app_launch_blocked) &&
        (app_size >= 8U) && (app_size <= APP_RUN_SIZE) &&
        (app_size <= W25QXX_HANDLER_REGION_SIZE) &&
        (0 == Bootloader_CheckAppEntry(
                   (const uint8_t *)APP_RUN_BASE_ADDR, app_size)))
    {
        app_crc = Bootloader_UpdateCRC32(
            0xFFFFFFFFU, (const uint8_t *)APP_RUN_BASE_ADDR, app_size) ^
            0xFFFFFFFFU;
        install_state = Bootloader_BackupAppToA((int32_t)app_size);
        if ((0 != install_state) ||
            (0 != Bootloader_SaveBackup(app_size, app_crc)))
        {
            elog_e(TAG, "Old APP backup/record commit failed.\r\n");
            Bootloader_RequestCurrentApp();
            return;
        }
    }
    /* 擦写前预置升级失败状态，安装中断复位后尝试恢复旧 APP。 */
    if (0 != Bootloader_SaveState(APP_STATE_UPDATE_FAILED))
    {
        elog_e(TAG, "Cannot commit unconfirmed state.\r\n");
        Bootloader_RequestCurrentApp();
        return;
    }
    g_app_launch_blocked = 1U;
    install_state = Bootloader_InstallFromB();
    if ((0 == install_state) &&
        (0 == Bootloader_SaveAppSize(
            W25QxxHandler_GetWrittenSize(W25QXX_HANDLER_REGION_B))) &&
        (0 == Bootloader_SaveState(APP_STATE_FIRST_BOOT_PENDING)))
    {
        elog_i(TAG, "APP installed. Start first check.\r\n");
        Bootloader_SoftwareReset();
        return;
    }
    elog_e(TAG, "APP installation/metadata commit failed.\r\n");
    Bootloader_RecoverApp();
}

/**
 * @brief 解密 A 区密文，再备份旧 APP 并安装新 APP。
 * @param[in] firmware_size : A 区完整加密包字节数。
 * @return 无；成功时复位，失败时尝试请求启动当前 APP 或返回。
 */
static void Bootloader_UpdateApp(int32_t firmware_size)
{
    /* 解密结果。 */
    int8_t install_state = Bootloader_DecryptAToB(firmware_size);

    /* 解密完成后才允许旧 APP 备份覆盖 A 区密文，并安装新 APP。 */
    if (0 != install_state)
    {
        elog_e(TAG, "Download/decrypt failed: size=%ld, result=%d.\r\n",
               (long)firmware_size, (int)install_state);
        Bootloader_RequestCurrentApp();
        return;
    }
    Bootloader_InstallApp();
}

/**
 * @brief 接收加密包并执行解密、备份和安装。
 * @return 无；成功时复位，失败时尝试请求启动当前 APP 或返回。
 * @note 使用静态缓冲区接收 YMODEM 文件到 A 区。
 */
void Bootloader_DownloadApp(void)
{
    /* 接收包会覆盖 A 区，必须先确认旧记录已经作废。 */
    if (0 != Bootloader_InvalidateBackup())
    {
        elog_e(TAG, "Cannot invalidate backup before download.\r\n");
        return;
    }

    /* 接收的加密包长度，失败时返回负值。 */
    int32_t firmware_size = Ymodem_Receive(g_ymodem_receive_buffer);

    Bootloader_UpdateApp(firmware_size);
}

/**
 * @brief 按 EEPROM 状态选择启动、安装或旧备份恢复。
 * @return 无；成功时复位或跳转，其他情况返回下载等待。
 * @note 调用前由 main() 完成 IWDG_Init()，仅首次试运行分支启动计时。
 * @note 首次试运行使用约 6 秒看门狗；复位后仍为升级失败状态则回滚。
 * @note 恢复失败或无有效旧备份时，禁止启动当前 APP 并等待下载。
 */
void Bootloader_ProcessState(void)
{
    /* EEPROM 保存的升级状态。 */
    uint8_t flag_ota_state = APP_STATE_NORMAL_BOOT;
    uint32_t firmware_size = 0U; /* EEPROM 保存的加密包有效长度。 */

    if (0 != AT24Cxx_ReadBytes(&flag_ota_state, 0x00U, 1U))
    {
        elog_e(TAG, "Failed to read OTA state from EEPROM.\r\n");
        g_app_launch_blocked = 1U;
        return;
    }

    elog_i(TAG, "Boot state=0x%02X\r\n", flag_ota_state);
    switch (flag_ota_state)
    {
        case APP_STATE_NORMAL_BOOT:
            if (KEY_PRESSED == Key_Scan())
            {
                Bootloader_DownloadApp();
            }
            else
            {
                if (0 == Bootloader_CheckAppEntry(
                              (const uint8_t *)APP_RUN_BASE_ADDR, APP_RUN_SIZE))
                {
                    /* 保留请求并复位，下次在外设初始化前跳转。 */
                    g_app_jump_request = APP_JUMP_REQUEST_MAGIC;
                    Bootloader_SoftwareReset();
                }
                else
                {
                    elog_e(TAG, "No valid APP.\r\n");
                }
            }
            break;

        case APP_STATE_DOWNLOAD_INCOMPLETE:
            /* 升级包未完整提交；优先启动当前 APP，否则重新下载。 */
            elog_e(TAG, "App download failed.\r\n");
            Bootloader_RequestCurrentApp();
            elog_w(TAG, "No valid APP. Download a new APP.\r\n");
            Bootloader_DownloadApp();
            break;

        case APP_STATE_INSTALL_PENDING:
            /* 升级包下载完成，恢复 A 区长度后执行解密和安装。 */
            if (0 != AT24Cxx_ReadBytes((uint8_t *)&firmware_size, 0x01U,
                                       sizeof(firmware_size)))
            {
                elog_e(TAG, "Failed to read pending firmware size.\r\n");
                Bootloader_RequestCurrentApp();
                break;
            }
            if ((firmware_size <= 16U) || (firmware_size > APP_BACKUP_SIZE) ||
                (0U != (firmware_size % 16U)))
            {
                elog_e(TAG, "Invalid pending firmware size: %lu.\r\n",
                       (unsigned long)firmware_size);
                Bootloader_RequestCurrentApp();
                break;
            }
            W25QxxHandler_RestoreRegion(W25QXX_HANDLER_REGION_A, firmware_size);
            Bootloader_UpdateApp((int32_t)firmware_size);
            break;

        case APP_STATE_FIRST_BOOT_PENDING:
            /* 试运行前预置升级失败状态，APP 确认成功后改为正常启动。 */
            if (0 != Bootloader_SaveState(APP_STATE_UPDATE_FAILED))
            {
                elog_e(TAG, "Cannot commit first-check state.\r\n");
                Bootloader_RecoverApp();
                break;
            }
            g_app_launch_blocked = 0U;
            /* 启动约 6 秒看门狗，再首次试运行新 APP。 */
            IWDG_Start(6000U);
            Bootloader_JumpToApp();
            /* 入口无效或 APP 返回，复位后按升级失败状态尝试恢复旧 APP。 */
            Bootloader_SoftwareReset();
            break;

        case APP_STATE_UPDATE_FAILED:
            /* 新 APP 升级失败，尝试恢复 A 区旧 APP 备份。 */
            Bootloader_RecoverApp();
            break;

        case APP_STATE_DOWNLOAD_REQUIRED:
            /* 禁止启动当前 APP，交由 main() 等待按键下载固件。 */
            g_app_launch_blocked = 1U;
            elog_e(TAG, "APP unconfirmed. Waiting for download.\r\n");
            break;

        default:
            g_app_launch_blocked = 1U;
            elog_e(TAG, "Invalid OTA state: 0x%02X.\r\n",
                   (unsigned int)flag_ota_state);
            break;
    }
}
/************************* Function Definitions *************************/
