/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file at24cxx.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares AT24C02 device detection, data access, erase, and test APIs.
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

#ifndef __ATC24CXX_H__
#define __ATC24CXX_H__

/****************************** Includes ******************************/

#include <stdint.h>  /* C 标准库：固定宽度整数类型 */

/****************************** Includes ******************************/

/************************ Function Declarations ************************/

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
void AT24Cxx_Init(void);

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
int AT24Cxx_CheckDevice(void);

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
    uint16_t byte_count);

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
    uint16_t byte_count);

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
void AT24Cxx_Erase(void);

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
int AT24Cxx_Test(void);

/************************ Function Declarations ************************/

#endif /* __ATC24CXX_H__ */
