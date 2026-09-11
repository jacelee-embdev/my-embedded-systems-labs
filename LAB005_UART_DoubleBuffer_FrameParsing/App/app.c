/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file app.c
 *
 * @par dependencies
 * - app.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements UART frame parsing and application-task delivery.
 *
 * Processing flow:
 *
 * app_init() creates the inter-task queues and starts the UART receive,
 * packet-parser, and application tasks. The parser consumes bytes from the
 * completed receive buffer, validates each protocol frame, and forwards valid
 * packet context to the application task.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "app.h"
/****************************** Includes ******************************/

/****************************** Defines *******************************/
#define TAG               "app"
#define MSG_PASE_OK       (0xF5U)  /* Reserved packet-complete message. */
#define FRAME_HEADER_1    (0xFFU)  /* First frame-header byte. */
#define FRAME_HEADER_2    (0xFEU)  /* Second frame-header byte. */
#define FRAME_TAIL        (0xAAU)  /* Frame-tail byte. */
/****************************** Defines *******************************/

/************************ Queue Handles ************************/
QueueHandle_t rx_ok_queue        = NULL;
QueueHandle_t rx_to_parser_queue = NULL;
QueueHandle_t packet_ok_queue    = NULL;

/*************************** Static Resources ************************/
static TaskHandle_t uart_rx_task_handle       = NULL;
static TaskHandle_t packet_parser_task_handle = NULL;
static TaskHandle_t app_task_handle           = NULL;

static circular_buf_t  *g_p_circular_buf1_from_driver = NULL;
static circular_buf_t  *g_p_circular_buf2_from_driver = NULL;
/*************************** Static Resources ************************/

/**
 * @brief Feeds one received byte into the UART packet state machine.
 *
 * The parser recognizes FF FE, validates the payload length, collects payload
 * bytes, verifies the LEN + DATA checksum, and checks the AA frame tail.
 * Parser context is preserved by the caller between calls.
 *
 * @param[in,out] parser Pointer to the persistent packet-parser context.
 * @param[in] data       Byte read from the UART receive buffer.
 *
 * @return packet_parse_result_t Result after consuming the byte.
 * @retval PACKET_PARSE_IN_PROGRESS The current frame is incomplete.
 * @retval PACKET_PARSE_COMPLETE    A complete valid frame was received.
 * @retval PACKET_PARSE_ERROR       The parser pointer or frame is invalid.
 */
static packet_parse_result_t packet_parser_feed_byte(packet_parser_t *parser,
                                                     uint8_t data)
{
    packet_parse_result_t result = PACKET_PARSE_IN_PROGRESS;

    /* Step 1: Reject an invalid parser context. */
    if (NULL == parser)
    {
        return PACKET_PARSE_ERROR;
    }

    /* Step 2: Consume the byte according to the current parser state. */
    switch (parser->frame_state)
    {
        case RX_WAIT_HEADER_1:
            /* Start a frame after receiving the first header byte. */
            if (FRAME_HEADER_1 == data)
            {
                parser->frame_state = RX_WAIT_HEADER_2;
            }
            break;

        case RX_WAIT_HEADER_2:
            /* Accept FE, retain an overlapping FF, or restart the search. */
            if (FRAME_HEADER_2 == data)
            {
                parser->frame_state = RX_WAIT_LENGTH;
            }
            else
            {
                if (FRAME_HEADER_1 != data)
                {
                    parser->frame_state = RX_WAIT_HEADER_1;
                }
            }
            break;

        case RX_WAIT_LENGTH:
            /* Validate length and initialize payload collection. */
            if ((data > 0U) && (data <= MAX_PAYLOAD_LEN))
            {
                parser->parsed_packet.len   = data;
                parser->payload_index       = 0;
                parser->calculated_checksum = parser->parsed_packet.len;
                parser->frame_state         = RX_READ_PAYLOAD;
            }
            else
            {
                parser->frame_state = RX_WAIT_HEADER_1;
                result              = PACKET_PARSE_ERROR;
            }
            break;

        case RX_READ_PAYLOAD:
            /* Store one payload byte and update the running checksum. */
            parser->parsed_packet.data[parser->payload_index] = data;
            parser->payload_index++;
            parser->calculated_checksum += data;

            if (parser->payload_index >= parser->parsed_packet.len)
            {
                parser->frame_state = RX_READ_CHECK;
            }
            break;

        case RX_READ_CHECK:
            /* Compare the received checksum with the accumulated value. */
            if (parser->calculated_checksum == data)
            {
                parser->frame_state = RX_WAIT_TAIL;
            }
            else
            {
                parser->frame_state = RX_WAIT_HEADER_1;
                result              = PACKET_PARSE_ERROR;
            }
            break;

        case RX_WAIT_TAIL:
            /* Complete a valid frame or reject an invalid tail byte. */
            if (FRAME_TAIL == data)
            {
                result = PACKET_PARSE_COMPLETE;
            }
            else
            {
                result = PACKET_PARSE_ERROR;
            }
            parser->frame_state = RX_WAIT_HEADER_1;
            break;

        default:
            /* Recover from a corrupted or unsupported parser state. */
            parser->frame_state = RX_WAIT_HEADER_1;
            result              = PACKET_PARSE_ERROR;
            break;
    }

    return result;
}

/**
 * @brief Entry function for the packet-parser task.
 *
 * Waits for completed-buffer identifiers from the UART receive task, drains
 * the selected circular buffer, and feeds each byte to the packet parser.
 * Valid packet context is forwarded to packet_ok_queue.
 *
 * @param[in] pvParameters Pointer to the task argument. This parameter is not
 *                         used.
 *
 * @return None. This task runs indefinitely.
 */
static void packet_parser_task(void *pvParameters)
{
    (void)pvParameters;

    uint32_t msg_rx_ok     = 0;
    packet_parser_t parser =
    {
        .frame_state         = RX_WAIT_HEADER_1,
        .payload_index       = 0U,
        .calculated_checksum = 0U,
        .parsed_packet       = {0}
    };
    packet_parser_t *p_parser = &parser;

    /* Step 1: Obtain the two receive buffers created by the UART task. */
    g_p_circular_buf1_from_driver = get_circualr_buffer(1);
    g_p_circular_buf2_from_driver = get_circualr_buffer(2);

    for (;;)
    {
        /* Step 2: Wait for the identifier of a completed receive buffer. */
        if (pdPASS != xQueueReceive(rx_to_parser_queue,
                                    &msg_rx_ok,
                                    portMAX_DELAY))
        {
            elog_e(TAG, "Failed to receive queue.");
        }

        /* Step 3: Stop if the parser-input queue is not available. */
        if (NULL == rx_to_parser_queue)
        {
            elog_e(TAG, "Invalid queue.");
            Error_Handler();
        }

        /* Step 4: Drain and parse every byte from the completed buffer. */
        buf_status_t buf_status = BUFFER_NULL;
        switch (msg_rx_ok)
        {
            case BUF_1:
                /* Consume all currently available bytes from buffer 1. */
                do
                {
                    uint8_t temp_data = 0;
                    if (BUFFER_WR_OK !=
                        buf_read_data(g_p_circular_buf1_from_driver,
                                      &temp_data))
                    {
                        elog_e(TAG, "Buffer read failed.");
                    }
                    else
                    {
                        packet_parse_result_t result =
                            packet_parser_feed_byte(&parser, temp_data);

                        if (PACKET_PARSE_COMPLETE == result)
                        {
                            if (pdPASS != xQueueSend(packet_ok_queue,
                                                     &p_parser,
                                                     0))
                            {
                                elog_e(TAG, "Queue send failed.\r\n");
                            }
                        }
                        else if (PACKET_PARSE_ERROR == result)
                        {
                            elog_e(TAG, "Data parsing failed.");
                        }
                    }

                    buf_status =
                        check_buf_status(g_p_circular_buf1_from_driver);
                } while ((BUFFER_EMPTY != buf_status) &&
                         (BUFFER_NULL != buf_status));
                break;

            case BUF_2:
                /* Consume all currently available bytes from buffer 2. */
                do
                {
                    uint8_t temp_data = 0;
                    if (BUFFER_WR_OK !=
                        buf_read_data(g_p_circular_buf2_from_driver,
                                      &temp_data))
                    {
                        elog_e(TAG, "Buffer read failed.");
                    }
                    else
                    {
                        packet_parse_result_t result =
                            packet_parser_feed_byte(&parser, temp_data);

                        if (PACKET_PARSE_COMPLETE == result)
                        {
                            if (pdPASS != xQueueSend(packet_ok_queue,
                                                     &p_parser,
                                                     0))
                            {
                                elog_e(TAG, "Queue send failed.\r\n");
                            }
                        }
                        else if (PACKET_PARSE_ERROR == result)
                        {
                            elog_e(TAG, "Data parsing failed.");
                        }
                    }

                    buf_status =
                        check_buf_status(g_p_circular_buf2_from_driver);
                } while ((BUFFER_EMPTY != buf_status) &&
                         (BUFFER_NULL != buf_status));
                break;

            default:
                break;
        }

#if 1
        elog_i(TAG, "msg_rx_ok:%x\r\n", msg_rx_ok);
#endif

//        elog_i(TAG, "Thread packet_parser test.\r\n");
//        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

/**
 * @brief Entry function for the application task.
 *
 * Waits for a validated packet-parser context and logs every valid payload
 * byte received from the packet-parser task.
 *
 * @param[in] pvParameters Pointer to the task argument. This parameter is not
 *                         used.
 *
 * @return None. This task runs indefinitely.
 */
static void app_task(void *pvParameters)
{
    (void)pvParameters;

    packet_parser_t *p_parser = NULL;

    for (;;)
    {
        /* Step 1: Wait for a validated packet from the parser task. */
        if (pdPASS != xQueueReceive(packet_ok_queue,
                                    &p_parser,
                                    portMAX_DELAY))
        {
            elog_e(TAG, "Failed to receive queue.");
        }

        /* Step 2: Report all valid payload bytes in the received packet. */
        for (uint8_t i = 0; i < p_parser->parsed_packet.len; i++)
        {
            elog_i(TAG,
                   "Payload%d:[%x]\r\n",
                   i,
                   p_parser->parsed_packet.data[i]);
        }

#if 1
        elog_i(TAG, "p_parser:%x\r\n", p_parser);
#endif
//        elog_i(TAG, "Thread app test.\r\n");
//        vTaskDelay(pdMS_TO_TICKS(3000U));
    }
}


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
void app_init(void)
{
    /* Step 1: Create the receive, parser, and valid-packet queues. */
    rx_ok_queue        = xQueueCreate(1, sizeof(uint32_t));
    rx_to_parser_queue = xQueueCreate(1, sizeof(uint32_t));
    packet_ok_queue    = xQueueCreate(1, sizeof(packet_parser_t *));
    if ((NULL == rx_ok_queue)        ||
        (NULL == rx_to_parser_queue) ||
        (NULL == packet_ok_queue))
    {
        elog_e(TAG, "Queue creation failed.\r\n");
        return;
    }

    /* Step 2: Create the UART receive task. */
    if (pdPASS != xTaskCreate(uart_rx_task,
                              "uart_rx",
                              128U,
                              NULL,
                              1U,
                              &uart_rx_task_handle))
    {
        elog_e(TAG, "UART receive task creation failed.\r\n");
        return;
    }

    /* Step 3: Create the packet-parser task. */
    if (pdPASS != xTaskCreate(packet_parser_task,
                              "packet_parser",
                              128U,
                              NULL,
                              1U,
                              &packet_parser_task_handle))
    {
        elog_e(TAG, "Packet parser task creation failed.\r\n");
        return;
    }

    /* Step 4: Create the application task. */
    if (pdPASS != xTaskCreate(app_task,
                              "app",
                              128U,
                              NULL,
                              1U,
                              &app_task_handle))
    {
        elog_e(TAG, "Application task creation failed.\r\n");
        return;
    }

    elog_i(TAG, "App initialed successfully.\r\n");
}

