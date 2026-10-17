/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file ymodem.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the YMODEM interface for receiving APP firmware.
 *
 * Processing flow:
 *
 * Call Ymodem_Receive() with a 1024-byte buffer to receive a file into A.
 * Check the returned file length or session error.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __YMODEM_H__
#define __YMODEM_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 接收一个 YMODEM 固件文件并写入外部 A 区。
 *
 * 逐包校验后写入有效文件字节，结束时刷新并核对长度。
 * 重传包不重复写入，协议填充不保存。
 *
 * @param[in,out] p_buffer 文件暂存缓冲区，至少 1024 字节，不得为空。
 *
 * @return 会话结果；成功返回正的文件字节数，失败返回负数。
 * @retval 正值 完整接收的文件字节数，最大 64 KiB。
 * @retval -1 参数、文件信息、大小、有效长度或闪存写入失败。
 * @retval -2 发送端取消。
 * @retval -3 收到用户终止字符。
 * @retval -4 连续接收或包序号错误超过阈值。
 * @note 阻塞接收单个文件，使用静态包缓冲区，不可重入。
 * @note 无输入时持续等待首包；单字节超时按轮询次数计算。
 * @note 仅检查逐包 CRC 和文件长度，不计算整文件 CRC 或解密。
 */
int32_t Ymodem_Receive(uint8_t *p_buffer);
/************************ Function Declarations ************************/

#endif /* __YMODEM_H__ */
