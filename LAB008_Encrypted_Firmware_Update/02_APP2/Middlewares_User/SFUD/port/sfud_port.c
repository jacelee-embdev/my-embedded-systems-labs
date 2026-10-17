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
 * @brief Adapts SFUD to APP2 HAL SPI1 and the FreeRTOS time base.
 * Original upstream copyright and license are retained.
 * The port reuses the SPI1 peripheral initialized by main().
 ******************************************************************************/

/****************************** Includes ******************************/

#include <sfud.h>  /* 中间件层：SFUD 接口与配置 */

#include <stdarg.h>  /* C 标准库：可变参数、类型与内存操作 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"  /* Core/HAL 层：片选引脚与 SPI1 接口 */
#include "spi.h"

#include "FreeRTOS.h"  /* 中间件层：FreeRTOS 任务与节拍接口 */
#include "task.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define SFUD_SPI_TIMEOUT_MS (1000U)  /* 单次 HAL 传输的超时上限。 */
#define SFUD_RETRY_TIMES (10000U)    /* 忙状态轮询的最大重试次数。 */
/****************************** Defines ******************************/

/************************ Variable Definitions ************************/
#ifdef SFUD_DEBUG_MODE
/* 调试日志共用缓冲区，按现有串行访问方式使用。 */
static char log_buf[256];
#endif
/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/
/**
 * @brief 在同一次片选期间先发送命令，再接收数据。
 *
 * 1. 检查非零长度对应的缓冲区是否有效。
 * 2. 拉低片选，分段发送命令及待写数据。
 * 3. 接收缓冲区先填充 0xFF，再复用该缓冲区发送占位字节。
 * 4. 成功与传输失败均通过统一出口释放片选。
 *
 * @param[in] spi 总线设备描述，当前固定使用本地 hspi1。
 * @param[in] write_buf 发送缓冲区，零长度时不访问。
 * @param[in] write_size 待发送字节数。
 * @param[out] read_buf 接收缓冲区，零长度时不访问。
 * @param[in] read_size 待读取字节数。
 *
 * @return 总线传输结果。
 * @retval SFUD_SUCCESS 传输成功。
 * @retval SFUD_ERR_TIMEOUT HAL 传输超时。
 * @retval SFUD_ERR_WRITE 发送参数无效或发送失败。
 * @retval SFUD_ERR_READ 接收参数无效或接收失败。
 *
 * @note 按 HAL 的 16 位长度分段，分段期间保持片选有效。
 */
static sfud_err spi_write_read(const sfud_spi *spi,
                               const uint8_t *write_buf, size_t write_size,
                               uint8_t *read_buf, size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;  /* 本次事务结果。 */
    HAL_StatusTypeDef status = HAL_OK;  /* 单次 HAL 传输结果。 */
    size_t offset = 0U;  /* 当前阶段已传输的字节数。 */
    uint16_t transfer_size = 0U;  /* 本次分段传输的字节数。 */
    (void)spi;

    if ((0U != write_size) && (NULL == write_buf))
    {
        return SFUD_ERR_WRITE;
    }
    if ((0U != read_size) && (NULL == read_buf))
    {
        return SFUD_ERR_READ;
    }

    /* 1. 一个写后读事务共用一次片选。 */
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

    /* 2. 按 HAL 接口长度上限分段发送。 */
    while (offset < write_size)
    {
        transfer_size = (uint16_t)((write_size - offset > UINT16_MAX)
                                 ? UINT16_MAX : write_size - offset);
        status = HAL_SPI_Transmit(&hspi1, (uint8_t *)&write_buf[offset],
                                  transfer_size, SFUD_SPI_TIMEOUT_MS);
        if (HAL_OK != status)
        {
            result = (HAL_TIMEOUT == status)
                   ? SFUD_ERR_TIMEOUT : SFUD_ERR_WRITE;
            goto exit;
        }
        offset += transfer_size;
    }

    /* 3. 复用接收缓冲区发送占位字节，不申请额外内存。 */
    if (0U != read_size)
    {
        memset(read_buf, 0xFF, read_size);
        offset = 0U;
        while (offset < read_size)
        {
            transfer_size = (uint16_t)((read_size - offset > UINT16_MAX)
                                     ? UINT16_MAX : read_size - offset);
            status = HAL_SPI_TransmitReceive(&hspi1, &read_buf[offset],
                                             &read_buf[offset], transfer_size,
                                             SFUD_SPI_TIMEOUT_MS);
            if (HAL_OK != status)
            {
                result = (HAL_TIMEOUT == status)
                       ? SFUD_ERR_TIMEOUT : SFUD_ERR_READ;
                goto exit;
            }
            offset += transfer_size;
        }
    }

exit:
    /* 4. 统一释放片选，避免错误后影响下一次命令。 */
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
    return result;
}

/**
 * @brief 执行一次重试延时，实际延时为 1 ms。
 *
 * 调度器运行时让出当前任务；启动前或调度器挂起时使用 HAL 时基。
 * 当前工程的 HAL 时基由独立定时器提供。
 *
 * @return 无。
 *
 * @note 保留原函数名；仅供启动阶段及正常任务上下文调用。
 */
static void retry_delay_100us(void)
{
    TickType_t delay_ticks = pdMS_TO_TICKS(1U);  /* 任务延时的节拍数。 */

    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState())
    {
        /* 节拍频率改变后，仍保证至少让出一个节拍。 */
        vTaskDelay((0U != delay_ticks) ? delay_ticks : 1U);
    }
    else
    {
        HAL_Delay(1U);
    }
}

/**
 * @brief 按设备索引配置 HAL 总线端口与重试参数。
 *
 * 使用 main() 已经初始化的 hspi1，不重复初始化外设。
 *
 * @param[in,out] flash 待绑定回调的闪存设备。
 *
 * @return 端口绑定结果。
 * @retval SFUD_SUCCESS 支持的设备索引已完成绑定。
 * @retval SFUD_ERR_NOT_FOUND 设备为空或索引不受支持。
 *
 * @note 当前 OTA 通过队列和完成信号量串行访问闪存，锁回调为空。
 *       后续增加并发访问者时，须同时保护总线和管理层状态。
 */
sfud_err sfud_spi_port_init(sfud_flash *flash)
{
    if ((NULL == flash) || (SFUD_W25Q64_DEVICE_INDEX != flash->index))
    {
        return SFUD_ERR_NOT_FOUND;
    }

    flash->spi.wr = spi_write_read;
    flash->spi.lock = NULL;
    flash->spi.unlock = NULL;
    flash->spi.user_data = NULL;
    flash->retry.delay = retry_delay_100us;
    flash->retry.times = SFUD_RETRY_TIMES;
    return SFUD_SUCCESS;
}

/**
 * @brief 按调试编译开关输出带来源位置的日志。
 *
 * @param[in] file 调用来源文件名。
 * @param[in] line 调用来源行号。
 * @param[in] format 输出格式字符串。
 * @param[in] ... 可变参数。
 *
 * @return 无。
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...)
{
#ifdef SFUD_DEBUG_MODE
    va_list args;  /* 可变参数列表。 */

    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
#else
    (void)file;
    (void)line;
    (void)format;
#endif
}

/**
 * @brief 按调试编译开关输出常规日志。
 *
 * @param[in] format 输出格式字符串。
 * @param[in] ... 可变参数。
 *
 * @return 无。
 *
 * @note 未启用 SFUD_DEBUG_MODE 时不输出内容。
 */
void sfud_log_info(const char *format, ...)
{
#ifdef SFUD_DEBUG_MODE
    va_list args;  /* 可变参数列表。 */

    va_start(args, format);
    printf("[SFUD]");
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
#else
    (void)format;
#endif
}
/************************ Function Definitions ************************/
