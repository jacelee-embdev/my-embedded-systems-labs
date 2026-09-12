/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file test.h
 *
 * @par dependencies
 * - stdio.h
 * - FreeRTOS.h
 * - task.h
 * - main.h
 * - usart.h
 * - SEGGER_RTT.h
 * - elog.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the UART circular-DMA Receive-to-Idle test interface.
 *
 * Processing flow:
 *
 * Call test_init() after the RTOS kernel objects can be created. The function
 * creates the task that starts and observes UART Receive-to-Idle DMA reception.
 *
 * @version V1.0 2026-09-12
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __TEST_H__
#define __TEST_H__

/****************************** Includes ******************************/
#include <stdio.h>  /* Compiler library. */

#include "FreeRTOS.h"  /* OS layer. */
#include "task.h"

#include "main.h"
#include "usart.h"

#include "SEGGER_RTT.h"  /* Middleware and BSP layer. */
#include "elog.h"
/****************************** Includes ******************************/

/****************************** Declaring ******************************/
/************************ API Declaration ************************/

/**
 * @brief Creates the UART DMA Receive-to-Idle test task.
 *
 * Creates the application task responsible for starting and observing USART1
 * circular DMA reception.
 *
 * @param None.
 *
 * @return None.
 */
void test_init(void);

/****************************** Declaring ******************************/

#endif /* End of __TEST_H__ */
