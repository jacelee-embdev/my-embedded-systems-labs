/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file iwdg.c
 *
 * @par dependencies
 * - iwdg.h
 * - stm32f4xx_iwdg.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Separates IWDG configuration from timed startup.
 *
 * Processing flow:
 *
 * Call IWDG_Init() during peripheral initialization to select divide-by-64
 * without starting the watchdog. Call IWDG_Start() with a timeout in ms
 * before the first APP launch. Start IWDG, wait for register updates,
 * and reload the counter. The APP confirmation flow waits for a reset.
 *
 * @version V1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "iwdg.h"  /* 用户 Core 层：独立看门狗模块接口 */

#include "stm32f4xx_iwdg.h"  /* 厂商 SPL 层：IWDG 驱动接口 */

/****************************** Includes ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 配置独立看门狗为固定 64 分频，不启动计时。
 *
 * 允许寄存器写入后设置预分频，供后续 IWDG_Start() 使用。
 *
 * @return 无返回值。
 *
 * @note 在 IWDG 未启动时调用；不等待分频寄存器更新完成。
 * @note 按软件启动模式使用，硬件自动启动模式不保证此时未计时。
 */
void IWDG_Init(void)
{
    /* 允许配置寄存器写入，仅设置固定分频。 */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
}

/**
 * @brief 按指定毫秒数配置重装载值，并启动独立看门狗。
 *
 * 处理步骤：
 * 1. 按固定 64 分频和 LSI 典型频率 32 kHz 计算重装载值。
 * 2. 写入重装载值并启动 IWDG，由硬件自动开启 LSI。
 * 3. 等待分频及重装载寄存器更新完成，再重装载计数器。
 *
 * @param[in] timeout_ms : 标称超时时间，单位为毫秒，范围 2 至 8192。
 *                        奇数毫秒向上取整至下一个 2 ms。
 *
 * @return 无返回值。
 *
 * @note 须先调用 IWDG_Init()；本接口不校验参数或调用顺序。
 * @note 实际超时时间随 LSI 频率变化；等待寄存器更新不设超时。
 * @note 启动后无法通过软件关闭，须经芯片复位停止。
 */
void IWDG_Start(uint32_t timeout_ms)
{
    /* 1. 每个计数周期标称 2 ms，向上取整后减去计数起始偏移。 */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetReload((uint16_t)(((timeout_ms + 1U) / 2U) - 1U));

    /* 2. 启动看门狗，硬件自动开启 LSI。 */
    IWDG_Enable();

    /* 3. 等待配置同步，再从指定重装载值开始倒计时。 */
    while (0U != IWDG->SR)
    {
        /* 等待 PVU 和 RVU 更新标志清零。 */
    }

    IWDG_ReloadCounter();
}

/************************ Function Definitions ************************/
