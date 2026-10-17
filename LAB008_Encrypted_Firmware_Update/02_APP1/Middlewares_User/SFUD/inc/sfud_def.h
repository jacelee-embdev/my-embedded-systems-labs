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
 * Function: It is the macro definition head file for this library.
 * Created on: 2016-04-23
 */

/******************************************************************************
 * @file sfud_def.h
 * @brief SFUD integration documentation for the local APP1 project.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

#ifndef _SFUD_DEF_H_
#define _SFUD_DEF_H_

/****************************** Includes ******************************/

#include <stdio.h> /* C 标准库 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <sfud_cfg.h> /* 中间件层：SFUD 接口与配置 */
#include "sfud_flash_def.h"

/****************************** Includes ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************** Defines ******************************/

/* 调试输出接口须由用户端口实现。 */
#ifdef SFUD_DEBUG_MODE
#ifndef SFUD_DEBUG
#define SFUD_DEBUG(...) sfud_log_debug(__FILE__, __LINE__, __VA_ARGS__)
#endif /* SFUD_DEBUG */
#else
#define SFUD_DEBUG(...)
#endif /* SFUD_DEBUG_MODE */

#ifndef SFUD_INFO
#define SFUD_INFO(...) sfud_log_info(__VA_ARGS__)
#endif

/* 调试模式下启用断言；非调试模式下宏为空。 */
#ifdef SFUD_DEBUG_MODE
#define SFUD_ASSERT(EXPR)                                                      \
    if (!(EXPR))                                                               \
    {                                                                          \
        SFUD_DEBUG("(%s) has assert failed at %s.", #EXPR, __FUNCTION__);      \
        while (1)                                                              \
            ;                                                                  \
    }
#else
#define SFUD_ASSERT(EXPR)
#endif

/**
 * @brief 执行一次延时重试，并在次数耗尽时设置超时结果。
 *
 * @param[in] delay      : 延时回调；NULL 表示不延时。
 * @param[in,out] retry  : 剩余重试次数。
 * @param[in,out] result : 次数耗尽时设置为 SFUD_ERR_TIMEOUT。
 *
 * @note 在循环体内使用；宏中的 break 用于退出调用处循环。
 */
#define SFUD_RETRY_PROCESS(delay, retry, result)                               \
    void (*__delay_temp)(void) = (void (*)(void))delay;                        \
    if (retry == 0)                                                            \
    {                                                                          \
        result = SFUD_ERR_TIMEOUT;                                             \
        break;                                                                 \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        if (__delay_temp)                                                      \
        {                                                                      \
            __delay_temp();                                                    \
        }                                                                      \
        retry--;                                                               \
    }

/* 库的软件版本号。 */
#define SFUD_SW_VERSION                 "1.1.0"
/* 支持的命令定义；已由外部定义的命令保持原值。 */
#ifndef SFUD_CMD_WRITE_ENABLE
#define SFUD_CMD_WRITE_ENABLE           0x06
#endif

#ifndef SFUD_CMD_WRITE_DISABLE
#define SFUD_CMD_WRITE_DISABLE          0x04
#endif

#ifndef SFUD_CMD_READ_STATUS_REGISTER
#define SFUD_CMD_READ_STATUS_REGISTER   0x05
#endif

#ifndef SFUD_VOLATILE_SR_WRITE_ENABLE
#define SFUD_VOLATILE_SR_WRITE_ENABLE   0x50
#endif

#ifndef SFUD_CMD_WRITE_STATUS_REGISTER
#define SFUD_CMD_WRITE_STATUS_REGISTER  0x01
#endif

#ifndef SFUD_CMD_PAGE_PROGRAM
#define SFUD_CMD_PAGE_PROGRAM           0x02
#endif

#ifndef SFUD_CMD_AAI_WORD_PROGRAM
#define SFUD_CMD_AAI_WORD_PROGRAM       0xAD
#endif

#ifndef SFUD_CMD_ERASE_CHIP
#define SFUD_CMD_ERASE_CHIP             0xC7
#endif

#ifndef SFUD_CMD_READ_DATA
#define SFUD_CMD_READ_DATA              0x03
#endif

#ifndef SFUD_CMD_FAST_READ_DATA
#define SFUD_CMD_FAST_READ_DATA         0x0B
#endif

#ifndef SFUD_CMD_DUAL_OUTPUT_READ_DATA
#define SFUD_CMD_DUAL_OUTPUT_READ_DATA  0x3B
#endif

#ifndef SFUD_CMD_DUAL_IO_READ_DATA
#define SFUD_CMD_DUAL_IO_READ_DATA      0xBB
#endif

#ifndef SFUD_CMD_QUAD_IO_READ_DATA
#define SFUD_CMD_QUAD_IO_READ_DATA      0xEB
#endif

#ifndef SFUD_CMD_QUAD_OUTPUT_READ_DATA
#define SFUD_CMD_QUAD_OUTPUT_READ_DATA  0x6B
#endif

#ifndef SFUD_CMD_MANUFACTURER_DEVICE_ID
#define SFUD_CMD_MANUFACTURER_DEVICE_ID 0x90
#endif

#ifndef SFUD_CMD_JEDEC_ID
#define SFUD_CMD_JEDEC_ID               0x9F
#endif

#ifndef SFUD_CMD_READ_UNIQUE_ID
#define SFUD_CMD_READ_UNIQUE_ID         0x4B
#endif

#ifndef SFUD_CMD_READ_SFDP_REGISTER
#define SFUD_CMD_READ_SFDP_REGISTER     0x5A
#endif

#ifndef SFUD_CMD_ENABLE_RESET
#define SFUD_CMD_ENABLE_RESET           0x66
#endif

#ifndef SFUD_CMD_RESET
#define SFUD_CMD_RESET                  0x99
#endif

#ifndef SFUD_CMD_ENTER_4B_ADDRESS_MODE
#define SFUD_CMD_ENTER_4B_ADDRESS_MODE  0xB7
#endif

#ifndef SFUD_CMD_EXIT_4B_ADDRESS_MODE
#define SFUD_CMD_EXIT_4B_ADDRESS_MODE   0xE9
#endif

#ifndef SFUD_WRITE_MAX_PAGE_SIZE
#define SFUD_WRITE_MAX_PAGE_SIZE        256
#endif

/* 读取数据时发送的占位字节。 */
#ifndef SFUD_DUMMY_DATA
#define SFUD_DUMMY_DATA                 0xFF
#endif

/* 快速读取等操作需要的占位字节数量。 */
#ifndef SFUD_READ_DUMMY_BYTE_CNT
#ifdef SFUD_USING_FAST_READ
#define SFUD_READ_DUMMY_BYTE_CNT        1
#else
#define SFUD_READ_DUMMY_BYTE_CNT        0
#endif
#endif

/* 初版 SFDP 标准支持的最大擦除类型数量。 */
#define SFUD_SFDP_ERASE_TYPE_MAX_NUM    4

/****************************** Defines ******************************/

/************************** Type Definitions **************************/

/**
 * 状态寄存器位定义。
 */
enum
{
    SFUD_STATUS_REGISTER_BUSY = (1 << 0), /**< 器件忙标志。 */
    SFUD_STATUS_REGISTER_WEL  = (1 << 1), /**< 写使能锁存标志。 */
    SFUD_STATUS_REGISTER_SRP  = (1 << 7), /**< 状态寄存器保护位。 */
};

/**
 * 接口错误码。
 */
typedef enum
{
    SFUD_SUCCESS               = 0, /**< 操作成功。 */
    SFUD_ERR_NOT_FOUND         = 1, /**< 未找到器件或功能不受支持。 */
    SFUD_ERR_WRITE             = 2, /**< 写入错误。 */
    SFUD_ERR_READ              = 3, /**< 读取错误。 */
    SFUD_ERR_TIMEOUT           = 4, /**< 操作超时。 */
    SFUD_ERR_ADDR_OUT_OF_BOUND = 5, /**< 访问地址超出芯片容量。 */
} sfud_err;

#ifdef SFUD_USING_QSPI
/**
 * 四线总线闪存读取命令格式。
 */
typedef struct
{
    uint8_t instruction;           /**< 读取指令。 */
    uint8_t instruction_lines;     /**< 指令阶段的线数。 */
    uint8_t address_size;          /**< 地址位宽。 */
    uint8_t address_lines;         /**< 地址阶段的线数。 */
    uint8_t alternate_bytes_lines; /**< 交替字节阶段的线数。 */
    uint8_t dummy_cycles;          /**< 占位时钟周期数。 */
    uint8_t data_lines;            /**< 数据阶段的线数。 */
} sfud_qspi_read_cmd_format;
#endif /* SFUD_USING_QSPI */

/* 总线写后读回调的函数指针类型。 */
typedef sfud_err (*spi_write_read_func)(const uint8_t *write_buf,
                                        size_t write_size,
                                        uint8_t *read_buf,
                                        size_t read_size);

#ifdef SFUD_USING_SFDP
/**
 * 本库使用的 SFDP 参数信息。
 */
typedef struct
{
    bool available;         /**< SFDP 读取解析成功标志。 */
    uint8_t major_rev;      /**< SFDP 主版本。 */
    uint8_t minor_rev;      /**< SFDP 次版本。 */
    uint16_t write_gran;    /**< 写入粒度，单位为字节。 */
    uint8_t erase_4k;       /**< 全地址空间是否支持 4 KiB 擦除。 */
    uint8_t erase_4k_cmd;   /**< 4 KiB 擦除命令。 */
    bool sr_is_non_vola;    /**< 状态寄存器是否支持非易失写入。 */
    uint8_t vola_sr_we_cmd; /**< 易失状态寄存器写使能命令。 */
    bool addr_3_byte;       /**< 是否支持三字节地址。 */
    bool addr_4_byte;       /**< 是否支持四字节地址。 */
    uint32_t capacity;      /**< 芯片容量，单位为字节。 */
    struct
    {
        uint32_t size; /**< 擦除单元尺寸，单位为字节；零表示不可用。 */
        uint8_t cmd;   /**< 擦除命令。 */
    } eraser[SFUD_SFDP_ERASE_TYPE_MAX_NUM]; /**< 支持的擦除类型表。 */
    /* TODO：补充快速读取模式及所需等待周期、占位周期等参数。 */
} sfud_sfdp, *sfud_sfdp_t;
#endif

/**
 * 总线设备与端口回调。
 */
typedef struct __sfud_spi
{
    /* 总线设备名称。 */
    char *name;
    /* 总线写后读回调。 */
    sfud_err (*wr)(const struct __sfud_spi *spi,
                   const uint8_t *write_buf,
                   size_t write_size,
                   uint8_t *read_buf,
                   size_t read_size);
#ifdef SFUD_USING_QSPI
    /* 四线总线快速读取回调。 */
    sfud_err (*qspi_read)(const struct __sfud_spi *spi,
                          uint32_t addr,
                          sfud_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf,
                          size_t read_size);
#endif
    /* 总线加锁回调；可为 NULL。 */
    void (*lock)(const struct __sfud_spi *spi);
    /* 总线解锁回调；可为 NULL。 */
    void (*unlock)(const struct __sfud_spi *spi);
    /* 用户扩展数据。 */
    void *user_data;
} sfud_spi, *sfud_spi_t;

/**
 * 串行闪存设备描述。
 */
typedef struct
{
    char *name;           /**< 串行闪存设备名称。 */
    size_t index;         /**< 设备在信息表中的索引。 */
    sfud_flash_chip chip; /**< 芯片参数。 */
    sfud_spi spi;         /**< 总线设备与回调。 */
    bool init_ok;         /**< 初始化成功标志。 */
    bool addr_in_4_byte;  /**< 当前是否采用四字节地址。 */
    struct
    {
        void (*delay)(void); /**< 每次重试执行的延时回调。 */
        size_t times;        /**< 错误重试的默认次数。 */
    } retry;                 /**< 延时与重试次数配置。 */
    void *user_data;         /**< 用户扩展数据。 */

#ifdef SFUD_USING_QSPI
    sfud_qspi_read_cmd_format read_cmd_format; /**< 快速读取命令格式。 */
#endif

#ifdef SFUD_USING_SFDP
    sfud_sfdp sfdp; /**< 按 JEDEC 标准发现的闪存参数。 */
#endif

} sfud_flash, *sfud_flash_t;

/************************** Type Definitions **************************/

#ifdef __cplusplus
}
#endif

#endif /* _SFUD_DEF_H_ */
