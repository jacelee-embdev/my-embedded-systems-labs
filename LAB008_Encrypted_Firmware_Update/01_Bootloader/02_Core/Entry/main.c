/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file main.c
 *
 * @par dependencies
 * - main.h
 * - stdint.h
 * - delay.h
 * - usart.h
 * - spi.h
 * - iwdg.h
 * - key.h
 * - led.h
 * - w25qxx_handler.h
 * - at24cxx.h
 * - debug.h
 * - bootloader.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Handles retained jump requests and Bootloader startup.
 *
 * Processing flow:
 *
 * Consume a retained jump request before peripheral initialization.
 * Initialize peripherals and configure IWDG without starting it.
 * Initialize the EEPROM bus through AT24Cxx_Init().
 * Initialize services, print startup messages with 100 ms intervals,
 * and process the EEPROM state. The first-boot trial branch starts IWDG.
 * Wait for a key to retry downloads if state processing returns.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/******************************* Includes *******************************/

#include "main.h"  /* 本模块接口 */

#include <stdint.h>  /* C 标准库 */

#include "delay.h"  /* Core 层 */
#include "usart.h"
#include "spi.h"
#include "iwdg.h"

#include "key.h"  /* BSP 层 */
#include "led.h"
#include "w25qxx_handler.h"
#include "at24cxx.h"

#include "debug.h"  /* 用户中间件层：日志封装接口 */

#include "bootloader.h"  /* 业务层 */

/******************************* Includes *******************************/

/******************************* Defines *******************************/

#define TAG "main"

/******************************* Defines *******************************/

/************************* Variable Definitions *************************/

/* 状态机写入、main() 消费的跳转请求，位于 RAM 0x2001FFF0。 */
/* NO_INIT 映射到 UNINIT 区域，复位后保留，不设置初值。 */
volatile uint32_t g_app_jump_request
    __attribute__((section("NO_INIT"), zero_init));

/************************* Variable Definitions *************************/

/************************* Function Definitions *************************/

/**
 * @brief 消费复位请求，初始化服务并处理启动及下载。
 *
 * 处理步骤：
 * 1. 消费保留 RAM 中的一次性请求，尝试提前跳转 APP。
 * 2. 初始化外设、日志和存储服务，仅配置 IWDG 分频。
 *    EEPROM 总线由 AT24Cxx_Init() 初始化。
 * 3. 输出启动日志，按 EEPROM 状态处理启动、安装或恢复。
 * 4. 状态处理返回后等待按键，重试完整下载升级流程。
 *
 * @return 正常运行时不返回。
 *
 * @note 请求检查位于 SystemInit() 和 C 运行库初始化之后。
 * @note IWDG_Init() 不启动计时，由业务层首次试运行分支启动。
 */
int main(void)
{
    /* 消费一次性跳转请求；先清除，避免下次复位重复命中。 */
    if (APP_JUMP_REQUEST_MAGIC == g_app_jump_request)
    {
        g_app_jump_request = 0U;
        Bootloader_JumpToApp();
    }

    /* 初始化外设、日志和存储服务；IWDG 此时仅配置分频。 */
    Delay_Init();
    USART_InitUsart1();
    SPI_InitSpi1();
    IWDG_Init();
    debug_init();
    Key_Init();
    LED_Init();
    AT24Cxx_Init();
    W25QxxHandler_Init();

    /* 输出 5 次启动日志，每次延时 100 ms。 */
    for (uint8_t startup_log_index = 0U; startup_log_index < 5;
         startup_log_index++)
    {
        elog_i(TAG, "Bootloader running...\r\n");
        Delay_Ms(100);
    }

    /* 按 EEPROM 状态处理启动、安装或恢复。 */
    Bootloader_ProcessState();

    elog_e(TAG, "APP unavailable/unconfirmed. Press key to download.\r\n");
    /* 启动流程返回后等待按键，每次重新执行完整升级。 */
    while (1)
    {
        if (KEY_PRESSED == Key_Scan())
        {
            Bootloader_DownloadApp();
            elog_e(TAG, "APP update returned. Press key to retry.\r\n");
        }
        Delay_Ms(50);
    }
}

/************************* Function Definitions *************************/
