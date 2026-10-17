/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file delay.c
 *
 * @par dependencies
 * - delay.h
 * - stm32f4xx.h
 * - system_stm32f4xx.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements blocking microsecond, millisecond, and second delays.
 *
 * Processing flow:
 *
 * Call Delay_Init() once before using the delay APIs. Call Delay_Tick() from
 * the 1 ms SysTick interrupt handler.
 *
 * @version V1.0 2026-10-14
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/

#include "delay.h"  /* 用户 Core 层：阻塞延时接口 */

#include "stm32f4xx.h"  /* 厂商 CMSIS 层：内核寄存器和系统时钟接口 */
#include "system_stm32f4xx.h"

/****************************** Includes ******************************/

/****************************** Defines ******************************/

#define DELAY_US_PER_SECOND  1000000U  /* 每秒的微秒数 */
#define DELAY_MS_PER_SECOND  1000U     /* 每秒的毫秒数 */

/****************************** Defines ******************************/

/************************ Variable Definitions ************************/

static __IO uint32_t g_ms_left  = 0U;    /* 剩余毫秒数，由 SysTick 递减 */
static uint32_t g_cycles_per_us = 0U;    /* 每微秒的内核周期数 */

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 初始化 DWT 和 SysTick 延时时基。
 *
 * 处理步骤：
 * 1. 更新内核时钟值并清零毫秒延时计数。
 * 2. 计算每微秒周期数，开启并清零 DWT 周期计数器。
 * 3. 配置 SysTick，产生 1 ms 节拍中断。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 使用延时接口前须调用；本函数会重新配置 SysTick。
 * @note 每次 1 ms SysTick 中断须调用一次 Delay_Tick()。
 */
void Delay_Init(void)
{
    /* 1. 更新延时时基使用的内核时钟值。 */
    SystemCoreClockUpdate();

    /* 2. 清零剩余毫秒数，并计算每微秒的内核周期数。 */
    g_ms_left = 0U;
    g_cycles_per_us = SystemCoreClock / DELAY_US_PER_SECOND;

    /* 3. 开启并清零 DWT 周期计数器。 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    /* 4. 配置 SysTick，产生 1 ms 节拍中断。 */
    SysTick_Config(SystemCoreClock / DELAY_MS_PER_SECOND);
}

/**
 * @brief 使用 DWT 周期计数器执行阻塞式微秒延时。
 *
 * 处理步骤：
 * 1. 时长或每微秒周期数为零时直接返回。
 * 2. 计算每段延时上限，并逐段等待目标周期数。
 *
 * @param[in] duration_us : 延时时长，单位为微秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 DWT 周期计数器运行。
 * @note 内核时钟变化后须重新初始化延时时基。
 */
void Delay_Us(uint32_t duration_us)
{
    uint32_t start_cycles = 0U;   /* 本段起始周期计数值 */
    uint32_t target_cycles = 0U;  /* 本段目标周期数 */
    uint32_t chunk_us = 0U;       /* 本段延时时长，单位为微秒 */
    uint32_t max_chunk_us = 0U;   /* 每段最大时长，单位为微秒 */

    /* 1. 时长为零或延时时基未初始化时直接返回。 */
    if ((0U == duration_us) || (0U == g_cycles_per_us))
    {
        return;
    }

    /* 2. 每段限制在计数器半周期内，避免乘法溢出。
     * 扩大完成条件的观察窗口，降低中断跨过该窗口的风险。
     */
    max_chunk_us = (UINT32_MAX / 2U) / g_cycles_per_us;

    /* 3. 逐段等待，直到完成请求的延时时长。 */
    while (0U != duration_us)
    {
        chunk_us = (duration_us > max_chunk_us) ? max_chunk_us : duration_us;
        target_cycles = chunk_us * g_cycles_per_us;
        start_cycles = DWT->CYCCNT;

        while ((uint32_t)(DWT->CYCCNT - start_cycles) < target_cycles)
        {
        }

        duration_us -= chunk_us;
    }
}

/**
 * @brief 使用 SysTick 递减计数执行阻塞式毫秒延时。
 *
 * 处理步骤：
 * 1. 将请求时长写入剩余毫秒计数。
 * 2. 等待 Delay_Tick() 将计数递减至零。
 *
 * @param[in] duration_ms : 延时时长，单位为毫秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 SysTick 正常运行。
 * @note 与 Delay_S() 共用计数器，不可重入或并发调用。
 * @note 不得在阻塞 SysTick 执行的中断上下文中调用。
 */
void Delay_Ms(uint32_t duration_ms)
{
    /* 1. 写入请求的毫秒延时时长。 */
    g_ms_left = duration_ms;

    /* 2. 等待 SysTick 中断将计数递减至零。 */
    while (0U != g_ms_left)
    {
    }
}

/**
 * @brief 逐秒调用毫秒延时，执行阻塞式秒延时。
 *
 * 每秒调用一次 1000 ms 延时，直到完成请求的秒数。
 *
 * @param[in] duration_s : 延时时长，单位为秒。
 *
 * @return 无返回值。
 * @note 调用前须完成 Delay_Init()，并保持 SysTick 正常运行。
 * @note 与 Delay_Ms() 共用计数器，不可重入或并发调用。
 * @note 不得在阻塞 SysTick 执行的中断上下文中调用。
 */
void Delay_S(uint32_t duration_s)
{
    /* 1. 将每秒请求转换为一次 1000 ms 延时。 */
    while (0U != duration_s)
    {
        Delay_Ms(DELAY_MS_PER_SECOND);
        duration_s--;
    }
}

/**
 * @brief 在 1 ms SysTick 中断中更新毫秒延时计数。
 *
 * 计数非零时递减一次，计数为零时保持不变。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 每次 1 ms SysTick 中断须调用一次。
 */
void Delay_Tick(void)
{
    /* 1. 仅在计数非零时递减，避免无符号下溢。 */
    if (0U != g_ms_left)
    {
        g_ms_left--;
    }
}

/************************ Function Definitions ************************/
