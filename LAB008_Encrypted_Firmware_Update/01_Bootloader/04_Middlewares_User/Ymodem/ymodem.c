/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file ymodem.c
 *
 * @par dependencies
 * - ymodem.h
 * - string.h
 * - w25qxx_handler.h
 * - common.h
 * - checksum.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Receives CRC-checked YMODEM firmware into the external A region.
 *
 * Processing flow:
 *
 * Validate the file size and packet CRC, then write file bytes to A.
 * Acknowledge retransmissions, flush remaining data, and check the length.
 * Packet CRC checks do not calculate a whole-file CRC.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "ymodem.h"          /* 用户中间件层：YMODEM 接口 */

#include <string.h>          /* C 标准库 */

#include "w25qxx_handler.h"  /* 用户 BSP 层：分区读写接口 */

#include "common.h"          /* 中间件层：串口、转换及 CRC 接口 */
#include "checksum.h"
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define PACKET_SEQNO_INDEX         (1U)     /* 包序号位置 */
#define PACKET_SEQNO_COMP_INDEX    (2U)     /* 序号反码位置 */
#define PACKET_HEADER              (3U)     /* 包头字节数 */
#define PACKET_TRAILER             (2U)     /* CRC 字节数 */
/* 包头与包尾的总字节数。 */
#define PACKET_OVERHEAD            (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE                (128U)   /* 短包数据区字节数 */
#define PACKET_1K_SIZE             (1024U)  /* 长包数据区字节数 */
#define FILE_NAME_LENGTH           (256U)   /* 文件名扫描上限 */
#define FILE_SIZE_LENGTH           (16U)    /* 长度字符串缓冲区字节数 */

#define SOH                        (0x01U)  /* 短包起始符 */
#define STX                        (0x02U)  /* 长包起始符 */
#define EOT                        (0x04U)  /* 文件数据结束符 */
#define ACK                        (0x06U)  /* 接收确认 */
#define NAK                        (0x15U)  /* 请求重传 */
#define CA                         (0x18U)  /* 连续两字节取消 */
#define CRC16                      (0x43U)  /* 请求 CRC 模式的字符 C */
#define ABORT1                     (0x41U)  /* 用户终止字符 A */
#define ABORT2                     (0x61U)  /* 用户终止字符 a */

/* 单字节最大轮询次数，不是毫秒超时。 */
#define NAK_TIMEOUT                (0x100000U)
#define MAX_ERRORS                 (3)      /* 连续错误超过此值时取消 */
/* 外部 A 区允许接收的最大文件字节数。 */
#define YMODEM_MAX_FILE_SIZE        ((uint32_t)W25QXX_HANDLER_REGION_SIZE)
/****************************** Defines ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 在指定轮询次数内接收一个字节。
 *
 * @param[out] p_character 接收字节的输出指针，不得为空。
 * @param[in]  timeout     最大轮询次数。
 *
 * @return int32_t 字节接收结果。
 * @retval  0 接收成功。
 * @retval -1 超时或参数无效。
 */
static int32_t Ymodem_ReceiveByte(uint8_t *p_character, uint32_t timeout)
{
    if (NULL == p_character)
    {
        return -1;
    }

    while (timeout-- > 0U)
    {
        if (0 == Common_TryReceiveByte(p_character))
        {
            return 0;
        }
    }

    return -1;
}

/**
 * @brief 发送一个 YMODEM 控制字节。
 *
 * @param[in] character 待发送的控制字节。
 *
 * @return 无。
 */
static void Ymodem_SendByte(uint8_t character)
{
    Common_SendByte(character);
}

/**
 * @brief 接收并校验一个 YMODEM 数据包。
 *
 * 数据包检查序号反码及完整数据区 CRC，包含填充字节。
 * 控制字符不执行 CRC 校验；期望序号由会话层检查。
 *
 * @param[out] p_data   完整包缓冲区，至少 1029 字节，含包头和包尾。
 * @param[out] p_length 成功时的数据区长度；0 表示结束，-1 表示取消。
 * @param[in]  timeout  每个字节的最大轮询次数。
 *
 * @return int32_t 数据包接收结果。
 * @retval 0 收到有效数据包、结束标志或发送端取消标志。
 * @retval -1 参数无效、超时、包格式或 CRC 错误。
 * @retval -2 收到用户终止字符。
 */
static int32_t Ymodem_ReceivePacket(uint8_t *p_data,
                                    int32_t *p_length,
                                    uint32_t timeout)
{
    uint16_t index = 0U;           /* 包内接收位置 */
    uint16_t packet_size = 0U;     /* 数据区长度 */
    uint8_t character = 0U;        /* 接收字节或控制字符 */
    uint16_t crc_calculated = 0U;  /* 本地数据区 CRC */
    uint16_t crc_received = 0U;    /* 包尾 CRC */

    if ((NULL == p_data) || (NULL == p_length))
    {
        return -1;
    }

    *p_length = 0;
    if (0 != Ymodem_ReceiveByte(&character, timeout))
    {
        return -1;
    }

    /* 识别包类型及数据区长度。 */
    switch (character)
    {
        case SOH:
            packet_size = PACKET_SIZE;
            break;

        case STX:
            packet_size = PACKET_1K_SIZE;
            break;

        case EOT:
            return 0;

        case CA:
            if ((0 == Ymodem_ReceiveByte(&character, timeout)) &&
                (CA == character))
            {
                *p_length = -1;
                return 0;
            }

            return -1;

        case ABORT1:
        case ABORT2:
            return -2;

        default:
            return -1;
    }

    /* 保存包头，接收序号、数据区及 CRC。 */
    p_data[0] = character;
    for (index = 1U;
         index < (packet_size + PACKET_OVERHEAD);
         index++)
    {
        if (0 != Ymodem_ReceiveByte(&p_data[index], timeout))
        {
            return -1;
        }
    }

    /* 校验包序号反码。 */
    if (p_data[PACKET_SEQNO_INDEX] !=
        ((p_data[PACKET_SEQNO_COMP_INDEX] ^ 0xFFU) & 0xFFU))
    {
        return -1;
    }

    /* 校验完整数据区，包含协议填充。 */
    crc_calculated = crc_xmodem(&p_data[PACKET_HEADER], packet_size);

    /* 包尾 CRC 高字节在前。 */
    crc_received =
        (uint16_t)(((uint16_t)p_data[PACKET_HEADER + packet_size] << 8) |
                   p_data[PACKET_HEADER + packet_size + 1U]);

    /* 拒绝 CRC 错误的数据包。 */
    if (crc_calculated != crc_received)
    {
        return -1;
    }

    *p_length = packet_size;
    return 0;
}

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
int32_t Ymodem_Receive(uint8_t *p_buffer)
{
    /* 静态包缓冲区，避免占用 1 KB 启动栈。 */
    static uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t file_size[FILE_SIZE_LENGTH] = {0U};  /* 文件长度字符串 */
    uint8_t *p_file = NULL;              /* 文件信息解析位置 */
    uint8_t *p_buffer_write = p_buffer;  /* 文件暂存缓冲区 */
    int32_t index = 0;                  /* 字段遍历位置 */
    int32_t packet_length = 0;          /* 数据区长度或控制标记 */
    int32_t session_done = 0;           /* 会话结束标记 */
    int32_t file_done = 0;              /* 文件数据结束标记 */
    int32_t packets_received = 0;       /* 下一个期望包的计数 */
    int32_t errors = 0;                 /* 连续接收错误计数 */
    int32_t session_begin = 0;          /* 会话开始标记 */
    int32_t size = 0;                   /* 声明的文件长度 */
    uint32_t bytes_received = 0U;       /* 已提交写入的字节数 */

    if (NULL == p_buffer)
    {
        return -1;
    }

    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);

    /* 接收单个文件，空文件信息包结束会话。 */
    for (session_done = 0, errors = 0, session_begin = 0; ;)
    {
        for (packets_received = 0, file_done = 0,
             p_buffer_write = p_buffer; ;)
        {
            switch (Ymodem_ReceivePacket(packet_data,
                                         &packet_length,
                                         NAK_TIMEOUT))
            {
                case 0:
                    switch (packet_length)
                    {
                        /* 发送端取消。 */
                        case -1:
                            Ymodem_SendByte(ACK);
                            return -2;

                        /* 文件数据结束。 */
                        case 0:
                            errors = 0;
                            if ((size <= 0) ||
                                (bytes_received != (uint32_t)size) ||
                                (W25QXX_HANDLER_OK !=
                                 W25QxxHandler_Flush(W25QXX_HANDLER_REGION_A)))
                            {
                                Ymodem_SendByte(CA);
                                Ymodem_SendByte(CA);
                                return -1;
                            }
                            Ymodem_SendByte(ACK);
                            Ymodem_SendByte(CRC16);
                            file_done = 1;
                            break;

                        default:
                            if ((packets_received > 0) &&
                                (packet_data[PACKET_SEQNO_INDEX] ==
                                 ((packets_received - 1) & 0xFFU)))
                            {
                                errors = 0;
                                /* ACK 丢失后重发的包只确认，不再次写入。 */
                                Ymodem_SendByte(ACK);
                                if (1 == packets_received)
                                {
                                    Ymodem_SendByte(CRC16);
                                }
                            }
                            else if ((packet_data[PACKET_SEQNO_INDEX] &
                                      0xFFU) !=
                                     (packets_received & 0xFFU))
                            {
                                /* 错序与接收失败共用连续错误计数。 */
                                errors++;
                                if (errors > MAX_ERRORS)
                                {
                                    Ymodem_SendByte(CA);
                                    Ymodem_SendByte(CA);
                                    return -4;
                                }
                                Ymodem_SendByte(NAK);
                            }
                            else if (0 == packets_received)
                            {
                                errors = 0;
                                /* 第 0 包保存文件信息。 */
                                if (0U != packet_data[PACKET_HEADER])
                                {
                                    /* 本接口每次只安装一个文件。 */
                                    if (size > 0)
                                    {
                                        Ymodem_SendByte(CA);
                                        Ymodem_SendByte(CA);
                                        return -1;
                                    }
                                    p_file = &packet_data[PACKET_HEADER];

                                    /* 跳过文件名。 */
                                    for (index = 0;
                                         (0U != *p_file) &&
                                         (index < FILE_NAME_LENGTH) &&
                                         (index < packet_length - 1);
                                         index++)
                                    {
                                        p_file++;
                                    }

                                    if ((0U != *p_file) ||
                                        (index >= packet_length - 1))
                                    {
                                        Ymodem_SendByte(CA);
                                        Ymodem_SendByte(CA);
                                        return -1;
                                    }

                                    p_file++;

                                    /* 复制长度字符串并补空字符。 */
                                    for (index = 0;
                                         (' ' != *p_file) &&
                                         ('\0' != *p_file) &&
                                         (p_file <
                                          &packet_data[PACKET_HEADER +
                                                       packet_length - 1]) &&
                                         (index <
                                          (FILE_SIZE_LENGTH - 1U));
                                         index++)
                                    {
                                        file_size[index] = *p_file++;
                                    }
                                    file_size[index] = '\0';

                                    if (0 !=
                                        Common_StringToInt(file_size, &size))
                                    {
                                        Ymodem_SendByte(CA);
                                        Ymodem_SendByte(CA);
                                        return -1;
                                    }

                                    /* 检查文件长度及分区容量。 */
                                    if ((size <= 0) ||
                                        ((uint32_t)size > YMODEM_MAX_FILE_SIZE))
                                    {
                                        Ymodem_SendByte(CA);
                                        Ymodem_SendByte(CA);
                                        return -1;
                                    }

                                    Ymodem_SendByte(ACK);
                                    Ymodem_SendByte(CRC16);
                                }
                                else
                                {
                                    /* 空文件信息包结束会话。 */
                                    Ymodem_SendByte(ACK);
                                    file_done = 1;
                                    session_done = 1;
                                    break;
                                }
                            }
                            else
                            {
                                /* 本包有效文件字节数。 */
                                uint32_t write_length = 0U;

                                errors = 0;
                                if ((size <= 0) ||
                                    (bytes_received >= (uint32_t)size))
                                {
                                    Ymodem_SendByte(CA);
                                    Ymodem_SendByte(CA);
                                    return -1;
                                }
                                /* 最后一包只保存文件字节，忽略 YMODEM 填充。 */
                                write_length = (uint32_t)size - bytes_received;
                                if (write_length > (uint32_t)packet_length)
                                {
                                    write_length = (uint32_t)packet_length;
                                }
                                memcpy(p_buffer_write,
                                       &packet_data[PACKET_HEADER],
                                       write_length);
                                if (W25QXX_HANDLER_OK !=
                                    W25QxxHandler_WriteData(
                                        W25QXX_HANDLER_REGION_A,
                                        p_buffer_write, write_length))
                                {
                                    Ymodem_SendByte(CA);
                                    Ymodem_SendByte(CA);
                                    return -1;
                                }
                                bytes_received += write_length;
                                Ymodem_SendByte(ACK);
                            }

                            if (packet_data[PACKET_SEQNO_INDEX] ==
                                (packets_received & 0xFFU))
                            {
                                packets_received++;
                            }
                            session_begin = 1;
                            break;
                    }
                    break;

                case -2:
                    Ymodem_SendByte(CA);
                    Ymodem_SendByte(CA);
                    return -3;

                default:
                    /* 会话开始后累计超时、格式或 CRC 错误。 */
                    if (session_begin > 0)
                    {
                        errors++;
                    }

                    if (errors > MAX_ERRORS)
                    {
                        Ymodem_SendByte(CA);
                        Ymodem_SendByte(CA);
                        return -4;
                    }

                    Ymodem_SendByte(CRC16);
                    break;
            }

            if (0 != file_done)
            {
                break;
            }
        }

        if (0 != session_done)
        {
            break;
        }
    }

    if ((size <= 0) ||
        (W25QxxHandler_GetWrittenSize(W25QXX_HANDLER_REGION_A) !=
         (uint32_t)size))
    {
        return -1;
    }
    return size;
}

/************************ Function Definitions ************************/
