/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file bsp_uart_driver.h
 *
 * @par dependencies
 * - FreeRTOS.h
 * - task.h
 * - queue.h
 * - app.h
 * - circular_buffer.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the BSP UART receive task, buffers, and selection types.
 *
 * Processing flow:
 *
 * Run uart_rx_task() as a FreeRTOS task. It creates two circular-buffer objects
 * and alternates interrupt-driven reception between their data arrays. After
 * each completed transfer, it advances the corresponding write index and
 * sends the completed-buffer identifier to the packet-parser queue.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_UART_DRIVER_H__
#define __BSP_UART_DRIVER_H__

/****************************** Includes ******************************/
#include "FreeRTOS.h"           // OS layer
#include "task.h"
#include "queue.h"

#include "app.h"                // Application / middleware layer
#include "circular_buffer.h"
/****************************** Includes ******************************/

/****************************** Defines *******************************/
typedef enum
{
    BUF_NO = 0,  /* No receive buffer is selected. */
    BUF_1,       /* Receive buffer 1 is selected. */
    BUF_2        /* Receive buffer 2 is selected. */
} buf_curr_slt_t; /* Active UART receive-buffer selection. */
/****************************** Defines *******************************/

/************************ Function Declarations ************************/
/**
 * @brief Entry function for interrupt-driven UART reception.
 *
 * Creates two circular-buffer objects and alternates interrupt-driven reception
 * between them. For each completed transfer, it forwards the completed-buffer
 * identifier to the packet-parser queue and restarts reception with the
 * alternate buffer.
 *
 * @param[in] pvParameters Pointer to the task argument. This parameter is not
 *                         used.
 *
 * @return None. This task runs indefinitely.
 */
void uart_rx_task(void *pvParameters);

/**
 * @brief Returns one UART receive circular-buffer object.
 *
 * @param[in] buf_num Receive-buffer number. Valid values are 1 and 2.
 *
 * @return circular_buf_t * Pointer to the selected circular buffer.
 * @retval NULL The buffer number is invalid or the buffer is not initialized.
 */
circular_buf_t *get_circualr_buffer(uint8_t buf_num);

/************************ Function Declarations ************************/

#endif /* __BSP_UART_DRIVER_H__ */
