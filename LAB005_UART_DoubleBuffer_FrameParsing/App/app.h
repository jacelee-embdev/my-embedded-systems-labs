/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file app.h
 *
 * @par dependencies
 * - stdlib.h
 * - string.h
 * - usart.h
 * - FreeRTOS.h
 * - task.h
 * - queue.h
 * - semphr.h
 * - circular_buffer.h
 * - elog.h
 * - main.h
 * - bsp_uart_driver.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares UART receive application resources and initialization.
 *
 * Processing flow:
 *
 * Call app_init() once after EasyLogger initialization and before the scheduler
 * starts. The function creates the inter-task queues and starts the UART
 * receive, packet-parser, and application tasks.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __APP_H__
#define __APP_H__

/****************************** Includes ******************************/
#include <stdlib.h>             // The Compiler Library
#include <string.h>

#include "usart.h"              // Core layer

#include "FreeRTOS.h"           // OS layer
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "circular_buffer.h"

#include "elog.h"               // Middleware / BSP layer
#include "main.h"
#include "bsp_uart_driver.h"

/****************************** Includes ******************************/

/****************************** Defines *******************************/
#define MAX_PAYLOAD_LEN    (100U)  /* Maximum accepted payload length. */

typedef enum
{
    RX_WAIT_HEADER_1,  /* Wait for the first frame-header byte. */
    RX_WAIT_HEADER_2,  /* Wait for the second frame-header byte. */
    RX_WAIT_LENGTH,    /* Wait for the payload-length byte. */
    RX_READ_PAYLOAD,   /* Collect the configured payload bytes. */
    RX_READ_CHECK,     /* Wait for and verify the checksum byte. */
    RX_WAIT_TAIL       /* Wait for the frame-tail byte. */
} packet_parser_state_t; /* Receive packet-parser state. */

typedef enum
{
    PACKET_PARSE_IN_PROGRESS,  /* The current frame is incomplete. */
    PACKET_PARSE_COMPLETE,     /* A valid complete frame was received. */
    PACKET_PARSE_ERROR         /* The current frame is invalid. */
} packet_parse_result_t; /* Result produced after consuming one byte. */

typedef struct
{
    uint8_t len;                    /* Number of valid payload bytes. */
    uint8_t data[MAX_PAYLOAD_LEN];  /* Parsed payload storage. */
} parsed_packet_t; /* Payload delivered after a frame is validated. */

typedef struct
{
    packet_parser_state_t frame_state;  /* Current parser state. */
    uint8_t payload_index;              /* Next payload write index. */
    uint8_t calculated_checksum;        /* Running LEN + DATA sum. */
    parsed_packet_t parsed_packet;      /* Frame under construction. */
} packet_parser_t; /* Persistent packet-parser context. */

/****************************** Defines *******************************/

/************************ Function Declarations ************************/

/************************ Queue Handles ************************/
extern QueueHandle_t rx_ok_queue;         /* UART receive-completion queue. */
extern QueueHandle_t rx_to_parser_queue;  /* Packet-parser input queue. */

/**
 * @brief Initializes the UART receive queues and application tasks.
 *
 * Steps:
 * 1. Creates and validates all inter-task queues.
 * 2. Creates the UART receive task.
 * 3. Creates the packet-parser task.
 * 4. Creates the application task.
 *
 * @return None. Resource or task creation failures are logged and stop the
 *         remaining initialization.
 *
 * @note Call once after EasyLogger initialization and before the scheduler
 *       starts.
 */
void app_init(void);

/************************ Function Declarations ************************/

#endif /* __APP_H__ */
