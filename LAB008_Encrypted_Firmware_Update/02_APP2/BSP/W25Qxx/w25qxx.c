/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file w25qxx.c
 *
 * @par dependencies
 * - w25qxx.h
 * - stddef.h
 * - main.h
 * - spi.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements W25Q flash identification, access, and error handling.
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

/****************************** Includes ******************************/
#include "w25qxx.h"  /* BSP 层：本模块公共接口 */

#include <stddef.h>  /* C 标准库：空指针定义 */

#include "main.h"  /* Core/HAL 层：片选引脚、HAL 时基与 SPI */
#include "spi.h"
/****************************** Includes ******************************/

/****************************** Defines ******************************/
/* 容量编码与单元大小。 */
#define W25QXX_MIN_DEVICE_ID        0x13U
#define W25QXX_MAX_DEVICE_ID        0x17U
#define W25QXX_MIN_CAPACITY_BYTES   0x100000U
#define W25QXX_PAGE_SIZE            256U
#define W25QXX_SECTOR_SIZE          4096U
#define W25QXX_BLOCK_SIZE           65536U

/* 超时阈值，单位为毫秒。 */
#define W25QXX_TIMEOUT_MS           1000U
#define W25QXX_SECTOR_TIMEOUT_MS    3000U
#define W25QXX_CHIP_TIMEOUT_MS      250000U

/* 器件命令。 */
#define W25QXX_RESET_ENABLE_CMD     0x66U
#define W25QXX_RESET_MEMORY_CMD     0x99U
#define W25QXX_READ_STATUS_CMD      0x05U
#define W25QXX_WRITE_ENABLE_CMD     0x06U
#define W25QXX_READ_ID_CMD          0x90U
#define W25QXX_READ_CMD             0x03U
#define W25QXX_PAGE_PROGRAM_CMD     0x02U
#define W25QXX_SECTOR_ERASE_CMD     0x20U
#define W25QXX_CHIP_ERASE_CMD       0xC7U

/* 状态标志与占位字节。 */
#define W25QXX_BUSY_MASK            0x01U
#define W25QXX_WRITE_ENABLE_MASK    0x02U
#define W25QXX_DUMMY_BYTE           0xFFU

/* 软件片选，低电平有效。 */
#define W25QXX_CS_LOW() \
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)
#define W25QXX_CS_HIGH() \
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)
/****************************** Defines ******************************/

/************************ Variable Definitions ************************/
/* 本模块保存的器件参数。 */
static w25qxx_parameters_t g_w25qxx_parameters = {0};
/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/
/**
 * @brief 分两次片选发送复位命令。
 * @param 无。
 * @return 命令发送结果。
 * @retval W25QXX_OK 两条命令发送成功。
 * @retval W25QXX_ERROR SPI 传输失败。
 * @note 命令各占一次片选，稳定延时由 Init 执行。
 */
static w25qxx_status_t W25Qxx_Reset(void)
{
    /* 复位命令与下标。 */
    uint8_t command[2] = {
        W25QXX_RESET_ENABLE_CMD, W25QXX_RESET_MEMORY_CMD
    };
    uint8_t command_index = 0U;

    /* 每次片选仅发送一条命令。 */
    for (command_index = 0U; command_index < 2U; command_index++)
    {
        W25QXX_CS_LOW();
        if (HAL_OK != HAL_SPI_Transmit(&hspi1, &command[command_index], 1U,
                                 W25QXX_TIMEOUT_MS))
        {
            W25QXX_CS_HIGH();
            return W25QXX_ERROR;
        }
        W25QXX_CS_HIGH();
    }

    return W25QXX_OK;
}

/**
 * @brief 读取状态寄存器 1。
 * @param[out] p_status_register 状态字节输出，不得为空。
 * @return 状态读取结果。
 * @retval W25QXX_OK 读取成功且器件空闲。
 * @retval W25QXX_BUSY 读取成功且器件忙。
 * @retval W25QXX_ERROR 空指针或 SPI 失败。
 * @note 成功时输出有效，读取失败不视为空闲。
 */
static w25qxx_status_t W25Qxx_GetStatus(uint8_t *p_status_register)
{
    uint8_t command = W25QXX_READ_STATUS_CMD; /* 状态读取命令 */

    if (NULL == p_status_register)
    {
        return W25QXX_ERROR;
    }
    *p_status_register = W25QXX_DUMMY_BYTE;

    /* 1. 读取状态，失败时释放片选。 */
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, &command, 1U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    if (HAL_OK != HAL_SPI_TransmitReceive(&hspi1, p_status_register,
                                            p_status_register, 1U,
                                            W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    W25QXX_CS_HIGH();

    /* 2. 读取成功后检查忙标志。 */
    if (0U != (*p_status_register & W25QXX_BUSY_MASK))
    {
        return W25QXX_BUSY;
    }
    return W25QXX_OK;
}

/**
 * @brief 等待空闲或返回异常。
 * @param[in] start_tick 等待起始毫秒计数。
 * @param[in] timeout_ms 超时阈值，单位为毫秒。
 * @param[in] poll_delay_ms 重试间隔，零表示不延时。
 * @param[out] p_status_register 最近一次状态字节。
 * @return 等待结果。
 * @retval W25QXX_OK 空闲，状态有效。
 * @retval W25QXX_ERROR 状态读取失败。
 * @retval W25QXX_TIMEOUT 忙等待超过阈值。
 * @note 依赖 HAL 毫秒时基，以无符号差值处理回绕。
 */
static w25qxx_status_t W25Qxx_WaitReady(uint32_t start_tick,
                                        uint32_t timeout_ms,
                                        uint32_t poll_delay_ms,
                                        uint8_t *p_status_register)
{
    w25qxx_status_t status = W25QXX_OK; /* 状态读取结果 */

    while (1)
    {
        status = W25Qxx_GetStatus(p_status_register);
        if (W25QXX_BUSY != status)
        {
            return status;
        }
        if ((HAL_GetTick() - start_tick) > timeout_ms)
        {
            return W25QXX_TIMEOUT;
        }
        if (0U != poll_delay_ms)
        {
            HAL_Delay(poll_delay_ms);
        }
    }
}

/**
 * @brief 检查非零长度访问区间。
 * @param[in] address 访问起始地址。
 * @param[in] size 访问字节数，须非零。
 * @return 区间检查结果。
 * @retval W25QXX_OK 区间有效。
 * @retval W25QXX_ERROR 未初始化或区间越界。
 * @note 以减法检查长度，避免加法溢出。
 */
static w25qxx_status_t W25Qxx_CheckRange(uint32_t address, uint32_t size)
{
    uint32_t capacity = g_w25qxx_parameters.flash_size; /* 已保存容量 */

    if ((0U == capacity) || (address >= capacity))
    {
        return W25QXX_ERROR;
    }
    if (size > (capacity - address))
    {
        return W25QXX_ERROR;
    }
    return W25QXX_OK;
}

/**
 * @brief 复位器件并保存容量参数。
 * @param 无。
 * @return 初始化结果。
 * @retval W25QXX_OK 初始化成功。
 * @retval W25QXX_ERROR SPI 失败或容量编码不支持。
 * @note 先初始化 SPI1 和 HAL 时基，保持 HAL 毫秒时基运行。
 * @note 复位后延时 5 ms；初始化失败时不得访问数据。
 */
w25qxx_status_t W25Qxx_Init(void)
{
    w25qxx_status_t status = W25QXX_OK; /* 复位和参数读取结果 */

    /* 1. 复位后等待器件稳定。 */
    status = W25Qxx_Reset();
    if (W25QXX_OK != status)
    {
        return status;
    }
    HAL_Delay(5U);

    /* 2. 保存器件参数。 */
    return W25Qxx_GetParameters(&g_w25qxx_parameters);
}

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
w25qxx_status_t W25Qxx_WriteEnable(void)
{
    uint8_t command = W25QXX_WRITE_ENABLE_CMD; /* 写使能命令 */
    uint8_t status_register = W25QXX_DUMMY_BYTE; /* 原始状态字节 */
    uint32_t start_tick = HAL_GetTick(); /* 本次等待起点 */
    w25qxx_status_t status = W25QXX_OK; /* 传输和忙等待结果 */

    /* 1. 发送写使能命令。 */
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, &command, 1U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    W25QXX_CS_HIGH();

    /* 2. 发送后等待空闲。 */
    status = W25Qxx_WaitReady(start_tick, W25QXX_TIMEOUT_MS, 1U,
                              &status_register);
    if (W25QXX_OK != status)
    {
        return status;
    }

    /* 3. 检查 WEL 标志。 */
    if (0U == (status_register & W25QXX_WRITE_ENABLE_MASK))
    {
        return W25QXX_ERROR;
    }
    return W25QXX_OK;
}

/**
 * @brief 读取制造商和器件标识。
 * @param[out] p_id 标识输出，不得为空。
 * @return 标识读取结果。
 * @retval W25QXX_OK 读取成功。
 * @retval W25QXX_ERROR 空指针或 SPI 失败。
 * @note 成功时更新输出：高字节为制造商，低字节为器件编码。
 * @note 先初始化 SPI1，保持 HAL 毫秒时基运行。
 */
w25qxx_status_t W25Qxx_ReadID(uint16_t *p_id)
{
    /* 读取命令与占位字节。 */
    uint8_t command[4] = {W25QXX_READ_ID_CMD, 0U, 0U, 0U};
    uint8_t id_bytes[2] = {W25QXX_DUMMY_BYTE, W25QXX_DUMMY_BYTE};

    if (NULL == p_id)
    {
        return W25QXX_ERROR;
    }

    /* 1. 发送命令并读取标识。 */
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, command, 4U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    if (HAL_OK != HAL_SPI_TransmitReceive(&hspi1, id_bytes,
                                            id_bytes, 2U,
                                            W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    W25QXX_CS_HIGH();

    /* 2. 成功后更新输出。 */
    *p_id = (uint16_t)(((uint16_t)id_bytes[0] << 8U) | id_bytes[1]);
    return W25QXX_OK;
}

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
    w25qxx_parameters_t *p_parameters)
{
    uint16_t id = 0U;       /* 制造商和器件标识 */
    uint8_t device_id = 0U; /* 低字节容量编码 */
    uint32_t size = 0U;     /* 计算后的总容量 */
    w25qxx_status_t status = W25QXX_OK; /* 标识读取结果 */

    if (NULL == p_parameters)
    {
        return W25QXX_ERROR;
    }

    /* 1. 填写固定尺寸。 */
    p_parameters->page_size = W25QXX_PAGE_SIZE;
    p_parameters->sector_size = W25QXX_SECTOR_SIZE;
    p_parameters->block_size = W25QXX_BLOCK_SIZE;

    /* 2. 读取并检查容量编码。 */
    status = W25Qxx_ReadID(&id);
    if (W25QXX_OK != status)
    {
        return status;
    }
    device_id = (uint8_t)(id & 0xFFU);
    if ((device_id < W25QXX_MIN_DEVICE_ID) ||
        (device_id > W25QXX_MAX_DEVICE_ID))
    {
        return W25QXX_ERROR;
    }

    /* 3. 通过整数移位计算容量。 */
    size = W25QXX_MIN_CAPACITY_BYTES <<
           (device_id - W25QXX_MIN_DEVICE_ID);
    p_parameters->flash_id = id;
    p_parameters->flash_size = size;
    p_parameters->sector_count = size / W25QXX_SECTOR_SIZE;
    p_parameters->block_count = size / W25QXX_BLOCK_SIZE;

    return W25QXX_OK;
}

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
                            uint32_t size)
{
    uint8_t command[4] = {0U}; /* 读取命令和 24 位地址 */
    uint32_t remaining_size = size; /* 剩余接收字节数 */
    uint16_t transfer_size = 0U; /* 单次 SPI 接收字节数 */

    /* 1. 检查长度、指针和容量。 */
    if (0U == size)
    {
        return W25QXX_OK;
    }
    if ((NULL == p_data) ||
        (W25QXX_OK != W25Qxx_CheckRange(read_address, size)))
    {
        return W25QXX_ERROR;
    }

    /* 2. 发送命令和地址。 */
    command[0] = W25QXX_READ_CMD;
    command[1] = (uint8_t)(read_address >> 16U);
    command[2] = (uint8_t)(read_address >> 8U);
    command[3] = (uint8_t)read_address;
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, command, 4U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }

    /* 3. 保持片选，按 SPI 长度上限分段读取。 */
    while (remaining_size > 0U)
    {
        transfer_size = (remaining_size > UINT16_MAX)
                        ? UINT16_MAX : (uint16_t)remaining_size;
        if (HAL_OK != HAL_SPI_TransmitReceive(&hspi1, p_data,
                                            p_data, transfer_size,
                                            W25QXX_TIMEOUT_MS))
        {
            W25QXX_CS_HIGH();
            return W25QXX_ERROR;
        }
        p_data += transfer_size;
        remaining_size -= transfer_size;
    }
    W25QXX_CS_HIGH();

    return W25QXX_OK;
}

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
                             uint32_t size)
{
    uint8_t command[4] = {0U}; /* 页编程命令和 24 位地址 */
    uint8_t status_register = W25QXX_DUMMY_BYTE; /* 原始状态字节 */
    uint32_t end_address = 0U;     /* 写入区间结束地址 */
    uint32_t current_size = 0U;    /* 当前页写入长度 */
    uint32_t current_address = 0U; /* 当前页写入地址 */
    uint32_t start_tick = 0U;      /* 当前页忙等待起点 */
    w25qxx_status_t status = W25QXX_OK; /* 写使能和忙等待结果 */

    /* 1. 检查长度、指针和容量。 */
    if (0U == size)
    {
        return W25QXX_OK;
    }
    if ((NULL == p_data) ||
        (W25QXX_OK != W25Qxx_CheckRange(write_address, size)))
    {
        return W25QXX_ERROR;
    }

    /* 2. 逐页累加，计算首页剩余空间。 */
    while (current_address <= write_address)
    {
        current_address += W25QXX_PAGE_SIZE;
    }
    current_size = current_address - write_address;
    if (current_size > size)
    {
        current_size = size;
    }
    current_address = write_address;
    end_address = write_address + size;

    /* 3. 逐页编程并等待完成。 */
    do
    {
        /* 3.1 组装命令并确认写使能。 */
        command[0] = W25QXX_PAGE_PROGRAM_CMD;
        command[1] = (uint8_t)(current_address >> 16U);
        command[2] = (uint8_t)(current_address >> 8U);
        command[3] = (uint8_t)current_address;
        status = W25Qxx_WriteEnable();
        if (W25QXX_OK != status)
        {
            return status;
        }

        /* 3.2 发送命令和数据，失败时释放片选。 */
        W25QXX_CS_LOW();
        if (HAL_OK != HAL_SPI_Transmit(&hspi1, command, 4U, W25QXX_TIMEOUT_MS))
        {
            W25QXX_CS_HIGH();
            return W25QXX_ERROR;
        }
        if (HAL_OK != HAL_SPI_Transmit(&hspi1, p_data, (uint16_t)current_size,
                                 W25QXX_TIMEOUT_MS))
        {
            W25QXX_CS_HIGH();
            return W25QXX_ERROR;
        }
        W25QXX_CS_HIGH();

        /* 3.3 本页独立计时，等待完成。 */
        start_tick = HAL_GetTick();
        status = W25Qxx_WaitReady(start_tick, W25QXX_TIMEOUT_MS, 0U,
                                  &status_register);
        if (W25QXX_OK != status)
        {
            return status;
        }

        /* 3.4 更新下一页地址、指针和长度。 */
        current_address += current_size;
        p_data += current_size;
        current_size = ((current_address + W25QXX_PAGE_SIZE) > end_address)
                       ? (end_address - current_address) : W25QXX_PAGE_SIZE;
    } while (current_address < end_address);

    return W25QXX_OK;
}

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
w25qxx_status_t W25Qxx_EraseSector(uint32_t address)
{
    uint8_t command[4] = {0U}; /* 扇区擦除命令和 24 位地址 */
    uint8_t status_register = W25QXX_DUMMY_BYTE; /* 原始状态字节 */
    uint32_t start_tick = HAL_GetTick(); /* 起始毫秒计数 */
    w25qxx_status_t status = W25QXX_OK; /* 写使能和擦除等待结果 */

    /* 1. 检查地址并组装擦除命令。 */
    if (W25QXX_OK != W25Qxx_CheckRange(address, 1U))
    {
        return W25QXX_ERROR;
    }
    command[0] = W25QXX_SECTOR_ERASE_CMD;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;

    /* 2. 写使能后发送擦除命令。 */
    status = W25Qxx_WriteEnable();
    if (W25QXX_OK != status)
    {
        return status;
    }
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, command, 4U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    W25QXX_CS_HIGH();

    /* 3. 等待扇区擦除完成。 */
    return W25Qxx_WaitReady(start_tick, W25QXX_SECTOR_TIMEOUT_MS, 0U,
                            &status_register);
}

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
w25qxx_status_t W25Qxx_EraseChip(void)
{
    uint8_t command = W25QXX_CHIP_ERASE_CMD; /* 整片擦除命令 */
    uint8_t status_register = W25QXX_DUMMY_BYTE; /* 原始状态字节 */
    uint32_t start_tick = HAL_GetTick(); /* 起始毫秒计数 */
    w25qxx_status_t status = W25QXX_OK; /* 写使能和擦除等待结果 */

    /* 1. 写使能后发送整片擦除命令。 */
    status = W25Qxx_WriteEnable();
    if (W25QXX_OK != status)
    {
        return status;
    }
    W25QXX_CS_LOW();
    if (HAL_OK != HAL_SPI_Transmit(&hspi1, &command, 1U, W25QXX_TIMEOUT_MS))
    {
        W25QXX_CS_HIGH();
        return W25QXX_ERROR;
    }
    W25QXX_CS_HIGH();

    /* 2. 等待整片擦除完成。 */
    return W25Qxx_WaitReady(start_tick, W25QXX_CHIP_TIMEOUT_MS, 0U,
                            &status_register);
}
/************************ Function Definitions ************************/
