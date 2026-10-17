/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file fw_update.c
 *
 * @par dependencies
 * - fw_update.h
 * - string.h
 * - usart.h
 * - uart_handler.h
 * - w25qxx_handler.h
 * - at24cxx.h
 * - key.h
 * - FreeRTOS.h
 * - task.h
 * - queue.h
 * - semphr.h
 * - ymodem.h
 * - debug.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Receives firmware and coordinates installation confirmation.
 *
 * Processing flow:
 *
 * Accept a download command and mark the package incomplete.
 * Receive encrypted data into A through a synchronized write task.
 * Verify the length, commit metadata, and wait for installation confirmation.
 * Stop the write task cooperatively before releasing download resources.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "fw_update.h"       /* 业务层：固件更新接口 */

#include <string.h>          /* C 标准库 */

#include "usart.h"           /* Core 层：HAL 串口配置 */

#include "uart_handler.h"    /* BSP 层：串口、Flash、EEPROM 与按键 */
#include "w25qxx_handler.h"
#include "at24cxx.h"
#include "key.h"

#include "FreeRTOS.h"        /* 中间件层：RTOS、传输与日志封装 */
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ymodem.h"
#include "debug.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define TAG  "fw_update"                   /* 日志标识 */
#define FW_UPDATE_RX_BUFFER_SIZE  (1030U)  /* 单包接收容量，单位为字节 */

/* 线程运行状态，与 EEPROM 启动标志分开管理。 */
typedef enum
{
    FW_STATE_WAIT_DOWNLOAD_CMD = 0,  /* 等待下载命令 */
    FW_STATE_RECEIVE_AND_COMPLETE,   /* 接收并完成包提交 */
    FW_STATE_WAIT_INSTALL_CMD,       /* 等待安装命令 */
    FW_STATE_WAIT_KEY_CONFIRM        /* 等待按键确认 */
} fw_update_state_t;

/****************************** Defines ******************************/

/************************ Variable Definitions ************************/

/* 当前线程状态。 */
static fw_update_state_t g_fw_state = FW_STATE_WAIT_DOWNLOAD_CMD;
static uint8_t g_fw_command_buffer[4] = {0U};  /* 命令接收缓冲区 */
static uint8_t g_fw_command_rx_started = 0U;  /* 接收或通知待处理 */

/* 双缓冲交替接收；写任务停止前不复用本次数据。 */
static uint8_t g_fw_receive_buffer_a[FW_UPDATE_RX_BUFFER_SIZE];
static uint8_t g_fw_receive_buffer_b[FW_UPDATE_RX_BUFFER_SIZE];

static TaskHandle_t g_fw_write_task_handle = NULL;  /* 写入任务句柄 */
/* 写任务已停止访问会话资源的独立确认。 */
static SemaphoreHandle_t g_fw_write_stopped_semaphore = NULL;
static volatile uint8_t g_fw_write_stop_requested = 0U;  /* 停止请求 */

QueueHandle_t g_fw_data_queue = NULL;  /* 文件长度及数据指针队列 */
SemaphoreHandle_t g_fw_write_done_semaphore = NULL;  /* 分包完成通知 */
volatile uint8_t g_fw_write_fault = 0U;  /* 本次写入故障标记 */

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 轮询 EEPROM 应答，等待内部写周期结束。
 * @retval 0 器件已就绪。
 * @retval -1 等待超时。
 * @note 每次未应答后延时 1 ms，等待窗口为 10 ms。
 */
static int FWUpdate_WaitWriteComplete(void)
{
    uint32_t start_tick = HAL_GetTick();  /* 轮询起点 */

    do
    {
        if (0 == AT24Cxx_CheckDevice())
        {
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    while ((uint32_t)(HAL_GetTick() - start_tick) < 10U);

    return -1;
}

/**
 * @brief 写入 EEPROM 状态，等待就绪后读回比较。
 * @param[in] app_state 与 Bootloader 约定的状态字节。
 * @retval 0 写入及校验成功。
 * @retval -1 写入、等待、读取或比较失败。
 */
static int FWUpdate_SaveState(uint8_t app_state)
{
    uint8_t read_state = APP_STATE_NORMAL_BOOT;  /* 回读状态 */

    if ((0 != AT24Cxx_WriteBytes(&app_state, 0x00U, 1U)) ||
        (0 != FWUpdate_WaitWriteComplete()) ||
        (0 != AT24Cxx_ReadBytes(&read_state, 0x00U, 1U)) ||
        (read_state != app_state))
    {
        elog_e(TAG, "EEPROM state save/verify failed: 0x%02X",
               (unsigned int)app_state);
        return -1;
    }
    return 0;
}

/**
 * @brief 写入并校验包长度，再提交待安装标志。
 * @param[in] firmware_size 完整加密包的字节数。
 * @retval 0 长度和状态均已确认。
 * @retval -1 任一写入、等待、读取或比较失败。
 */
static int FWUpdate_SaveMetadata(uint32_t firmware_size)
{
    uint32_t read_size = 0U;  /* 回读包长度 */

    /* 长度先确认，待安装标志最后提交。 */
    if ((0 != AT24Cxx_WriteBytes((const uint8_t *)&firmware_size, 0x01U,
                                sizeof(firmware_size))) ||
        (0 != FWUpdate_WaitWriteComplete()) ||
        (0 != AT24Cxx_ReadBytes((uint8_t *)&read_size, 0x01U,
                               sizeof(read_size))) ||
        (read_size != firmware_size))
    {
        elog_e(TAG, "EEPROM firmware size save/verify failed");
        return -1;
    }
    return FWUpdate_SaveState(APP_STATE_INSTALL_PENDING);
}

/**
 * @brief 保留已有命令接收，或重新启动串口空闲 DMA 接收。
 * @retval 0 接收已准备，或已有通知等待处理。
 * @retval -1 句柄无效、中止失败或 DMA 启动失败。
 * @note 重新启动前取空通知并清理缓冲区及溢出标志。
 */
static int FWUpdate_StartCommandReceive(void)
{
    uint16_t received_length = 0U;  /* 已接收长度通知 */

    if ((NULL == g_uart_rx_length_queue) || (NULL == huart1.hdmarx))
    {
        elog_e(TAG, "OTA command RX handle invalid");
        return -1;
    }
    if (0U != g_fw_command_rx_started)
    {
        return 0;
    }
    if (HAL_OK != HAL_UART_AbortReceive(&huart1))
    {
        elog_e(TAG, "OTA command RX abort failed");
        return -1;
    }
    while (pdTRUE == xQueueReceive(g_uart_rx_length_queue,
                                   &received_length, 0U))
    {
    }
    memset(g_fw_command_buffer, 0, sizeof(g_fw_command_buffer));
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    if (HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(&huart1,
                                              g_fw_command_buffer,
                                              sizeof(g_fw_command_buffer)))
    {
        elog_e(TAG, "OTA command RX start failed");
        return -1;
    }
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    g_fw_command_rx_started = 1U;
    return 0;
}

/**
 * @brief 等待接收通知并匹配三字节命令。
 * @param[in] p_command 期望命令，须可读取三个字节。
 * @retval 0 收到完整匹配的命令。
 * @retval -1 参数、接收准备、通知、长度或内容不符。
 * @note 无限等待通知；取到通知后清除接收标记。
 */
static int FWUpdate_ReceiveCommand(const uint8_t *p_command)
{
    uint16_t received_length = 0U;  /* 队列通知的接收长度 */

    if ((NULL == p_command) || (0 != FWUpdate_StartCommandReceive()))
    {
        return -1;
    }
    if ((pdTRUE != xQueueReceive(g_uart_rx_length_queue, &received_length,
                                 portMAX_DELAY)) ||
        (3U != received_length))
    {
        g_fw_command_rx_started = 0U;
        return -1;
    }
    g_fw_command_rx_started = 0U;
    return (0 == memcmp(g_fw_command_buffer, p_command, 3U)) ? 0 : -1;
}

/**
 * @brief 扫描按键，等待安装确认。
 * @retval 0 检测到有效按键。
 * @retval -1 扫描轮数耗尽。
 * @note 最多 400 轮，未确认后延时 50 ms；另有消抖和调度耗时。
 */
static int FWUpdate_WaitKeyConfirm(void)
{
    for (uint16_t count = 0U; count < 400U; count++)
    {
        if (KEY_PRESSED == Key_Scan())
        {
            return 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50U));
    }
    return -1;
}

/**
 * @brief 读取文件长度及数据指针，将有效数据写入 A 区。
 *
 * 校验首条长度消息，逐包写入并通知结果。
 * 收到停止请求后等待当前 Flash 调用返回，再确认停止并挂起。
 *
 * @param[in] p_argument 线程参数，当前未使用，允许为空。
 * @return 无；停止后由主控线程删除。
 */
static void FWUpdate_WriteTaskEntry(void *p_argument)
{
    uint8_t *p_data = NULL;  /* 当前数据区 */
    int32_t *p_file_size = NULL;  /* 首条消息的稳定长度存储 */
    uint32_t remaining_size = 0U;  /* 剩余有效字节数 */
    uint32_t write_length = 0U;  /* 当前写入长度 */
    BaseType_t receive_status = pdFALSE;  /* 队列接收结果 */
    (void)p_argument;

    /* 首条消息为文件长度；停止哨兵也可唤醒此处。 */
    receive_status = xQueueReceive(g_fw_data_queue, &p_file_size,
                                   portMAX_DELAY);
    if (0U == g_fw_write_stop_requested)
    {
        if ((pdTRUE != receive_status) || (NULL == p_file_size) ||
            (*p_file_size <= 16) ||
            ((uint32_t)*p_file_size > FW_UPDATE_MAX_SIZE) ||
            (0U != ((uint32_t)*p_file_size % 16U)))
        {
            g_fw_write_fault = 1U;
        }
        else
        {
            remaining_size = (uint32_t)*p_file_size;
        }
        if (pdTRUE != xSemaphoreGive(g_fw_write_done_semaphore))
        {
            elog_e(TAG, "File preparation signal failed");
        }
    }

    while (0U == g_fw_write_stop_requested)
    {
        receive_status = xQueueReceive(g_fw_data_queue, &p_data,
                                       portMAX_DELAY);
        if (0U != g_fw_write_stop_requested)
        {
            break;
        }
        if (pdTRUE != receive_status)
        {
            continue;
        }
        if ((NULL == p_data) || (g_ymodem_packet_length <= 0) ||
            ((uint32_t)g_ymodem_packet_length > remaining_size))
        {
            g_fw_write_fault = 1U;
        }
        else if (0U == g_fw_write_fault)
        {
            write_length = (uint32_t)g_ymodem_packet_length;
            if (W25QXX_HANDLER_OK != W25QxxHandler_WriteData(
                    W25QXX_HANDLER_REGION_A, p_data, write_length))
            {
                g_fw_write_fault = 1U;
            }
            else
            {
                remaining_size -= write_length;
            }
        }
        /* 停止时跳过分包通知，转而发送独立退出确认。 */
        if (0U != g_fw_write_stop_requested)
        {
            break;
        }
        if (pdTRUE != xSemaphoreGive(g_fw_write_done_semaphore))
        {
            elog_e(TAG, "Data completion signal failed");
        }
    }

    /* 确认后不再访问本次数据及完成信号量。 */
    if (pdTRUE != xSemaphoreGive(g_fw_write_stopped_semaphore))
    {
        elog_e(TAG, "Write task stop signal failed");
    }
    for (;;)
    {
        vTaskSuspend(NULL);
    }
}

/**
 * @brief 请求写任务停止，确认后释放下载资源。
 * @return 无。
 * @note 等待当前 Flash 调用返回；不因等待超时强制删除写任务。
 */
static void FWUpdate_ReleaseResources(void)
{
    uint8_t *p_stop = NULL;  /* 唤醒队列等待的停止哨兵 */

    if (NULL != g_fw_write_task_handle)
    {
        g_fw_write_stop_requested = 1U;
        if (pdPASS != xQueueSend(g_fw_data_queue, &p_stop, 0U))
        {
            /* 队列已满时，写任务出队后也会检查停止请求。 */
        }
        while (pdTRUE != xSemaphoreTake(g_fw_write_stopped_semaphore,
                                        portMAX_DELAY))
        {
        }
        vTaskDelete(g_fw_write_task_handle);
        g_fw_write_task_handle = NULL;
    }
    if (NULL != g_fw_data_queue)
    {
        vQueueDelete(g_fw_data_queue);
        g_fw_data_queue = NULL;
    }
    if (NULL != g_fw_write_done_semaphore)
    {
        vSemaphoreDelete(g_fw_write_done_semaphore);
        g_fw_write_done_semaphore = NULL;
    }
    if (NULL != g_fw_write_stopped_semaphore)
    {
        vSemaphoreDelete(g_fw_write_stopped_semaphore);
        g_fw_write_stopped_semaphore = NULL;
    }
    g_fw_write_stop_requested = 0U;
}

/**
 * @brief 标记下载未完成，重置 A 区并创建下载资源。
 * @retval 0 状态已确认，资源创建完成。
 * @retval -1 状态保存或资源创建失败。
 * @note 创建失败时仅释放已经创建的资源。
 */
static int FWUpdate_PrepareDownload(void)
{
    if (0 != FWUpdate_SaveState(APP_STATE_DOWNLOAD_INCOMPLETE))
    {
        return -1;
    }
    W25QxxHandler_ResetRegion(W25QXX_HANDLER_REGION_A);
    g_fw_write_fault = 0U;
    g_fw_write_stop_requested = 0U;
    g_fw_write_done_semaphore = xSemaphoreCreateBinary();
    if (NULL != g_fw_write_done_semaphore)
    {
        g_fw_write_stopped_semaphore = xSemaphoreCreateBinary();
    }
    if (NULL != g_fw_write_stopped_semaphore)
    {
        g_fw_data_queue = xQueueCreate(2U, sizeof(uint8_t *));
    }
    if ((NULL == g_fw_write_done_semaphore) ||
        (NULL == g_fw_write_stopped_semaphore) ||
        (NULL == g_fw_data_queue) ||
        (pdPASS != xTaskCreate(FWUpdate_WriteTaskEntry, "fw_write_task",
                               512U, NULL, 1U, &g_fw_write_task_handle)))
    {
        elog_e(TAG, "OTA download resource creation failed");
        FWUpdate_ReleaseResources();
        return -1;
    }
    return 0;
}

/**
 * @brief 屏蔽异常并请求系统软复位。
 * @return 无；复位后由 Bootloader 处理 EEPROM 状态。
 */
void FWUpdate_SoftReset(void)
{
    __set_FAULTMASK(1U);
    NVIC_SystemReset();
}

/**
 * @brief 接收加密包，并等待安装命令和按键确认。
 *
 * 将包写入 A 区并校验长度，再提交 EEPROM 长度和待安装标志。
 * 完成应答前先准备命令接收，收到安装命令后等待按键复位。
 * 下载失败或按键超时后重新等待下载；按键超时保留待安装标志。
 *
 * @param[in] p_argument 线程参数，当前未使用，允许为空。
 * @return 无；线程持续运行，或复位进入 Bootloader。
 * @note 调度器运行后调用；启动状态清除失败仅记录错误。
 */
void FWUpdate_TaskEntry(void *p_argument)
{
    /* 下载、安装命令及完成应答，均保持原协议字节。 */
    const uint8_t download_cmd[3] = {0x11U, 0x22U, 0x33U};
    const uint8_t install_cmd[3] = {0x77U, 0x88U, 0x99U};
    uint8_t ack_cmd[3] = {0x44U, 0x55U, 0x66U};
    int32_t receive_result = 0;  /* YMODEM 文件长度或负错误值 */
    uint8_t download_valid = 0U;  /* 数据和元数据均已确认 */
    (void)p_argument;

    if (0 != FWUpdate_SaveState(APP_STATE_NORMAL_BOOT))
    {
        elog_e(TAG, "Failed to clear startup OTA state");
    }
    for (;;)
    {
        switch (g_fw_state)
        {
            case FW_STATE_WAIT_DOWNLOAD_CMD:
                if ((0 == FWUpdate_ReceiveCommand(download_cmd)) &&
                    (0 == FWUpdate_PrepareDownload()))
                {
                    g_fw_state = FW_STATE_RECEIVE_AND_COMPLETE;
                }
                break;

            case FW_STATE_RECEIVE_AND_COMPLETE:
                receive_result = Ymodem_Receive(g_fw_receive_buffer_a,
                                                g_fw_receive_buffer_b);
                download_valid = 0U;
                if ((receive_result > 16) &&
                    ((uint32_t)receive_result <= FW_UPDATE_MAX_SIZE) &&
                    (0U == ((uint32_t)receive_result % 16U)) &&
                    (0U == g_fw_write_fault) &&
                    (W25QXX_HANDLER_OK ==
                     W25QxxHandler_Flush(W25QXX_HANDLER_REGION_A)) &&
                    ((uint32_t)receive_result == W25QxxHandler_GetWrittenSize(
                        W25QXX_HANDLER_REGION_A)))
                {
                    download_valid = (0 == FWUpdate_SaveMetadata(
                        (uint32_t)receive_result)) ? 1U : 0U;
                }
                FWUpdate_ReleaseResources();
                if (0U == download_valid)
                {
                    elog_e(TAG, "OTA download/commit failed: result=%ld",
                           (long)receive_result);
                    /* 尝试作废；失败时仍按原流程等待重新下载。 */
                    if (0 != FWUpdate_SaveState(APP_STATE_DOWNLOAD_INCOMPLETE))
                    {
                        elog_e(TAG, "Failed to invalidate OTA metadata");
                    }
                    g_fw_state = FW_STATE_WAIT_DOWNLOAD_CMD;
                    break;
                }

                g_fw_state = FW_STATE_WAIT_INSTALL_CMD;
                elog_i(TAG, "OTA ready: size=%ld, EEPROM state=0x22",
                       (long)receive_result);
                vTaskDelay(pdMS_TO_TICKS(200U));
                /* 先接收再应答，保留上位机提前发来的安装命令。 */
                if (0 != FWUpdate_StartCommandReceive())
                {
                    if (0 != FWUpdate_SaveState(APP_STATE_DOWNLOAD_INCOMPLETE))
                    {
                        elog_e(TAG, "Failed to invalidate OTA metadata");
                    }
                    g_fw_state = FW_STATE_WAIT_DOWNLOAD_CMD;
                    break;
                }
                if (HAL_OK != HAL_UART_Transmit(&huart1, ack_cmd, 3U, 1000U))
                {
                    elog_e(TAG, "OTA reply 44 55 66 transmit failed");
                }
                break;

            case FW_STATE_WAIT_INSTALL_CMD:
                if (0 == FWUpdate_ReceiveCommand(install_cmd))
                {
                    g_fw_state = FW_STATE_WAIT_KEY_CONFIRM;
                }
                break;

            case FW_STATE_WAIT_KEY_CONFIRM:
                if (0 == FWUpdate_WaitKeyConfirm())
                {
                    FWUpdate_SoftReset();
                }
                else
                {
                    elog_w(TAG, "OTA key wait timed out; download again");
                    g_fw_state = FW_STATE_WAIT_DOWNLOAD_CMD;
                }
                break;

            default:
                g_fw_state = FW_STATE_WAIT_DOWNLOAD_CMD;
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(10U));
    }
}

/************************ Function Definitions ************************/
