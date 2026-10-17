/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file at24cxx.c
 *
 * @par dependencies
 * - at24cxx.h
 * - stddef.h
 * - string.h
 * - debug.h
 * - iic.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides AT24C02 operations through the HAL software I2C driver.
 *
 * Processing flow:
 *
 * Initialize the bus with AT24Cxx_Init() before device access.
 * AT24Cxx_CheckDevice() sends the device write address and checks ACK.
 * Device detection uses the software I2C transfer APIs directly.
 * Write operations poll for completion before reporting success.
 * AT24Cxx_Test() checks ACK, writes fixed F1-F8 data to the last page,
 * then logs and compares the readback without a millisecond time base.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "at24cxx.h"  /* 用户 BSP 层：EEPROM 接口 */

#include <stddef.h>  /* C 标准库：空指针与内存比较 */
#include <string.h>

#include "debug.h"  /* 用户中间件层：日志封装接口 */

#include "iic.h"  /* 用户 BSP 层：软件 IIC 接口 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define TAG            "at24cxx"  /* 日志标签 */
#define AT24CXX_DEBUG  1          /* 调试日志开关：1 开启，0 关闭 */

/*
 * AT24C02：256 字节，32 页，每页 8 字节。
 * 地址选择位全零：写 0xA0，读 0xA1。
 * AT24C01/02 页长 8 字节，AT24C04/08A/16A 页长 16 字节。
 */
#define AT24CXX_DEVICE_ADDR       0xA0U  /* 设备写地址 */
#define AT24CXX_PAGE_SIZE         8U     /* 页长，字节 */
#define AT24CXX_SIZE              256U   /* 容量，字节 */
#define AT24CXX_ACK_RETRY_LIMIT   1000U  /* 轮询上限 */
#define AT24CXX_ERASE_VALUE       0xFFU  /* 擦除填充值 */

/****************************** Defines ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 初始化 AT24Cxx 使用的 IIC 总线。
 *
 * 调用 IIC_Init() 配置总线 GPIO 并发送停止信号。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 仅初始化总线；设备应答由 AT24Cxx_CheckDevice() 检测。
 * @note 检测、非零长度读写、擦除或测试前调用一次。
 */
void AT24Cxx_Init(void)
{
    IIC_Init();
}

/**
 * @brief 检测 AT24Cxx 设备应答。
 *
 * 处理步骤：
 * 1. 发送起始信号和预设设备写地址。
 * 2. 采样 ACK，发送停止信号。
 * 3. 将应答状态转换为检测结果。
 *
 * @param 无。
 *
 * @return 检测结果。
 * @retval 0 已应答。
 * @retval -1 未应答。
 * @note 须先调用 AT24Cxx_Init()；本函数不初始化总线。
 * @note 无论是否应答均发送停止信号，不读取芯片 ID。
 */
int AT24Cxx_CheckDevice(void)
{
    /* 设备地址的应答状态。 */
    iic_ack_status_t ack_status = IIC_ACK_NOT_RECEIVED;

    /* 1. 发送起始信号和设备写地址。 */
    IIC_Start();
    IIC_SendByte(AT24CXX_DEVICE_ADDR);

    /* 2. 采样应答并停止总线。 */
    ack_status = IIC_WaitAck();
    IIC_Stop();

    /* 3. 转换为模块检测结果：0 为应答，-1 为未应答。 */
    return (IIC_ACK_RECEIVED == ack_status) ? 0 : -1;
}

/**
 * @brief 连续读取数据。
 *
 * 处理步骤：
 * 1. 校验参数，零长度直接成功。
 * 2. 发送设备写地址和起始地址。
 * 3. 切换读方向，连续读取。
 * 4. 末字节发 NACK，随后停止。
 *
 * @param[out] p_read_buf   : 接收缓冲区，非零长度时须有效。
 * @param[in] start_address : 起始地址，0～255。
 * @param[in] byte_count    : 字节数，不得跨容量边界。
 *
 * @return 读取结果。
 * @retval 0 读取完成或长度为零。
 * @retval -1 参数无效。
 * @retval -2 设备未应答。
 * @note 零长度不访问总线，允许空指针，忽略地址。
 * @note 非零请求须先调用 AT24Cxx_Init()。
 */
int AT24Cxx_ReadBytes(
    uint8_t *p_read_buf,
    uint16_t start_address,
    uint16_t byte_count)
{
    uint16_t byte_index = 0U;  /* 读索引 */

    /* 零长度直接成功。 */
    if (0U == byte_count)
    {
        return 0;
    }

    /* 先校验地址，再计算剩余容量。 */
    if ((NULL == p_read_buf) ||
        (start_address >= AT24CXX_SIZE) ||
        (byte_count > (AT24CXX_SIZE - start_address)))
    {
        return -1;
    }

    /* 1. 发送起始信号。 */
    IIC_Start();

    /* 2. 发送写方向设备地址。 */
    IIC_SendByte(AT24CXX_DEVICE_ADDR | EEPROM_I2C_WR);

    /* 3. 等待设备应答。 */
    if (IIC_ACK_RECEIVED != IIC_WaitAck())
    {
        goto cmd_fail; /* 设备未应答。 */
    }

    /* 4. 发送单字节起始地址。 */
    IIC_SendByte((uint8_t)start_address);

    /* 5. 等待设备应答。 */
    if (IIC_ACK_RECEIVED != IIC_WaitAck())
    {
        goto cmd_fail; /* 设备未应答。 */
    }

    /* 6. 重发起始信号。 */
    IIC_Start();

    /* 7. 发送读方向的设备地址。 */
    IIC_SendByte(AT24CXX_DEVICE_ADDR | EEPROM_I2C_RD);

    /* 8. 等待设备应答。 */
    if (IIC_ACK_RECEIVED != IIC_WaitAck())
    {
        goto cmd_fail; /* 设备未应答。 */
    }

    /* 9. 连续读取数据。 */
    for (byte_index = 0U; byte_index < byte_count; byte_index++)
    {
        p_read_buf[byte_index] = IIC_ReadByte(); /* 读取一个字节。 */

        /* 末字节发 NACK，其余发 ACK。 */
        if (byte_index != byte_count - 1U)
        {
            IIC_SendAck(); /* 低电平应答。 */
        }
        else
        {
            IIC_SendNack(); /* 高电平非应答。 */
        }
    }

    /* 读取完成，停止总线。 */
    IIC_Stop();
    return 0;

cmd_fail: /* 失败时停止总线。 */

    IIC_Stop();
    return -2;
}

/**
 * @brief 按页写入，等待末页完成。
 *
 * 处理步骤：
 * 1. 校验参数，零长度直接成功。
 * 2. 首字节或页首重新寻址。
 * 3. 发送数据并检查应答。
 * 4. 停止总线，轮询末页完成。
 *
 * @param[in] p_write_buf  : 只读缓冲区，非零长度时须有效。
 * @param[in] start_address : 起始地址，0～255。
 * @param[in] byte_count   : 字节数，不得跨容量边界。
 *
 * @return 写入结果。
 * @retval 0 数据已发送且设备就绪，或长度为零。
 * @retval -1 参数无效。
 * @retval -2 设备未应答。
 * @retval -3 设备就绪轮询次数耗尽。
 * @note 零长度不访问总线，允许空指针，忽略地址。
 * @note 每轮最多轮询 1000 次，不依赖 SysTick。
 * @note 不作回读校验；失败可能已部分写入。
 * @note 非零请求须先调用 AT24Cxx_Init()。
 */
int AT24Cxx_WriteBytes(
    const uint8_t *p_write_buf,
    uint16_t start_address,
    uint16_t byte_count)
{
    uint16_t byte_index = 0U;       /* 写索引 */
    uint16_t ack_retry_count = 0U;  /* 应答重试数 */
    uint16_t current_address = 0U;  /* 写入地址 */
    int result = -2; /* 默认设备未应答；轮询耗尽时返回 -3。 */

    /* 零长度直接成功。 */
    if (0U == byte_count)
    {
        return 0;
    }

    /* 先校验地址，再计算剩余容量。 */
    if ((NULL == p_write_buf) ||
        (start_address >= AT24CXX_SIZE) ||
        (byte_count > (AT24CXX_SIZE - start_address)))
    {
        return -1;
    }

    /* 按 8 字节页边界分段。 */
    current_address = start_address;
    for (byte_index = 0U; byte_index < byte_count; byte_index++)
    {
        /* 首字节或页首重新寻址。 */
        if ((0U == byte_index) ||
            (0U == (current_address & (AT24CXX_PAGE_SIZE - 1U))))
        {
            /* 0. 发送停止信号。 */
            IIC_Stop();

            /* 轮询设备就绪。 */
            for (ack_retry_count = 0U;
                ack_retry_count < AT24CXX_ACK_RETRY_LIMIT;
                ack_retry_count++)
            {
                /* 1. 发送起始信号。 */
                IIC_Start();

                /* 2. 发送写方向的设备地址。 */
                IIC_SendByte(AT24CXX_DEVICE_ADDR | EEPROM_I2C_WR);

                /* 3. 收到应答后结束轮询。 */
                if (IIC_ACK_RECEIVED == IIC_WaitAck())
                {
                    break;
                }
            }
            if (AT24CXX_ACK_RETRY_LIMIT == ack_retry_count)
            {
                result = -3;
                goto cmd_fail; /* 应答重试次数已用尽。 */
            }

            /* 4. 发送单字节起始地址。 */
            IIC_SendByte((uint8_t)current_address);

            /* 5. 等待设备应答。 */
            if (IIC_ACK_RECEIVED != IIC_WaitAck())
            {
                goto cmd_fail; /* 设备未应答。 */
            }
        }

        /* 6. 发送当前数据字节。 */
        IIC_SendByte(p_write_buf[byte_index]);

        /* 7. 等待设备应答。 */
        if (IIC_ACK_RECEIVED != IIC_WaitAck())
        {
            goto cmd_fail; /* 设备未应答。 */
        }
        current_address++; /* 递增写入地址。 */
    }

    /* 停止总线，提交末页。 */
    IIC_Stop();

    /* 轮询末页完成，每次检测后停止总线。 */
    for (ack_retry_count = 0U;
        ack_retry_count < AT24CXX_ACK_RETRY_LIMIT;
        ack_retry_count++)
    {
        IIC_Start();
        IIC_SendByte(AT24CXX_DEVICE_ADDR | EEPROM_I2C_WR);
        if (IIC_ACK_RECEIVED == IIC_WaitAck())
        {
            IIC_Stop();
            return 0;
        }
        IIC_Stop();
    }
    return -3;

cmd_fail: /* 失败时停止总线。 */

    IIC_Stop();
    return result;
}

/**
 * @brief 全片写入 0xFF。
 *
 * 处理步骤：
 * 1. 填充擦除缓冲区。
 * 2. 写入全片并等待末页完成。
 * 3. 输出处理结果。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 覆盖全部 256 字节，失败可能已部分写入。
 * @note 须先调用 AT24Cxx_Init()。
 */
void AT24Cxx_Erase(void)
{
    uint16_t byte_index = 0U;       /* 擦除索引 */
    uint8_t erase_buf[AT24CXX_SIZE]; /* 擦除缓冲区。 */

    /* 1. 将缓冲区填充为擦除值。 */
    for (byte_index = 0U; byte_index < AT24CXX_SIZE; byte_index++)
    {
        erase_buf[byte_index] = AT24CXX_ERASE_VALUE;
    }

    /* 2. 从地址零写入全片。 */
    if (0 != AT24Cxx_WriteBytes(erase_buf, 0U, AT24CXX_SIZE))
    {
#if AT24CXX_DEBUG
        elog_e(TAG, "EEPROM erase failed.");
#endif
        return;
    }
    else
    {
#if AT24CXX_DEBUG
        elog_i(TAG, "EEPROM erase successful.");
#endif
    }
}

/**
 * @brief 检测设备，写入 F1～F8，读回打印并验证。
 *
 * 处理步骤：
 * 1. 检测设备应答，向末页写入 8 字节固定数据 F1～F8。
 * 2. 写入完成后读回；检测或读写失败时立即返回。
 * 3. 逐字节打印回读地址和数据。
 * 4. 比较读写数据，返回验证结果。
 *
 * @param 无。
 *
 * @return 测试结果。
 * @retval 0 设备应答、写入、读取及数据比较均成功。
 * @retval -1 设备未应答。
 * @retval -2 写入失败。
 * @retval -3 读取失败。
 * @retval -4 回读数据不一致。
 * @note 须先调用 AT24Cxx_Init()；覆盖地址 0xF8～0xFF。
 * @note 写入接口轮询完成后才读取，无额外延时或 SysTick 依赖。
 */
int AT24Cxx_Test(void)
{
    /* 固定测试数据。 */
    const uint8_t write_buf[8] =
    {
        0xF1U, 0xF2U, 0xF3U, 0xF4U,
        0xF5U, 0xF6U, 0xF7U, 0xF8U
    };
    uint8_t read_buf[8] = {0U}; /* 回读缓冲区。 */

    /* 最后一页起始地址：0xF8。 */
    const uint16_t address = AT24CXX_SIZE - AT24CXX_PAGE_SIZE;

    /* 检测、写入、读取依次执行；任一步失败即返回。 */
    if (0 != AT24Cxx_CheckDevice())
    {
        return -1;
    }
    if (0 != AT24Cxx_WriteBytes(write_buf, address, sizeof(write_buf)))
    {
        return -2;
    }
    if (0 != AT24Cxx_ReadBytes(read_buf, address, sizeof(read_buf)))
    {
        return -3;
    }

#if AT24CXX_DEBUG
    /* 读回成功后，逐字节打印地址和数据。 */
    for (uint8_t byte_index = 0U;
         byte_index < sizeof(read_buf); byte_index++)
    {
        elog_i(TAG, "addr=0x%02X, read=0x%02X",
               (unsigned int)(address + byte_index),
               (unsigned int)read_buf[byte_index]);
    }
#endif

    /* 比较全部 8 字节，完全一致时返回 0。 */
    return (0 == memcmp(write_buf, read_buf, sizeof(write_buf))) ? 0 : -4;
}

/************************ Function Definitions ************************/
