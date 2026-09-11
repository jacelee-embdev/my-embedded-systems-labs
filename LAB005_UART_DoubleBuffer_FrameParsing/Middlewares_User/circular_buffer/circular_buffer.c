/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file circular_buffer.c
 *
 * @par dependencies
 * - circular_buffer.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements circular-buffer creation, status, write, and read APIs.
 *
 * Processing flow:
 *
 * Create a circular-buffer object with create_empty_circular_buf(). Use
 * buf_write_data() to write at the head position and buf_read_data() to read
 * from the tail position. Both indices wrap at BUFFER_SIZE. One array position
 * remains unused so that empty and full states can be distinguished.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "circular_buffer.h"
/****************************** Includes ******************************/

/****************************** Defines *******************************/
#define TAG    "circular_buffer"
/****************************** Defines *******************************/

/**
 * @brief Creates an empty circular buffer.
 *
 * Allocates memory for a circular-buffer object and initializes all fields
 * to zero.
 *
 * @return circular_buf_t * Pointer to the created circular buffer.
 * @retval NULL Memory allocation failed.
 */
circular_buf_t *create_empty_circular_buf(void)
{
    circular_buf_t *p_circular_buf = NULL;

    /* Allocate the memory space. */
    p_circular_buf = (circular_buf_t *)malloc(sizeof(circular_buf_t));
    if (NULL == p_circular_buf)
    {
        elog_e(TAG, "Memory allocation failed\r\n");
        return NULL;
    }

    /* Initialize the allocated memory. */
    memset(p_circular_buf, 0, sizeof(circular_buf_t));

    return p_circular_buf;
}

/**
 * @brief Checks the current state of a circular buffer.
 *
 * @param[in] p_bufer Pointer to the circular buffer to be checked.
 *
 * @return buf_status_t Current state of the circular buffer.
 * @retval BUFFER_NULL   The buffer pointer is NULL.
 * @retval BUFFER_NORMAL The buffer is neither empty nor full.
 * @retval BUFFER_EMPTY  The buffer contains no data.
 * @retval BUFFER_FULL   The buffer has no writable position.
 */
buf_status_t check_buf_status(circular_buf_t *p_bufer)
{
    /* 1. Reject an invalid buffer pointer. */
    if (NULL == p_bufer)
    {
        elog_e(TAG, "Invalid parameter.\r\n");
        return BUFFER_NULL;
    }

    /* 2. Equal indices indicate that the buffer is empty. */
    if (p_bufer->head == p_bufer->tail)
    {
        return BUFFER_EMPTY;
    }

    /* 3. The buffer is full if the next head position reaches the tail. */
    if (((p_bufer->head + 1U) % BUFFER_SIZE) == p_bufer->tail)
    {
        return BUFFER_FULL;
    }

    /* 4. Otherwise, the buffer is available for reading and writing. */
    return BUFFER_NORMAL;
}

/**
 * @brief Writes one data item to a circular buffer.
 *
 * @param[in,out] p_bufer Pointer to the destination circular buffer.
 * @param[in] data Data item to be written.
 *
 * @return buf_wr_status_t Result of the write operation.
 * @retval BUFFER_WR_OK    The data was written successfully.
 * @retval BUFFER_WR_ERROR The buffer pointer is NULL or the buffer is full.
 */
buf_wr_status_t buf_write_data(circular_buf_t *p_bufer, data_type_t data)
{
    /* 1. Check that the buffer is valid and has writable space. */
    buf_status_t buf_status = check_buf_status(p_bufer);

    if ((BUFFER_NULL == buf_status) || (BUFFER_FULL == buf_status))
    {
        elog_e(TAG, "Buffer error!\r\n");
        return BUFFER_WR_ERROR;
    }

    /* 2. Write at the head, then advance the head with wraparound. */
    p_bufer->data[p_bufer->head] = data;
    p_bufer->head = (p_bufer->head + 1) % BUFFER_SIZE;

    return BUFFER_WR_OK;
}

/**
 * @brief Reads one data item from a circular buffer.
 *
 * @param[in,out] p_bufer Pointer to the source circular buffer.
 * @param[out] p_data Pointer to the variable that receives the data item.
 *
 * @return buf_wr_status_t Result of the read operation.
 * @retval BUFFER_WR_OK    The data was read successfully.
 * @retval BUFFER_WR_ERROR An input pointer is NULL or the buffer is empty.
 */
buf_wr_status_t buf_read_data(circular_buf_t *p_bufer,
                              data_type_t *p_data)
{
    /* 1. Check that the input pointers are valid and data is available. */
    buf_status_t buf_status = check_buf_status(p_bufer);

    if ((BUFFER_NULL == buf_status)  ||
        (BUFFER_EMPTY == buf_status) ||
        (NULL == p_data))
    {
        elog_e(TAG, "Buffer error or data error!\r\n");
        return BUFFER_WR_ERROR;
    }

    /* 2. Read at the tail, then advance the tail with wraparound. */
    *p_data = p_bufer->data[p_bufer->tail];
    p_bufer->tail = (p_bufer->tail + 1) % BUFFER_SIZE;

    return BUFFER_WR_OK;
}

