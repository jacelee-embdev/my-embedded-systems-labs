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
 * Function: It is an head file for this library. You can see all of the functions which can be called by user.
 * Created on: 2016-04-23
 */

/******************************************************************************
 * @file sfud.h
 * @brief SFUD integration documentation for the local bootloader.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

#ifndef _SFUD_H_
#define _SFUD_H_

/****************************** Includes ******************************/

#include "sfud_def.h" /* 中间件层：SFUD 接口与配置 */

/****************************** Includes ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

/****************************** Declaring ******************************/

/* 公开接口由 sfud.c 实现。 */
/**
 * @brief 初始化设备表中的全部闪存设备。
 *
 * @return 全部成功时返回 SFUD_SUCCESS；否则返回最后一次初始化错误。
 */
sfud_err sfud_init(void);

/**
 * @brief 初始化指定设备的端口、芯片参数与软件状态。
 *
 * @param[in,out] flash : 待初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_device_init(sfud_flash *flash);

/**
 * @brief 按设备表索引获取闪存设备。
 *
 * @param[in] index : 设备表中的索引。
 *
 * @return 有效索引对应的设备指针；索引越界时返回 NULL。
 */
sfud_flash *sfud_get_device(size_t index);

/**
 * @brief 获取设备表中的闪存设备数量。
 *
 * @return 已配置的设备数量。
 */
size_t sfud_get_device_num(void);

/**
 * @brief 获取闪存设备表的首地址。
 *
 * @return 设备表首元素的指针。
 */
const sfud_flash *sfud_get_device_table(void);

#ifdef SFUD_USING_QSPI
/**
 * @brief 依据芯片扩展表与总线宽度配置快速读取格式。
 *
 * @param[in,out] flash       : 已配置或已初始化的闪存设备。
 * @param[in] data_line_width : 总线支持的最大数据线宽度：1、2 或 4。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 须在设备初始化成功后调用；未匹配时使用普通读取格式。
 */
sfud_err sfud_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width);
#endif /* SFUD_USING_QSPI */

/**
 * @brief 从指定地址读取闪存数据。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[out] data : 接收读取数据的缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 设备须初始化成功，访问范围须在芯片容量内。
 */
sfud_err sfud_read(const sfud_flash *flash,
                   uint32_t addr,
                   size_t size,
                   uint8_t *data);

/**
 * @brief 按器件擦除粒度擦除指定范围。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 擦除覆盖完整擦除单元，可能影响请求范围边界外的数据。
 */
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size);

/**
 * @brief 依据器件写入模式执行闪存编程。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[in] data  : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 不执行擦除；双缓冲写入分支尚未实现。
 */
sfud_err sfud_write(const sfud_flash *flash,
                    uint32_t addr,
                    size_t size,
                    const uint8_t *data);

/**
 * @brief 先擦除指定范围，再写入数据。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 * @param[in] addr  : 闪存访问起始字节地址。
 * @param[in] size  : 本次访问的字节数。
 * @param[in] data  : 待写入的数据缓冲区。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 *
 * @note 擦除影响完整擦除单元，不保存请求范围边界外的原有数据。
 */
sfud_err sfud_erase_write(const sfud_flash *flash,
                          uint32_t addr,
                          size_t size,
                          const uint8_t *data);

/**
 * @brief 执行整片闪存擦除并等待完成。
 *
 * @param[in] flash : 已配置或已初始化的闪存设备。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_chip_erase(const sfud_flash *flash);

/**
 * @brief 读取闪存状态寄存器。
 *
 * @param[in] flash   : 已配置或已初始化的闪存设备。
 * @param[out] status : 状态寄存器值的输出地址。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *status);

/**
 * @brief 按易失或非易失方式写入状态寄存器。
 *
 * @param[in] flash       : 已配置或已初始化的闪存设备。
 * @param[in] is_volatile : true 使用易失写使能命令，false 使用常规写使能。
 * @param[in] status      : 待写入的状态寄存器值。
 *
 * @return SFUD 操作结果；SFUD_SUCCESS 表示成功，其他值表示错误。
 */
sfud_err sfud_write_status(const sfud_flash *flash,
                           bool is_volatile,
                           uint8_t status);

/****************************** Declaring ******************************/

#ifdef __cplusplus
}
#endif

#endif /* _SFUD_H_ */
