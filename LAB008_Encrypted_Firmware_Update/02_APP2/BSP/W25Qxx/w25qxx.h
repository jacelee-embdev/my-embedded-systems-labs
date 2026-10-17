/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file w25qxx.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares W25Q flash parameters and status-based APIs.
 *
 * Processing flow:
 *
 * Initialize SPI1 and the millisecond time base, then call W25Qxx_Init().
 * After successful initialization, use the read, write, and erase APIs.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __W25QXX_H__
#define __W25QXX_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

#ifdef __cplusplus
extern "C" {
#endif

/****************************** Types ******************************/
/* 驱动接口的执行状态。 */
typedef enum
{
    W25QXX_OK = 0,      /* 操作成功 */
    W25QXX_ERROR = 1,   /* 参数、通信或器件状态异常 */
    W25QXX_BUSY = 2,    /* 器件正在执行内部操作 */
    W25QXX_TIMEOUT = 3  /* 等待器件空闲超时 */
} w25qxx_status_t;

/* 器件容量与单元参数。 */
typedef struct
{
    uint32_t flash_id;      /* 制造商和器件标识 */
    uint32_t flash_size;    /* 总容量，单位为字节 */
    uint32_t block_count;   /* 64 KB 块数量 */
    uint32_t block_size;    /* 块大小，单位为字节 */
    uint32_t sector_count;  /* 4 KB 扇区数量 */
    uint32_t sector_size;   /* 扇区大小，单位为字节 */
    uint32_t page_size;     /* 编程页大小，单位为字节 */
} w25qxx_parameters_t;
/****************************** Types ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 复位器件并保存容量参数。
 * @param 无。
 * @return 初始化结果。
 * @retval W25QXX_OK 初始化成功。
 * @retval W25QXX_ERROR SPI 失败或容量编码不支持。
 * @note 先初始化 SPI1 和 HAL 时基，保持 HAL 毫秒时基运行。
 * @note 复位后延时 5 ms；初始化失败时不得访问数据。
 */
w25qxx_status_t W25Qxx_Init(void);

/**
 * @brief 写使能并检查 WEL 标志。
 * @param 无。
 * @return 写使能结果。
 * @retval W25QXX_OK 器件空闲且 WEL 置位。
 * @retval W25QXX_ERROR SPI 失败或 WEL 未置位。
 * @retval W25QXX_TIMEOUT 等待空闲超时。
 * @note 忙时每隔 1 ms 重试，超时阈值为 1000 ms。
 * @note 依赖 HAL 毫秒时基，不得在阻塞其执行的上下文中调用。
 */
w25qxx_status_t W25Qxx_WriteEnable(void);

/**
 * @brief 读取制造商和器件标识。
 * @param[out] p_id 标识输出，不得为空。
 * @return 标识读取结果。
 * @retval W25QXX_OK 读取成功。
 * @retval W25QXX_ERROR 空指针或 SPI 失败。
 * @note 成功时更新输出：高字节为制造商，低字节为器件编码。
 * @note 先初始化 SPI1，保持 HAL 毫秒时基运行。
 */
w25qxx_status_t W25Qxx_ReadID(uint16_t *p_id);

/**
 * @brief 读取器件参数。
 * @param[out] p_parameters 参数输出，不得为空。
 * @return 参数读取结果。
 * @retval W25QXX_OK 参数读取成功。
 * @retval W25QXX_ERROR 空指针、SPI 失败或容量编码不支持。
 * @note 支持编码 0x13 至 0x17，容量为 1 至 16 MiB。
 * @note 失败时可能已填固定尺寸；仅初始化时保存内部容量。
 * @note 先初始化 SPI1，保持 HAL 毫秒时基运行。
 */
w25qxx_status_t W25Qxx_GetParameters(
    w25qxx_parameters_t *p_parameters);

/**
 * @brief 从指定地址分段读取数据。
 * @param[in,out] p_data 收发缓冲区，非零长度时非空。
 * @param[in] read_address 读取起始地址。
 * @param[in] size 读取字节数。
 * @return 读取结果。
 * @retval W25QXX_OK 读取成功或长度为零。
 * @retval W25QXX_ERROR 参数无效、未初始化或传输失败。
 * @note 非零长度须先成功初始化，且访问区间不越界。
 * @note 缓冲区须已初始化，原内容用作 SPI 占位数据。
 * @note 零长度无通信；失败时缓冲区可能部分更新。
 * @note 依赖 HAL 毫秒时基，不得在阻塞其执行的上下文中调用。
 */
w25qxx_status_t W25Qxx_Read(uint8_t *p_data, uint32_t read_address,
                            uint32_t size);

/**
 * @brief 按页写入并等待编程完成。
 * @param[in] p_data 写入缓冲区，非零长度时非空。
 * @param[in] write_address 写入起始地址。
 * @param[in] size 写入字节数。
 * @return 写入结果。
 * @retval W25QXX_OK 写入成功或长度为零。
 * @retval W25QXX_ERROR 参数无效、未初始化、传输或写使能失败。
 * @retval W25QXX_TIMEOUT 写使能或页编程等待超时。
 * @note 非零长度须先成功初始化，且访问区间不越界。
 * @note 零长度无通信；不自动擦除，失败前可能已写入。
 * @note 每页释放片选后独立计时，忙等待阈值为 1000 ms。
 * @note 依赖 HAL 毫秒时基，不得在阻塞其执行的上下文中调用。
 */
w25qxx_status_t W25Qxx_Write(uint8_t *p_data, uint32_t write_address,
                             uint32_t size);

/**
 * @brief 擦除地址所在的 4 KB 扇区。
 * @param[in] address 扇区内地址，须小于容量。
 * @return 擦除结果。
 * @retval W25QXX_OK 擦除成功。
 * @retval W25QXX_ERROR 地址越界、未初始化、传输或写使能失败。
 * @retval W25QXX_TIMEOUT 写使能或扇区擦除等待超时。
 * @note 须先成功初始化；地址无需对齐，擦除整个扇区。
 * @note 从接口入口计时，擦除等待阈值为 3000 ms。
 * @note 依赖 HAL 毫秒时基，不得在阻塞其执行的上下文中调用。
 */
w25qxx_status_t W25Qxx_EraseSector(uint32_t address);

/**
 * @brief 擦除整个器件。
 * @param 无。
 * @return 擦除结果。
 * @retval W25QXX_OK 擦除成功。
 * @retval W25QXX_ERROR SPI 或写使能失败。
 * @retval W25QXX_TIMEOUT 写使能或整片擦除等待超时。
 * @note 先初始化 SPI1；操作会清除全部数据。
 * @note 从接口入口计时，擦除等待阈值为 250000 ms。
 * @note 依赖 HAL 毫秒时基，不得在阻塞其执行的上下文中调用。
 */
w25qxx_status_t W25Qxx_EraseChip(void);
/************************ Function Declarations ************************/

#ifdef __cplusplus
}
#endif

#endif /* __W25QXX_H__ */
