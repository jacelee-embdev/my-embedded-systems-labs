/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file key.c
 *
 * @par dependencies
 * - key.h
 * - stm32f4xx_hal.h
 * - FreeRTOS.h
 * - task.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Configures the user key and polls its debounced pressed state.
 *
 * Processing flow:
 *
 * Call Key_Init() once before using Key_Scan(). Debouncing uses
 * FreeRTOS task delay while the scheduler runs, or HAL delay otherwise.
 *
 * @version v1.0 2026-10-16
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "key.h"  /* 用户 BSP 层：按键模块接口 */

#include "stm32f4xx_hal.h"  /* 厂商 HAL 层：GPIO、RCC 与时基接口 */

#include "FreeRTOS.h"  /* 中间件层：FreeRTOS 任务与节拍接口 */
#include "task.h"
/****************************** Includes ******************************/

/****************************** Defines ******************************/
#define KEY_GPIO_PORT         GPIOA       /* 按键端口 */
#define KEY_GPIO_PIN          GPIO_PIN_0  /* 按键引脚 */
#define KEY_DEBOUNCE_TIME_MS  10U         /* 消抖时间，单位为毫秒 */
/****************************** Defines ******************************/

/************************ Function Definitions ************************/

/**
 * @brief 执行 10 ms 按键消抖延时。
 *
 * 调度器运行时让出当前任务；其他启动或挂起阶段使用 HAL 延时。
 * 任务延时按节拍换算，至少等待一个节拍。
 *
 * @param 无。
 *
 * @return 无返回值。
 * @note 仅限启动阶段或任务上下文调用。
 */
static void Key_DelayDebounce(void)
{
    /* 目标消抖时间对应的系统节拍数。 */
    TickType_t delay_ticks = pdMS_TO_TICKS(KEY_DEBOUNCE_TIME_MS);

    if (taskSCHEDULER_RUNNING == xTaskGetSchedulerState())
    {
        vTaskDelay((0U != delay_ticks) ? delay_ticks : 1U);
    }
    else
    {
        HAL_Delay(KEY_DEBOUNCE_TIME_MS);
    }
}

/**
 * @brief 开启 GPIOA 时钟并将 PA0 配置为上拉输入。
 *
 * 处理步骤：
 * 1. 开启 GPIOA 外设时钟。
 * 2. 配置 PA0 为上拉输入，低电平表示按下。
 *
 * @param 无。
 *
 * @return 无返回值。
 */
void Key_Init(void)
{
    GPIO_InitTypeDef gpio_init_structure = {0};  /* 按键引脚配置 */

    /* 1. 开启 GPIOA 外设时钟。 */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 2. 将 PA0 配置为上拉输入。 */
    gpio_init_structure.Pin   = KEY_GPIO_PIN;
    gpio_init_structure.Mode  = GPIO_MODE_INPUT;
    gpio_init_structure.Pull  = GPIO_PULLUP;
    gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(KEY_GPIO_PORT, &gpio_init_structure);
}

/**
 * @brief 扫描按键并对按下状态进行消抖。
 *
 * 处理步骤：
 * 1. 读取按键电平，首次为低电平时进入消抖。
 * 2. 延时 10 ms 后再次采样。
 * 3. 两次采样均为低电平时确认按下，否则返回未按下。
 *
 * @param 无。
 *
 * @return 按键扫描结果。
 * @retval KEY_PRESSED  两次采样均为低电平，确认按下。
 * @retval KEY_RELEASED 未确认按下。
 * @note 返回按下状态，持续按住可重复返回 KEY_PRESSED。
 * @note 调用前须完成 Key_Init()，并保持 HAL 毫秒时基运行。
 * @note 调度器运行时按系统节拍延时，否则使用 HAL 延时。
 * @note 仅限启动阶段或任务上下文调用，不得在中断中调用。
 */
uint8_t Key_Scan(void)
{
    uint8_t key_state = KEY_RELEASED;  /* 按键扫描结果 */

    /* 1. 首次检测为低电平时进入消抖。 */
    if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN))
    {
        /* 2. 延时消抖后再次采样。 */
        Key_DelayDebounce();

        /* 3. 再次采样仍为低电平时确认按下。 */
        if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_GPIO_PORT, KEY_GPIO_PIN))
        {
            key_state = KEY_PRESSED;
        }
    }
    return key_state;
}

/************************ Function Definitions ************************/
