/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

/******************************************************************************
 * @file sfud_port.c
 * @brief SFUD integration documentation for the local bootloader.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

/****************************** Includes ******************************/

#include <sfud.h> /* 中间件层：SFUD 接口与配置 */

#include <stdarg.h> /* C 标准库 */

#include "stm32f4xx_gpio.h" /* 厂商库层：片选引脚操作 */

#include "delay.h" /* 用户 Core 层：延时与 SPI 接口 */
#include "spi.h"

#include "string.h" /* C 标准库 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* 将教师例程的毫秒延时接口映射到本地实现，仅在本文件生效。 */
#define Delay(ms) Delay_Ms((uint32_t)(ms))

/****************************** Defines ******************************/

/************************ Variable Definitions ************************/

/* 调试日志共用缓冲区；当前端口按串行调用使用。 */
#ifdef SFUD_DEBUG_MODE
static char log_buf[256];
#endif

/************************ Variable Definitions ************************/

/****************************** Declaring ******************************/

void sfud_log_debug(const char *file, const long line, const char *format, ...);

/****************************** Declaring ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 在同一次片选期间先发送命令，再接收数据。
 *
 * @param[in] spi        : 总线设备描述，当前使用固定的本地 SPI1。
 * @param[in] write_buf  : 待发送缓冲区；零长度时不访问。
 * @param[in] write_size : 待发送字节数。
 * @param[out] read_buf  : 接收缓冲区；接收前填充 0xFF 作为占位数据。
 * @param[in] read_size  : 待读取字节数。
 *
 * @return 成功返回 SFUD_SUCCESS；传输失败返回 SFUD_ERR_TIMEOUT。
 *
 * @note 正常和错误路径均释放片选。
 */
static sfud_err spi_write_read(const sfud_spi *spi,
                               const uint8_t *write_buf,
                               size_t write_size,
                               uint8_t *read_buf,
                               size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;
    /* 1. 拉低片选，开始本次事务。 */
    GPIO_ResetBits(SPI_CS_GPIO_PORT, SPI_CS_PIN);

    /* 2. 发送命令与待写数据。 */
    if (write_size > 0)
    {
        if (0 != SPI_WriteBytes((uint8_t *)write_buf, write_size, 1000))
        {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }

    /* 3. 接收数据，复用接收缓冲区发送占位字节。 */
    if (read_size > 0)
    {
        /* 本地 SPI 接口使用同一缓冲区发送占位字节并保存接收数据。 */
        memset(read_buf, 0xFF, read_size);

        if (0 != SPI_ReadBytes(read_buf, read_size, 1000))
        {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }

exit:
    /* 4. 统一释放片选。 */
    GPIO_SetBits(SPI_CS_GPIO_PORT, SPI_CS_PIN);
    return result;
}

#ifdef SFUD_USING_QSPI
/**
 * @brief 保留四线总线读取的适配入口。
 *
 * @param[in] spi                  : 总线设备描述。
 * @param[in] addr                 : 闪存起始地址。
 * @param[in] qspi_read_cmd_format : 读取命令格式。
 * @param[out] read_buf            : 接收缓冲区。
 * @param[in] read_size            : 待读取字节数。
 *
 * @return 当前占位实现直接返回 SFUD_SUCCESS。
 *
 * @note 尚未执行实际读取；本工程未启用 SFUD_USING_QSPI。
 */
static sfud_err qspi_read(const struct __sfud_spi *spi,
                          uint32_t addr,
                          sfud_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf,
                          size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;

    /* TODO：补充实际的四线总线读取实现。 */

    return result;
}
#endif /* SFUD_USING_QSPI */

/**
 * @brief 执行一次重试延时，实际延时为 1 ms。
 *
 * @return 无。
 *
 * @note 保留原函数名；通过 Delay 宏调用本地毫秒延时函数。
 */
static void retry_delay_100us(void)
{
    /* 保持教师例程实际使用的 1 ms 重试间隔。 */
    Delay(1);
}

/**
 * @brief 按设备索引配置总线端口和重试参数。
 *
 * @param[in,out] flash : 待绑定端口回调的闪存设备。
 *
 * @return 支持的索引返回 SFUD_SUCCESS，其他索引返回 SFUD_ERR_NOT_FOUND。
 *
 * @note 当前引导程序无总线并发访问，加锁与解锁回调均为空。
 */
sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    sfud_err result = SFUD_SUCCESS;

    switch (flash->index)
    {
        case SFUD_W25Q64_DEVICE_INDEX:
        {
            /* 初始化本地 SPI1 外设。 */
            SPI_InitSpi1();

            /* 绑定写后读回调与总线状态。 */
            flash->spi.wr = spi_write_read;
            /* 当前引导程序无 SPI 并发访问，保持 SysTick 中断可运行。 */
            flash->spi.lock      = NULL;
            flash->spi.unlock    = NULL;
            flash->spi.user_data = NULL;

            /* 设置重试延时和次数。 */
            flash->retry.delay = retry_delay_100us;
            flash->retry.times = 10000;

            break;
        }
        default:
            result = SFUD_ERR_NOT_FOUND;
            break;
    }

    return result;
}

/**
 * @brief 按调试编译开关输出带来源位置的日志。
 *
 * @param[in] file   : 调用来源文件名。
 * @param[in] line   : 调用来源行号。
 * @param[in] format : 输出格式字符串。
 * @param[in] ...    : 可变参数。
 *
 * @return 无。
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...)
{
#ifdef SFUD_DEBUG_MODE
    va_list args;

    /* 初始化可变参数列表。 */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* 将可变参数格式化到有界日志缓冲区。 */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
#endif
}

/**
 * @brief 按调试编译开关输出常规日志。
 *
 * @param[in] format : 输出格式字符串。
 * @param[in] ...    : 可变参数。
 *
 * @return 无。
 *
 * @note 当前端口在未启用 SFUD_DEBUG_MODE 时不输出内容。
 */
void sfud_log_info(const char *format, ...)
{
#ifdef SFUD_DEBUG_MODE
    va_list args;

    /* 初始化可变参数列表。 */
    va_start(args, format);
    printf("[SFUD]");
    /* 将可变参数格式化到有界日志缓冲区。 */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
#endif
}

/************************ Function Definitions ************************/
