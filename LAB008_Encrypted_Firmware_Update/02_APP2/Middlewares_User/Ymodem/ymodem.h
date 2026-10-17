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
 * @brief Declares the APP dual-buffer YMODEM interface.
 *
 * Processing flow:
 *
 * Prepare the queues and semaphore, then pass two receive buffers.
 * Check the returned file length or session error.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __YMODEM_H__
#define __YMODEM_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/************************ Variable Declarations ***********************/
extern int32_t g_ymodem_packet_length;  /* 包长或控制标记，供写入任务使用 */
/************************ Variable Declarations ***********************/

/************************ Function Declarations ************************/
/**
 * @brief 双缓冲接收加密固件并交给写入任务。
 *
 * 处理步骤：
 * 1. 准备 DMA 接收，解析并检查文件信息。
 * 2. 校验数据包，将有效字节交给写入任务。
 * 3. 等待处理完成，再切换缓冲区并应答。
 * 4. 核对完整长度，停止接收并返回结果。
 *
 * @param[in,out] p_buffer_1 第一缓冲区，至少 1030 字节，不得为空。
 * @param[in,out] p_buffer_2 第二缓冲区，同样大小且与第一缓冲区不同。
 *
 * @return 正值为完整接收的文件字节数，最大 64 KiB。
 * @retval -1 参数、文件、DMA 准备或任务处理失败。
 * @retval -2 发送端取消。
 * @retval -3 用户终止。
 * @retval -4 连续接收或错序错误超过阈值。
 * @note 任务上下文调用，不可重入；队列和信号量须已创建。
 * @note 加密包须大于 16 字节并按 16 字节对齐。
 * @note 重传包只确认；每次任务交接最多等待 2 秒。
 * @note 逐包校验 CRC，填充不写入；不计算整文件 CRC。
 * @note 每次 IDLE/TC 按整包处理；无输入时持续等待首包。
 */
int32_t Ymodem_Receive(uint8_t *p_buffer_1, uint8_t *p_buffer_2);
/************************ Function Declarations ************************/

#endif /* __YMODEM_H__ */
