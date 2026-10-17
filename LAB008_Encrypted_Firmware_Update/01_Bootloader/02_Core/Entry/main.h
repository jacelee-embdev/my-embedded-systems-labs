/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file main.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the retained APP jump request and its marker.
 *
 * Processing flow:
 *
 * Bootloader stores a jump request and resets the MCU.
 * main() clears a matching request before calling the APP entry.
 *
 * @version V1.0 2026-10-15
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __MAIN_H__
#define __MAIN_H__

/****************************** Includes ******************************/

#include <stdint.h>  /* C 标准库 */

/****************************** Includes ******************************/

/******************************* Defines *******************************/

/* 复位后由 main() 消费的一次性 APP 跳转请求 */
#define APP_JUMP_REQUEST_MAGIC (0x55AA55AAU)

/******************************* Defines *******************************/

/************************** External Variables **************************/

/* main.c 定义的保留 RAM 请求，复位后由入口消费。 */
extern volatile uint32_t g_app_jump_request;

/************************** External Variables **************************/

#endif /* __MAIN_H__ */
