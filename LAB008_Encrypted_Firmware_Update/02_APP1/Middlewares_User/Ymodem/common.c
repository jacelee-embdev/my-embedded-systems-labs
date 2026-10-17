/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file common.c
 *
 * @par dependencies
 * - common.h
 * - stddef.h
 * - usart.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements conversion and serial helpers used by YMODEM.
 *
 * Processing flow:
 *
 * Convert a size string, or poll and send bytes through USART1.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "common.h"          /* 用户中间件层：公共辅助接口 */

#include <stddef.h>          /* C 标准库 */

#include "usart.h"           /* Core 层：HAL 串口接口 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
/* 判断大写十六进制字母。 */
#define IS_UPPER_HEX(character) \
    (((character) >= 'A') && ((character) <= 'F'))
/* 判断小写十六进制字母。 */
#define IS_LOWER_HEX(character) \
    (((character) >= 'a') && ((character) <= 'f'))
/* 判断十进制数字。 */
#define IS_DECIMAL(character) \
    (((character) >= '0') && ((character) <= '9'))
/* 判断十六进制字符。 */
#define IS_VALID_HEX(character) \
    (IS_UPPER_HEX(character) || IS_LOWER_HEX(character) || \
     IS_DECIMAL(character))
/* 转换十进制数字。 */
#define CONVERT_DECIMAL(character)    ((character) - '0')
/* 转换十六进制字母。 */
#define CONVERT_HEX_ALPHA(character) \
    (IS_UPPER_HEX(character) ? ((character) - 'A' + 10) : \
                               ((character) - 'a' + 10))
/* 转换十六进制字符。 */
#define CONVERT_HEX(character) \
    (IS_DECIMAL(character) ? CONVERT_DECIMAL(character) : \
                             CONVERT_HEX_ALPHA(character))
/****************************** Defines ******************************/

/************************ Function Definitions ************************/

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
                       int32_t *p_integer)
{
    uint8_t index = 0U;    /* 字符位置 */
    int result = -2;       /* 转换状态 */
    uint32_t value = 0U;   /* 数值累加结果 */

    if ((NULL == p_input_string) || (NULL == p_integer))
    {
        return -1;
    }

    if ('\0' == p_input_string[0])
    {
        return -2;
    }

    /* 解析十六进制数字。 */
    if (('0' == p_input_string[0]) &&
        (('x' == p_input_string[1]) || ('X' == p_input_string[1])))
    {
        if ('\0' == p_input_string[2])
        {
            return -2;
        }

        for (index = 2U; index < 11U; index++)
        {
            if ('\0' == p_input_string[index])
            {
                *p_integer = (int32_t)value;
                result = 0;
                break;
            }

            if (IS_VALID_HEX(p_input_string[index]))
            {
                /* 移位前检查结果上限。 */
                if (value > ((uint32_t)INT32_MAX >> 4U))
                {
                    return -2;
                }
                value = (value << 4U) +
                        CONVERT_HEX(p_input_string[index]);
            }
            else
            {
                result = -2;
                break;
            }
        }

        if (index >= 11U)
        {
            result = -2;
        }
    }
    else
    {
        /* 解析十进制数字及 K/M 后缀。 */
        for (index = 0U; index < 11U; index++)
        {
            if ('\0' == p_input_string[index])
            {
                *p_integer = (int32_t)value;
                result = 0;
                break;
            }
            else if ((index > 0U) &&
                     (('k' == p_input_string[index]) ||
                      ('K' == p_input_string[index])))
            {
                /* 检查后缀结束位置及换算范围。 */
                if (('\0' != p_input_string[index + 1U]) ||
                    (value > ((uint32_t)INT32_MAX >> 10U)))
                {
                    return -2;
                }
                value <<= 10U;
                *p_integer = (int32_t)value;
                result = 0;
                break;
            }
            else if ((index > 0U) &&
                     (('m' == p_input_string[index]) ||
                      ('M' == p_input_string[index])))
            {
                /* 检查后缀结束位置及换算范围。 */
                if (('\0' != p_input_string[index + 1U]) ||
                    (value > ((uint32_t)INT32_MAX >> 20U)))
                {
                    return -2;
                }
                value <<= 20U;
                *p_integer = (int32_t)value;
                result = 0;
                break;
            }
            else if (IS_DECIMAL(p_input_string[index]))
            {
                /* 累加前检查结果上限。 */
                if (value >
                    (((uint32_t)INT32_MAX -
                      (uint32_t)CONVERT_DECIMAL(p_input_string[index])) /
                     10U))
                {
                    return -2;
                }
                value = (value * 10U) +
                        CONVERT_DECIMAL(p_input_string[index]);
            }
            else
            {
                result = -2;
                break;
            }
        }

        if (index >= 11U)
        {
            result = -2;
        }
    }

    return result;
}

/**
 * @brief 非阻塞查询 USART1 并读取一个字节。
 *
 * @param[out] p_character 接收字节的输出指针。
 *
 * @return 接收状态。
 * @retval 0 收到字节并更新输出。
 * @retval -1 输出指针为空，不修改输出。
 * @retval -2 暂无数据，不修改输出；调用方可继续轮询。
 * @note USART1 须已初始化，且 DMA 接收已关闭。
 * @note 仅查询一次，不等待数据。
 */
int Common_TryReceiveByte(uint8_t *p_character)
{
    if (NULL == p_character)
    {
        return -1;
    }

    if (RESET != __HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE))
    {
        *p_character = (uint8_t)huart1.Instance->DR;
        return 0;
    }

    return -2;
}

/**
 * @brief 通过 USART1 发送一个字节。
 *
 * @param[in] character 待发送的字节。
 *
 * @return 无。
 * @note USART1 须已初始化；阻塞发送，超时为 1000 毫秒。
 * @note 不反馈 HAL 发送结果。
 */
void Common_SendByte(uint8_t character)
{
    (void)HAL_UART_Transmit(&huart1, &character, 1U, 1000U);
}

/************************ Function Definitions ************************/
