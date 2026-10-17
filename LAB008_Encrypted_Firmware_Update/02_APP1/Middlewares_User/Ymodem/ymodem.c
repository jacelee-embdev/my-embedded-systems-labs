/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file ymodem.c
 *
 * @par dependencies
 * - ymodem.h
 * - stddef.h
 * - usart.h
 * - uart_handler.h
 * - FreeRTOS.h
 * - queue.h
 * - semphr.h
 * - common.h
 * - checksum.h
 * - debug.h
 * - fw_update.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Receives CRC-checked YMODEM data through UART DMA.
 *
 * Processing flow:
 *
 * Validate packets and hand payloads to the APP write task.
 * Wait for completion, then switch buffers and acknowledge data.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "ymodem.h"        /* 用户中间件层：YMODEM 接口 */

#include <stddef.h>        /* C 标准库 */

#include "usart.h"         /* Core 层：HAL 串口接口 */

#include "uart_handler.h"  /* BSP 层：接收长度队列 */

#include "FreeRTOS.h"      /* 中间件层：RTOS、协议和日志 */
#include "queue.h"
#include "semphr.h"
#include "common.h"
#include "checksum.h"
#include "debug.h"

#include "fw_update.h"       /* 业务层：固件更新任务交接 */
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define TAG                         "ymodem"  /* 日志标识 */
#define YMODEM_RX_BUFFER_SIZE       (1030U)  /* DMA 缓冲区字节数 */
#define YMODEM_PROCESS_TIMEOUT_MS   (2000U)  /* 写入任务等待毫秒数 */
#define PACKET_SEQNO_INDEX          (1U)     /* 包序号位置 */
#define PACKET_SEQNO_COMP_INDEX     (2U)     /* 序号反码位置 */
#define PACKET_HEADER               (3U)     /* 包头字节数 */
#define PACKET_TRAILER              (2U)     /* CRC 字节数 */
/* 包头与包尾的总字节数。 */
#define PACKET_OVERHEAD             (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE                 (128U)   /* 短包数据区字节数 */
#define PACKET_1K_SIZE              (1024U)  /* 长包数据区字节数 */
#define FILE_NAME_LENGTH            (256U)   /* 文件名扫描上限 */
#define FILE_SIZE_LENGTH            (16U)    /* 长度字符串缓冲区字节数 */

#define SOH                         (0x01U)  /* 短包起始符 */
#define STX                         (0x02U)  /* 长包起始符 */
#define EOT                         (0x04U)  /* 文件数据结束符 */
#define ACK                         (0x06U)  /* 接收确认 */
#define NAK                         (0x15U)  /* 请求重传 */
#define CA                          (0x18U)  /* 连续两字节取消 */
#define CRC16                       (0x43U)  /* 请求 CRC 模式的字符 C */
#define ABORT1                      (0x41U)  /* 用户终止字符 A */
#define ABORT2                      (0x61U)  /* 用户终止字符 a */

/* DMA 长度通知等待时间，单位为毫秒。 */
#define NAK_TIMEOUT                 (2000U)
#define MAX_ERRORS                  (3)      /* 连续错误超过此值时取消 */
/* 外部 A 区允许接收的最大文件字节数。 */
#define YMODEM_MAX_FILE_SIZE        ((uint32_t)FW_UPDATE_MAX_SIZE)
/****************************** Defines ******************************/

/************************ Variable Definitions ************************/
static uint8_t *g_p_ymodem_rx_buffer = NULL;  /* 当前 DMA 缓冲区 */
static uint8_t g_ymodem_rx_started = 0U;      /* 接收或通知待处理 */

int32_t g_ymodem_packet_length = 0;  /* 包长或控制标记，供写入任务使用 */
static int32_t g_ymodem_file_size = 0;  /* 超时退出期间仍有效的交接长度 */
/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/
/**
 * @brief 中止接收并清空旧长度通知。
 *
 * @return 无。
 * @note 用于会话初始化、超时恢复和退出。
 */
static void Ymodem_StopReceive(void)
{
    uint16_t discarded_length = 0U;  /* 旧长度通知。 */

    /* 1. 中止接收并清除标记。 */
    (void)HAL_UART_AbortReceive(&huart1);
    g_ymodem_rx_started = 0U;

    /* 2. 清空旧长度通知。 */
    if (NULL != g_uart_rx_length_queue)
    {
        while (pdTRUE == xQueueReceive(g_uart_rx_length_queue,
                                       &discarded_length,
                                       0U))
        {
        }
    }
}

/**
 * @brief 准备下一包的普通 DMA 接收。
 *
 * 保留已启动接收及待处理通知，重新启动时关闭 HT 中断。
 *
 * @retval 0 已启动，或已有接收等待处理。
 * @retval -1 缓冲区、队列无效，或 DMA 启动失败。
 */
static int Ymodem_StartReceive(void)
{
    /* 1. 保留接收及待处理通知。 */
    if (0U != g_ymodem_rx_started)
    {
        return 0;
    }

    if ((NULL == g_p_ymodem_rx_buffer) ||
        (NULL == g_uart_rx_length_queue))
    {
        return -1;
    }

    /* 2. 清理状态并启动 DMA。 */
    Ymodem_StopReceive();
    __HAL_UART_CLEAR_OREFLAG(&huart1);

    if (HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(
        &huart1,
        g_p_ymodem_rx_buffer,
        YMODEM_RX_BUFFER_SIZE))
    {
        Ymodem_StopReceive();
        return -1;
    }

    /* 3. 关闭半传输中断。 */
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

    g_ymodem_rx_started = 1U;
    return 0;
}

/**
 * @brief 等待一块 DMA 数据的长度通知。
 *
 * @param[in,out] p_data 共享接收缓冲区，不得为空。
 * @param[out] p_received_length 实际接收字节数，不得为空。
 * @param[in] timeout_ms 等待毫秒数。
 *
 * @retval 0 收到有效长度通知。
 * @retval -1 参数、启动、等待或长度检查失败。
 * @note 超时后清理接收，数据保留在 DMA 缓冲区。
 */
static int Ymodem_ReceiveData(uint8_t *p_data,
                              uint16_t *p_received_length,
                              uint32_t timeout_ms)
{
    if ((NULL == p_data) ||
        (NULL == p_received_length) ||
        (p_data != g_p_ymodem_rx_buffer))
    {
        return -1;
    }

    *p_received_length = 0U;

    if (0 != Ymodem_StartReceive())
    {
        return -1;
    }

    /* 等待长度通知，超时后清理接收。 */
    if (pdTRUE != xQueueReceive(g_uart_rx_length_queue,
                                p_received_length,
                                pdMS_TO_TICKS(timeout_ms)))
    {
        Ymodem_StopReceive();
        return -1;
    }

    /* 普通 DMA 已结束，检查通知长度。 */
    g_ymodem_rx_started = 0U;

    if ((0U == *p_received_length) ||
        (*p_received_length > YMODEM_RX_BUFFER_SIZE))
    {
        return -1;
    }

    return 0;
}

/**
 * @brief 准备下一包接收并发送控制字节。
 *
 * @param[in] character 待发送的控制字节。
 *
 * @retval 0 已调用发送接口。
 * @retval -1 ACK、NAK 或 C 发送前的 DMA 准备失败。
 * @note 只反馈接收准备结果，不反馈串口发送结果。
 */
static int Ymodem_SendByte(uint8_t character)
{
    /* 先准备接收，再发送应答。 */
    if ((ACK == character) ||
        (NAK == character) ||
        (CRC16 == character))
    {
        if (0 != Ymodem_StartReceive())
        {
            return -1;
        }
    }

    Common_SendByte(character);
    return 0;
}

/**
 * @brief 接收并校验一个 YMODEM 数据包。
 *
 * 检查包长、序号反码和完整数据区 CRC，包含填充。
 *
 * @param[in,out] p_data 共享接收缓冲区，不得为空。
 * @param[out] p_length 数据区长度；结束为 0，取消为 -1。
 * @param[in] timeout 等待通知的毫秒数。
 *
 * @retval 0 收到有效包、结束或取消标志。
 * @retval -1 接收、参数、格式或 CRC 检查失败。
 * @retval -2 收到用户终止字符。
 * @note 每次 IDLE/TC 按整包处理，不累计拆分包。
 */
static int Ymodem_ReceivePacket(uint8_t *p_data,
                                int32_t *p_length,
                                uint32_t timeout)
{
    uint16_t packet_size = 0U;  /* 数据区长度。 */
    uint16_t received_length = 0U;  /* 实际接收长度。 */
    uint16_t crc_calculated = 0U;  /* 本地 CRC16。 */
    uint16_t crc_received = 0U;    /* 包尾 CRC16。 */

    if ((NULL == p_data) || (NULL == p_length))
    {
        return -1;
    }

    *p_length = 0;

    /* 1. 等待数据并识别包类型。 */
    if (0 != Ymodem_ReceiveData(p_data, &received_length, timeout))
    {
        return -1;
    }

    switch (p_data[0])
    {
        case SOH:
            packet_size = PACKET_SIZE;
            break;

        case STX:
            packet_size = PACKET_1K_SIZE;
            break;

        case EOT:
            return (1U == received_length) ? 0 : -1;

        case CA:
            if ((2U == received_length) && (CA == p_data[1]))
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

    /* 2. 检查完整包长。 */
    if (received_length != (packet_size + PACKET_OVERHEAD))
    {
        return -1;
    }

    /* 3. 检查包序号反码。 */
    if (0xFFU != ((uint32_t)p_data[PACKET_SEQNO_INDEX] +
                  (uint32_t)p_data[PACKET_SEQNO_COMP_INDEX]))
    {
        return -1;
    }

    /* 4. 校验完整数据区，包含填充。 */
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
int32_t Ymodem_Receive(uint8_t *p_buffer_1, uint8_t *p_buffer_2)
{
    uint8_t *p_packet_data = p_buffer_1;  /* 当前包缓冲区。 */
    uint8_t file_size[FILE_SIZE_LENGTH] = {0U};  /* 文件长度字符串。 */
    uint8_t *p_file = NULL;  /* 文件信息位置。 */

    uint32_t index = 0U;  /* 字段遍历位置。 */
    int32_t session_done = 0;  /* 会话结束标记。 */
    int32_t file_done = 0;  /* 当前文件结束标记。 */
    int32_t packets_received = 0;  /* 下一个期望包的计数。 */
    int32_t errors = 0;  /* 连续错误计数。 */
    int32_t session_begin = 0;  /* 会话开始标记。 */
    int32_t size = 0;  /* 声明的文件字节数。 */
    uint32_t bytes_received = 0U;  /* 已处理的有效字节数。 */

    if ((NULL == p_buffer_1) || (NULL == p_buffer_2) ||
        (p_buffer_1 == p_buffer_2) ||
        (NULL == g_uart_rx_length_queue) ||
        (NULL == g_fw_data_queue) ||
        (NULL == g_fw_write_done_semaphore))
    {
        return -1;
    }

    /* 1. 清理会话并请求首包。 */
    Ymodem_StopReceive();
    g_p_ymodem_rx_buffer = p_buffer_1;
    if (0 != Ymodem_SendByte(CRC16))
    {
        Ymodem_StopReceive();
        return -1;
    }

    /* 2. 逐包处理，空文件信息包结束会话。 */
    for (session_done = 0, errors = 0, session_begin = 0; ;)
    {
        for (packets_received = 0, file_done = 0; ;)
        {
            switch (Ymodem_ReceivePacket(p_packet_data,
                                         &g_ymodem_packet_length,
                                         NAK_TIMEOUT))
            {
                case 0:
                    switch (g_ymodem_packet_length)
                    {
                        /* 发送端取消。 */
                        case -1:
                            if (0 != Ymodem_SendByte(ACK))
                            {
                                Ymodem_StopReceive();
                                return -1;
                            }
                            Ymodem_StopReceive();
                            return -2;

                        /* 数据结束，核对长度并请求结束包。 */
                        case 0:
                            errors = 0;
                            if ((size <= 16) ||
                                (bytes_received != (uint32_t)size) ||
                                (0U != g_fw_write_fault))
                            {
                                Common_SendByte(CA);
                                Common_SendByte(CA);
                                Ymodem_StopReceive();
                                return -1;
                            }
                            if ((0 != Ymodem_SendByte(ACK)) ||
                                (0 != Ymodem_SendByte(CRC16)))
                            {
                                Ymodem_StopReceive();
                                return -1;
                            }
                            file_done = 1;
                            break;

                        default:
                            if ((packets_received > 0) &&
                                (p_packet_data[PACKET_SEQNO_INDEX] ==
                                 ((packets_received - 1) & 0xFFU)))
                            {
                                errors = 0;
                                /* 重传包只确认，不重复交给写入任务。 */
                                if (0 != Ymodem_SendByte(ACK))
                                {
                                    Ymodem_StopReceive();
                                    return -1;
                                }
                                if (1 == packets_received)
                                {
                                    if (0 != Ymodem_SendByte(CRC16))
                                    {
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                }
                                break;
                            }
                            else if ((p_packet_data[PACKET_SEQNO_INDEX] &
                                      0xFFU) !=
                                     (packets_received & 0xFFU))
                            {
                                /* 错序与接收失败共用错误计数。 */
                                errors++;
                                if (errors > MAX_ERRORS)
                                {
                                    Common_SendByte(CA);
                                    Common_SendByte(CA);
                                    Ymodem_StopReceive();
                                    return -4;
                                }
                                if (0 != Ymodem_SendByte(NAK))
                                {
                                    Ymodem_StopReceive();
                                    return -1;
                                }
                                session_begin = 1;
                                break;
                            }
                            else if (0 == packets_received)
                            {
                                errors = 0;
                                /* 第 0 包携带文件信息。 */
                                if (0U != p_packet_data[PACKET_HEADER])
                                {
                                    /* 一次 OTA 只接收一个文件。 */
                                    if (size > 0)
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    p_file = &p_packet_data[PACKET_HEADER];

                                    /* 跳过文件名。 */
                                    for (index = 0;
                                         (p_file < p_packet_data +
                                                    PACKET_HEADER +
                                                   g_ymodem_packet_length) &&
                                         (0U != *p_file) &&
                                         (index < FILE_NAME_LENGTH);
                                         index++)
                                    {
                                        p_file++;
                                    }

                                    if ((p_file >= p_packet_data +
                                                    PACKET_HEADER +
                                                   g_ymodem_packet_length) ||
                                        (0U != *p_file))
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }

                                    p_file++;

                                    /* 复制长度字段并补空字符。 */
                                    for (index = 0;
                                         (p_file < p_packet_data +
                                                    PACKET_HEADER +
                                                   g_ymodem_packet_length) &&
                                         (' ' != *p_file) &&
                                         ('\0' != *p_file) &&
                                         (index <
                                          (FILE_SIZE_LENGTH - 1U));
                                         index++)
                                    {
                                        file_size[index] = *p_file++;
                                    }
                                    file_size[index] = '\0';

                                    if ((p_file >= p_packet_data +
                                                    PACKET_HEADER +
                                                   g_ymodem_packet_length) ||
                                        (0 != Common_StringToInt(file_size,
                                                                 &size)))
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }

                                    /* 检查加密包长度、容量和对齐。 */
                                    if ((size <= 16) ||
                                        ((uint32_t)size >
                                         YMODEM_MAX_FILE_SIZE) ||
                                        (0U != ((uint32_t)size % 16U)))
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }

                                    /* 长度副本在接收返回后仍保持有效。 */
                                    g_ymodem_file_size = size;
                                    int32_t *p_size = &g_ymodem_file_size;
                                    if (pdPASS != xQueueSend(
                                        g_fw_data_queue,
                                        &p_size,
                                        0U))
                                    {
                                        elog_e(
                                            TAG,
                                            "file size queue send failed: "
                                            "size=%ld",
                                            (long)size);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    /* 等待文件准备完成。 */
                                    if (pdTRUE != xSemaphoreTake(
                                        g_fw_write_done_semaphore,
                                        pdMS_TO_TICKS(
                                            YMODEM_PROCESS_TIMEOUT_MS)))
                                    {
                                        elog_e(
                                            TAG,
                                            "file preparation timeout: "
                                            "size=%ld",
                                            (long)size);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    if (0U != g_fw_write_fault)
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    if (p_packet_data == p_buffer_1)
                                    {
                                        p_packet_data = p_buffer_2;
                                    }
                                    else
                                    {
                                        p_packet_data = p_buffer_1;
                                    }
                                    g_p_ymodem_rx_buffer = p_packet_data;

                                    if ((0 != Ymodem_SendByte(ACK)) ||
                                        (0 != Ymodem_SendByte(CRC16)))
                                    {
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                }
                                else
                                {
                                    if ((size <= 16) ||
                                        (bytes_received != (uint32_t)size) ||
                                        (0U != g_fw_write_fault))
                                    {
                                        Common_SendByte(CA);
                                        Common_SendByte(CA);
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    if (0 != Ymodem_SendByte(ACK))
                                    {
                                        Ymodem_StopReceive();
                                        return -1;
                                    }
                                    file_done = 1;
                                    session_done = 1;  /* 空文件名结束会话。 */
                                    break;
                                }
                            }
                            else
                            {
                                /* 本包有效文件字节数。 */
                                uint32_t write_length = 0U;

                                errors = 0;
                                if (bytes_received >= (uint32_t)size)
                                {
                                    Common_SendByte(CA);
                                    Common_SendByte(CA);
                                    Ymodem_StopReceive();
                                    return -1;
                                }
                                /* 末包只保存有效字节。 */
                                write_length = (uint32_t)size - bytes_received;
                                if (write_length >
                                    (uint32_t)g_ymodem_packet_length)
                                {
                                    write_length =
                                        (uint32_t)g_ymodem_packet_length;
                                }
                                g_ymodem_packet_length = (int32_t)write_length;
                                /* 交接已校验的数据区。 */
                                uint8_t *p_payload =
                                    p_packet_data + PACKET_HEADER;
                                if (pdPASS != xQueueSend(
                                    g_fw_data_queue,
                                    &p_payload,
                                    0U))
                                {
                                    elog_e(TAG,
                                           "data queue send failed: "
                                           "packet=%ld, data=%p",
                                           (long)packets_received,
                                           (void *)p_payload);

                                    Ymodem_StopReceive();
                                    return -1;
                                }

                                /* 等待处理完成，再切换缓冲区并应答。 */
                                if (pdTRUE != xSemaphoreTake(
                                    g_fw_write_done_semaphore,
                                    pdMS_TO_TICKS(YMODEM_PROCESS_TIMEOUT_MS)))
                                {
                                    elog_e(TAG,
                                           "data processing timeout: "
                                           "packet=%ld",
                                           (long)packets_received);

                                    Ymodem_StopReceive();
                                    return -1;
                                }

                                if (0U != g_fw_write_fault)
                                {
                                    Common_SendByte(CA);
                                    Common_SendByte(CA);
                                    Ymodem_StopReceive();
                                    return -1;
                                }
                                bytes_received += write_length;

                                if (p_packet_data == p_buffer_1)
                                {
                                    p_packet_data = p_buffer_2;
                                }
                                else
                                {
                                    p_packet_data = p_buffer_1;
                                }
                                g_p_ymodem_rx_buffer = p_packet_data;
                                if (0 != Ymodem_SendByte(ACK))
                                {
                                    Ymodem_StopReceive();
                                    return -1;
                                }
                            }

                            packets_received++;
                            session_begin = 1;
                            break;
                    }
                    break;

                /* 用户终止会话。 */
                case -2:
                    Common_SendByte(CA);
                    Common_SendByte(CA);
                    Ymodem_StopReceive();
                    return -3;

                default:
                    /* 会话开始后累计接收错误。 */
                    if (session_begin > 0)
                    {
                        errors++;
                    }

                    if (errors > MAX_ERRORS)
                    {
                        Common_SendByte(CA);
                        Common_SendByte(CA);
                        Ymodem_StopReceive();
                        return -4;
                    }

                    if (0 != Ymodem_SendByte(CRC16))
                    {
                        Ymodem_StopReceive();
                        return -1;
                    }
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

    /* 3. 停止接收并返回文件长度。 */
    Ymodem_StopReceive();
    return size;
}

/************************ Function Definitions ************************/
