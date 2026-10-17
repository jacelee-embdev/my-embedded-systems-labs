/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file common.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares shared conversion and serial helper interfaces.
 *
 * Processing flow:
 *
 * Convert a size string, or poll and send bytes through USART1.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/************************ Function Declarations ************************/
/**
 * @brief 将数字字符串转换为非负整数。
 *
 * 支持十进制、0x/0X 十六进制及十进制 K/M 后缀。
 *
 * @param[in] p_input_string 以空字符结尾的数字字符串，不得为空。
 * @param[out] p_integer 接收 0～INT32_MAX 的结果，不得为空。
 *
 * @return 转换状态。
 * @retval 0 转换成功。
 * @retval -1 输出或输入指针为空，输出保持原值。
 * @retval -2 格式、长度或范围无效，输出保持原值。
 * @note 最多 10 位十进制或 8 位十六进制数字。
 * @note K/M 分别按 1024/1048576 换算，须位于末尾。
 */
int Common_StringToInt(const uint8_t *p_input_string,
                       int32_t *p_integer);

/**
 * @brief 非阻塞查询 USART1 并读取一个字节。
 *
 * @param[out] p_character 接收字节的输出指针。
 *
 * @return 接收状态。
 * @retval 0 收到字节并更新输出。
 * @retval -1 输出指针为空，不修改输出。
 * @retval -2 暂无数据，不修改输出；调用方可继续轮询。
 * @note USART1 须已初始化，仅检查一次接收就绪状态。
 */
int Common_TryReceiveByte(uint8_t *p_character);

/**
 * @brief 通过 USART1 发送一个字节。
 *
 * @param[in] character 待发送的字节。
 *
 * @return 无。
 * @note USART1 须已初始化；阻塞轮询，无超时。
 */
void Common_SendByte(uint8_t character);
/************************ Function Declarations ************************/

#endif /* __COMMON_H__ */
